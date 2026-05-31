# Terminus Module — Design

## What is Terminus?

Terminus is the data hub and application scaffolding for nioc robotic platforms. It does three
things:

1. **Routes data** between hardware drivers, software components, and persistent storage.
2. **Owns the recording directory** — creates it, places chronicle inside it, loads and merges
   config into it, copies in any extra files the application calls out as logged resources, and
   writes the metadata that lets a playback binary find everything again.
3. **Provides lifecycle primitives** (`Routine`, `Runner`) so applications have a small, readable
   `main()`.

The same binary handles live operation and log playback. Downstream components are mode-agnostic
because in playback mode the log reader is *itself a driver*. Port routes data the same way in both
modes.

Everything lives in one module: `modules/terminus/`, namespace `nioc::terminus`.

## Implementation status (first slice)

The first vertical slice — driving the bagConverter: a `RosbagPlayer` driver on a `ThreadRunner`
publishing Imu/PointCloud through Port, a `MessageCounter` component (in `nioc::example`)
subscribing and counting — is built. What exists today:

- `Routine` (`routine.hpp`): `step()` performs one iteration and returns `State{Continue, Waiting,
  Done}` (throw = failure). It takes no arguments — the Runner owns the loop and the stop token.
- `Component` (`component.hpp`): a bounded **inbox**, a `boost::circular_buffer` of
  `pair<ChannelId, shared_ptr<const MsgBase>>` sized at construction. The port-side subscription
  callback calls `push(channelId, message)`; `step()` drains one entry and dispatches it to the
  single handler registered for that channel. `OverflowPolicy{Overwrite, Block}` decides what a
  full inbox does.
- `Runner` + `ThreadRunner` (`runner.hpp`): one thread per Routine; the loop watches the stop token
  between iterations and keeps running/stopped/failed counters.
- `Publisher<Schema>` and `Port::publisher` / `Port::subscribe` returning a `[[nodiscard]]`
  `Subscription`; per-topic type binding; channel id = hash(topic, msgType).
- Port runs two worker threads: one fans each published message out to subscribers, one drains a
  queue into the chronicle writer. A published message is shared as `shared_ptr<const MsgBase>` to
  both; it is immutable after publish and the two threads read it concurrently.

Deferred (sketched elsewhere in this document, not yet built): the readiness-based
`ThreadPoolRunner` scheduler (designed below), the `Disposition` end-state enum, `SignalCatcher`,
playback mode, `topics.txt`, lock-free dispatch (the first cut guards the subscriber table with a
mutex — which also gives `Subscription` teardown its wait-for-in-flight guarantee), and the
`Port::allocator()` zero-copy path.

## Modes of operation

- **Live** — `Port{rootDir, configPaths, cmdline}`. Port creates a fresh recording subdirectory
  under `rootDir` and writes everything into it. Hardware drivers come online and push data
  through Port.
- **Playback** — `Port::playback(existingRecordingDir)`. Port opens the existing recording
  read-only and exposes the config + resources from it. A `LogReader` driver replays the chronicle
  through Port; downstream components consume it the same way they do live.
- **Re-log** — playback dir as input to a `LogReader` driver, plus a Port in live mode writing a
  fresh recording. The two recordings are independent on disk; the LogReader bridges them. Detailed
  semantics deferred until we have a concrete need.

The mode of the binary is determined by **which Port constructor was used** and **which Routines
main() constructs** (hardware drivers vs. LogReader). There is no `Mode` enum on Routine or
Dispatcher.

## System at a glance

```
                              main
                                |
        +-------+---------+-----+-----+
        |       |                     |
       CLI    Port               SignalCatcher
              |                       |
   +----------+-----------+      request_stop on
   |          |           |        driverStop
   v          v           v
recordingDir   Dispatcher   ConfigManager
   |              ^               |
   |              |          [json merge]
   |     publisher/subscribe       |
   |              |                v
   +-- chronicle/ |          config.json
   +-- config.json|          (under recordingDir)
   +-- metadata.json
   +-- <flat resource files>
                  |
              components and drivers
              (each owns shared_ptr to itself;
               Runner runs the iteration loop)
```

Port is the central object — it owns the recording directory, the chronicle writer, the config
manager, the topic registry, the dispatcher, and (when recording) the metadata bookkeeping.
Components and drivers see only `Publisher<Schema>` and `Subscription` handles; they do not see
the directory or chronicle.

`LogReader` is an ordinary `Driver`. It has no privileged relationship with Port; it publishes
through normal `Publisher<Schema>` handles.

## What `main()` looks like

Pseudocode for a live binary. The CLI follows the existing `bagConverter` main: boost
program_options, `argC`/`argV`, help short-circuits, `po::notify` after the help check, errors
caught and printed with usage.

```cpp
namespace po = boost::program_options;
using namespace nioc::terminus;

int main(const int argC, const char* const* const argV) {
    auto options = po::options_description{"myRobot options"};
    options.add_options()
        ("help,h", "Print this help message")
        ("config,c", po::value<std::vector<std::string>>()->multitoken()->required(),
                     "Config JSON files (merged in order)")
        ("recordingsRoot,r", po::value<std::string>()->required(),
                     "Parent directory under which a fresh recording will be created");

    try {
        auto vm = po::variables_map{};
        po::store(po::parse_command_line(argC, argV, options), vm);
        if(vm.contains("help")) { std::cout << options << '\n'; return EXIT_SUCCESS; }
        po::notify(vm);

        // 1. Phased shutdown.
        auto driverStop    = std::stop_source{};
        auto componentStop = std::stop_source{};

        // 2. RAII: block SIGINT/TERM/HUP, sigwait thread, request_stop on driverStop.
        auto signals = SignalCatcher{driverStop};

        // 3. The central hub. Port:
        //    - creates <recordingsRoot>/<isoTimestamp>_<uuid>/
        //    - loads and merges the listed config files; writes config.json under it
        //    - opens chronicle::Writer at <port.dir()>/chronicle/
        //    - writes metadata.json (cmdline + resource map) on destruction
        auto port = Port{
            vm.at("recordingsRoot").as<std::string>(),
            vm.at("config").as<std::vector<std::string>>(),
            std::span{argV, static_cast<std::size_t>(argC)}};

        // 4. Optional: declare resource files to log alongside the recording.
        //    Each file is copied flat into port.dir() and recorded in metadata.json
        //    as {originalPath -> flatName}. acquireResource() returns the live path
        //    in live mode and the in-recording path in playback mode (TODO: relative-path
        //    simplification, see TODOs).
        port.addResource("/etc/myrobot/urdf/robot.urdf");
        port.addResource("/etc/myrobot/calibration/imu.yaml");

        // 5. Components. Held as shared_ptr.
        auto componentRunner = ThreadPoolRunner{componentStop, /*threads=*/4};

        auto fusion = std::make_shared<FusionPipeline>(
            port.config().at("fusion"),
            port.publisher<Pose>("fused_pose"));
        auto _fusionImuSub = port.subscribe<Imu>(  "imu",    fusion, &FusionPipeline::onImu);
        auto _fusionCamSub = port.subscribe<Image>("camera", fusion, &FusionPipeline::onImage);
        // Real code stores the Subscriptions inside the component; shown here at call site
        // for visibility. The component's destructor unsubscribes via these members.
        componentRunner.launch(fusion);

        // 6. Drivers. Acquire Publisher handles from Port.
        auto driverRunner = ThreadRunner{driverStop};

        auto imuDrv = std::make_shared<ImuDriver>(
            port.config().at("imu"),
            port.publisher<Imu>("imu"),
            wireToImu);
        driverRunner.launch(imuDrv);

        auto camDrv = std::make_shared<CameraDriver>(
            port.config().at("camera"),
            port.publisher<Image>("camera"),
            wireToImage);
        driverRunner.launch(camDrv);

        // 7. Kick off — drivers start producing data. Main waits.
        driverRunner.waitUntilAllStopped();

        // 8. Teardown — components drain; Port flushes chronicle + writes metadata.json.
        componentStop.request_stop();
        componentRunner.join();

        return exitStatus(signals, driverRunner, componentRunner);
    }
    catch(const po::error& e) {
        std::cerr << "Error: " << e.what() << '\n' << options << '\n';
        return EXIT_FAILURE;
    }
    catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
```

A **playback** binary uses the playback factory and constructs a `LogReader` driver instead of
hardware drivers:

```cpp
        auto port = Port::playback(vm.at("recording").as<std::string>());
        // ... same Runners, SignalCatcher, etc.

        auto reader = std::make_shared<LogReader>(
            port,
            port.publisher<Imu>(  "imu"),
            port.publisher<Image>("camera"),
            port.publisher<Pose>( "fused_pose"));
        driverRunner.launch(reader);
```

`port.config()` reads from the recording's `config.json`. `port.acquireResource(originalPath)`
remaps the path to the in-recording location. `LogReader::step()` reads one entry per iteration;
when the recording is exhausted it returns `Done` and `driverRunner` considers it stopped.

## The pieces

### Port — the central hub

Port is the central object. It is the data router, and it is also the owner of the recording
directory.

Internals (none exposed publicly):

- `TopicRegistry` — maps topic names to channel IDs (the 64-bit hash of `(topicName, msgType)`);
  records first-publish events so `topics.txt` can be appended.
- `Dispatcher` — pub/sub fan-out. Subscriber list stored as raw entries; lifetime managed by
  `Subscription` RAII tokens (see Ownership).
- `chronicle::Writer` (live mode) — opens at `<port.dir()>/chronicle/`.
- `ConfigManager` — loads JSON sources, merges them, writes `config.json` at `<port.dir()>/`,
  exposes the merged result.
- Resource map — `originalPath → flatFilename`, written into `metadata.json` on shutdown.

Construction (live mode):

```cpp
Port(std::filesystem::path recordingsRoot,
     std::vector<std::string> configFilePaths,
     std::span<const char* const> cmdline);
```

At construction Port:

1. Creates `<recordingsRoot>/<isoTimestamp>_<uuid>/` and remembers it as `dir()`.
2. Creates `<dir()>/chronicle/` and hands its path to a `chronicle::Writer`.
3. Hands `configFilePaths` to `ConfigManager`, which loads and merges them; writes the merged
   result to `<dir()>/config.json`.
4. Records `cmdline` for later inclusion in `metadata.json`.

On destruction Port writes `<dir()>/metadata.json` and flushes chronicle.

Construction (playback mode):

```cpp
static Port playback(std::filesystem::path recordingDir);
```

Opens an existing recording read-only: reads `config.json` and `metadata.json`, exposes the same
`config()` / `acquireResource()` / `publisher` / `subscribe` API. No chronicle writer.

Public API:

- `dir() → const std::filesystem::path&` — the recording directory.
- `config() → const nlohmann::json&` — the merged config (live) or loaded config (playback).
- `addResource(std::filesystem::path source)` — live only. Copies the file flat into `dir()`
  under its basename. Records `source → basename` in the manifest. Throws on basename collision.
- `acquireResource(std::filesystem::path source) → std::filesystem::path` — in live mode returns
  `source` unchanged; in playback mode returns the in-recording path for the resource that was
  originally `source`.
- `publisher<Schema>(std::string_view topic) → Publisher<Schema>` — creates the channel on first
  call for the topic name; subsequent calls return a handle to the existing channel.
- `subscribe<Schema>(std::string_view topic, std::shared_ptr<Owner>, MemberFn) → [[nodiscard]] Subscription`
- `subscribe<Schema>(std::string_view topic, std::shared_ptr<Owner>, Lambda) → [[nodiscard]] Subscription`

Topic-name → type binding is enforced internally: if `publisher<Imu>("imu")` is followed by
`subscribe<Image>("imu", ...)`, the second call throws. First use of a topic name binds its type
for the lifetime of the Port.

The chronicle writer is intentionally **not** a Component the user constructs. It's a process-level
participant inside Port. The future zero-copy story expects Port to expose the writer's backing
storage as an `Allocator` interface so components and drivers can build capnp messages directly
into recording-backed memory; keeping the writer inside Port lets `Port::allocator()` be a clean
factory method when we get there.

### Routine, Component, Driver

`Routine` is the base for anything that does work in iterations. The **loop lives in the Runner**;
`step()` performs one iteration and returns a `State` telling the Runner what to do next.

```cpp
class Routine {
public:
    enum class State : std::uint8_t { Continue, Waiting, Done };   // failure → throw

    virtual ~Routine() = default;
    [[nodiscard]] virtual State step() = 0;
    [[nodiscard]] virtual std::string_view name() const = 0;   // for logs only
};
```

- `Continue` — has more work now; schedule another iteration immediately.
- `Waiting` — no work right now; reschedule later rather than spinning (an empty inbox is the
  canonical case).
- `Done` — finished naturally (end of log, time limit, sentinel reached); stop scheduling.
- Throw — failed. The Runner catches, logs, and stops scheduling it.

`step()` takes no arguments. An earlier version passed a `std::stop_token` into every iteration so
a blocking iteration could be woken; that was removed. The model is a *non-blocking* single
iteration that signals idleness with `Waiting`, while the Runner owns the stop token and checks it
between iterations. A Routine that genuinely must block until stop (a driver on a blocking syscall)
will get its stop hook through a separate mechanism, not a per-iteration argument (see TODOs).

`State` is purely the routine's *forward* signal — what to do next. It deliberately has no
`Stopped`/`Failed`/`Error` cases: how a routine's lifetime *ended* (completed vs. stopped
externally vs. failed) is a separate, backward-looking concern the Runner owns (its
running/stopped/failed counters; a named `Disposition` enum is a deferred refinement — see TODOs).
Failure is reported by throwing, which carries a diagnostic message a bare `Error` state could not.

The interface is intentionally small. No `vitals()`, no health enum. Whether a Routine is alive is
something the Runner knows (it scheduled it); whether it's "stalled" or "degraded" is application
territory.

`Component : Routine` — subscribes via Port, may publish via Port. Held by `main` as
`std::shared_ptr<Component>` so the Runner can keep a reference for the duration of work.

Each Component owns a bounded **inbox**: a `boost::circular_buffer` of
`std::pair<ChannelId, ConstMsgBasePtr>`, sized at construction. The channel id is
stored alongside the message so a drained entry knows which handler it belongs to.

The two dispatch pathways are set up by the templated `subscribe<Schema>(topic, callback)`:

1. **Port → inbox.** On first subscription to a channel, the Component registers a callback with
   Port that calls `push(channelId, message)`, with the channel id captured in that callback (where
   it is known). Port holds the callback weakly; the Component owns the `shared_ptr` that keeps it
   alive — one per subscribed channel.
2. **inbox → handler.** `subscribe` stores **one** type-erasing handler per channel (a
   `std::function<void(ConstMsgBasePtr)>` that down-casts to `Msg<Schema>` and
   invokes the user callback). Re-subscribing a channel replaces that handler: one handler per
   channel, last-writer-wins — compose multiple behaviors in one lambda rather than registering
   several.

`step()` drains one entry under the inbox mutex, then (outside the lock) invokes the handler
registered for that entry's channel; an empty inbox returns `State::Waiting`. The inbox's
`OverflowPolicy` decides what a full inbox does: `Overwrite` drops the oldest (live capture — fresh
data beats stale), `Block` backpressures the producer until a slot frees (playback — lossless).

`Driver : Routine` — a plain base (no `Wire` template). A driver holds one or more
`Publisher<Schema>` handles and does its `wire→msg` conversion inside `step()`. This is deliberately
not templated on a single wire type: a real source (a ROS bag, a multi-sensor device) carries
several message types on several topics, so it owns several typed publishers and routes each input
to the right one. Publishes only.

The user constructs each Component or Driver with its config and its publishers (if any), then
registers any subscriptions with Port, then launches it on a Runner. Component and Driver classes
themselves never see Port for publishing — they hold `Publisher` handles and member functions Port
can dispatch to. (A subscribing Component does take `Port&` at construction to register its
subscriptions.) Symmetry: both sides see typed handles for the message flow.

### Ownership — `Subscription` RAII tokens

`subscribe()` returns a `[[nodiscard]] Subscription`. The Subscription's destructor calls
`Port::unsubscribe()`, which removes the entry from the dispatch table and waits for any in-flight
dispatch on that subscriber to complete before returning.

Components store their `Subscription`s as members, so a Component's destruction naturally tears
down its subscriptions:

```cpp
class FusionPipeline : public Component {
    Subscription _imuSub;
    Subscription _camSub;
public:
    FusionPipeline(Config, Port& port, Channel<Imu> imu, Channel<Image> cam)
      : _imuSub{port.subscribe(imu, this, &FusionPipeline::onImu)},
        _camSub{port.subscribe(cam, this, &FusionPipeline::onImage)} {}
};
```

When `main` resets its `shared_ptr<FusionPipeline>`, the Component is destroyed; its `Subscription`
members destruct first and unsubscribe from Port; then the Component itself goes away. No dangling
calls.

The cost lives at unsubscribe time, not at dispatch time. The dispatch path stores raw pointers
(or a stable typed handle) for its subscribers and iterates them unconditionally — no
`weak_ptr.lock()` per message. The trade-off is intentional: a Component publishes through Port
at message rate (kHz for IMU); it unsubscribes once, ever.

Port itself is stored on the stack in `main` and outlives every Component and Driver it serves —
no need for shared ownership of Port.

`Publisher<Schema>` is a thin handle into Port; it pays no per-call lifetime check either. If Port
is destroyed before a publisher tries to use it, that's a wiring bug — caught by construction
order in `main`.

### Publisher — deferred design

A `Publisher<Schema>` is the handle a Routine uses to push messages into Port. Open shape
questions: lifetime model for the message (`std::unique_ptr<Msg<...>>` that can be promoted to
`shared_ptr<const ...>`, vs. ownership transfer into Port), whether a Component publishes through a
captured `Port*` instead of a typed handle, whether outputs are namespaced under the Component so
the Component itself decides what to publish in its own namespace. There is enough logic here that
a real `Publisher` type may be warranted instead of a bare lambda. **Iterate on this once a real
component is being written.** For the first scaffolding pass, treat `Publisher<Schema>` as an
opaque typed handle obtained from `port.publisher(channel)`.

### Runner — executor abstraction

Runner owns the iteration loop. It's constructed with a `std::stop_source`; the Runner watches that
source's token between a Routine's iterations (the Routine itself no longer receives the token).

```cpp
class Runner {
public:
    virtual ~Runner() = default;
    virtual void launch(std::shared_ptr<Routine>) = 0;
    virtual void waitUntilAllStopped() = 0;
    virtual std::size_t runningCount() const = 0;
    virtual std::size_t stoppedCount() const = 0;
    virtual std::size_t failedCount() const = 0;
};
```

The loop, shared by every Runner implementation:

```cpp
void runLoop(Routine& routine, std::stop_token tok) {
    try {
        while(!tok.stop_requested()) {
            if(routine.step() == Routine::State::Done) break;
        }
    } catch(const std::exception& e) {
        // log routine.name() and e.what() to console; increment failedCount.
    }
    // increment stoppedCount.
}
```

`ThreadRunner` today re-runs a `Waiting` routine immediately — a spin. Proper `Waiting` handling
(parking the routine off-thread until an external wake, rather than re-running or sleeping) is the
readiness-based `ThreadPoolRunner` design below.

Per-Routine diagnostic detail beyond the three counters is not tracked by the framework. If `main`
needs to know "which Routine failed and why," it reads the console log written by the catch clause
above.

Implementations:

- `ThreadRunner` — one thread per Routine; the loop watches the stop token between iterations.
  Best for drivers and any long-lived component. Built today.
- `ThreadPoolRunner` — fixed pool shared by many Routines, scheduled by readiness. Best for many
  small, idle-most-of-the-time components. Designed below; not yet built.
- `FiberRunner` — optional, later.

Mix runners freely. A driver typically wants its own thread; a fleet of mostly-idle pipeline stages
may share a pool.

### ThreadPoolRunner — readiness-based scheduling (design, not yet built)

`ThreadRunner` gives each Routine its own thread, so `Waiting` can be a crude spin on that one
thread. That does not scale: N components that are all idle should cost *zero* running threads, and
when one of them gets a message exactly that one should run, on a shared pool. This is a
readiness-based scheduler — the same "don't wait on N things; collapse to one ready queue and
park/wake against it" move used elsewhere in the system.

**Shape.**

- **Executor** = a fixed thread pool plus one **ready queue** (a mutex + condition variable, or a
  counting semaphore) of *runnable* scheduling handles. Worker threads block on the ready queue,
  pop a handle, run one `step()`, then requeue / park / retire by its result.
- **Runner** (the per-Routine "one each") becomes a lightweight **scheduling record**, not a
  thread: it owns the Routine, a scheduling state, and a back-pointer to the Executor, and exposes
  `wake()`.
- **Wake path.** A Component, after `push()` inserts into its inbox, calls a `wake` callback
  injected at registration (which keeps Component decoupled from the scheduler). `Runner::wake()`
  enqueues the record on the Executor and notifies the one ready-queue CV. A free worker pops *that
  record* and runs it — "which component" is answered by the queue carrying the handle, not by the
  pool guessing.

**`step()` result → action**, applied by the worker after each `step()`:

- `Continue` → re-enqueue (more work asserted now),
- `Waiting` → **park** (do nothing; only an external `wake()` brings it back),
- `Done` / throw → retire.

This finally retires the "`Waiting` spins" gap in `ThreadRunner`: `Waiting` means park, not sleep,
not re-run.

**The lost-wakeup race.** Between a component observing its inbox empty (deciding `Waiting`) and the
scheduler parking it, a `push` can land. If that wakeup is processed while the scheduler still
thinks the record is "running, about to park," it is lost — data sits in the inbox, the component
parked forever. Close it with a small per-record state machine under one lock plus a "woken while
running" flag:

```cpp
enum class Sched { Idle, Queued, Running };   // guarded by mSchedMutex

void Runner::wake() {                          // called by push()
    std::scoped_lock lock{mSchedMutex};
    if (mState == Sched::Idle)         { mState = Sched::Queued; executor.enqueue(this); }
    else if (mState == Sched::Running) { mResubmit = true; }   // honor after step() returns
    // Queued: already pending — nothing to do
}

// worker, running one record:
{ std::scoped_lock l{mSchedMutex}; mState = Sched::Running; mResubmit = false; }
const auto result = routine.step();            // no lock held during step
{ std::scoped_lock l{mSchedMutex};
  if      (result == State::Done /*or threw*/) { /* retire */ }
  else if (result == State::Continue)          { mState = Sched::Queued; executor.enqueue(this); }
  else /* Waiting */ {
      if (mResubmit) { mState = Sched::Queued; executor.enqueue(this); }  // push slipped in → requeue
      else             mState = Sched::Idle;                              // safe to park
  }
}
```

The invariant: `wake()` and the park decision take the *same* lock, and `push()` always calls
`wake()` after it has inserted (release the inbox mutex first, then `wake()` — no nested locks). A
push during `step()` sets `mResubmit` → requeue; a push after parking sees `Idle` → enqueue. The
`Queued` state also guarantees a record is enqueued once and run by one worker at a time, so a
component is never `step()`-ed concurrently with itself and per-component ordering is preserved.

**Caveats.**

- **Blocking routines do not belong on a bounded pool.** A driver that blocks inside `step()` ties
  up a worker for the whole block; enough of them starve the pool. Poll-shaped routines (Components)
  fit; truly-blocking drivers want their own dedicated threads (`ThreadRunner`) or an async reactor
  (epoll + eventfd) that calls `wake()` on fd readiness.
- **Fairness.** A component that always returns `Continue` goes to the back of the ready queue, so
  others still run; cap consecutive `Continue`s (or drain-until-`Waiting` with a budget) if per-step
  latency matters.

**Open forks:** run-one-step vs. drain-until-`Waiting` per pop; ready-queue primitive (CV — lets
stop fold in via `condition_variable_any` + `stop_token` — vs. counting semaphore); whether `wake`
is an injected `std::function` (decoupled) or a `Runner*` back-pointer.

### Status reporting — deferred

The previous design had a `Vitals` struct on every Routine (per-input counts, last-activity
timestamps, expected intervals, terminal flag). It's gone. Terminus is infrastructure; spending
cycles on observability the framework didn't ask for, on the publish path, is a tax the actual
robot software pays.

What the framework tracks: the Runner's three counters (running, stopped, failed). What it does
not track: per-Routine message counts, timestamps, intervals, health labels.

Applications that want richer observability wire it themselves — typically as a subscriber that
records what it cares about. This keeps the cost opt-in and out of the hot path. If experience
shows the framework should impose *some* minimum, revisit with concrete needs in hand.

### ConfigManager — internal to Port

Port owns one. ConfigManager reads the JSON files listed on the CLI, merges them with an opaque
policy (see Non-goals), and writes the merged result to `<port.dir()>/config.json`. The merged
result is exposed via `port.config()` as `const nlohmann::json&`.

This is the bootstrap stage and it's all the framework commits to today. A future live-tuning
mechanism (a capnp-backed mmap region whose values can be modified at human speed by an external
tool) is sketched in TODOs but not in the first cut.

### Recording directory layout

Port creates `<recordingsRoot>/<isoTimestamp>_<uuid>/` and populates it as the run progresses:

```
<port.dir()>/
    chronicle/             # chronicle's own files, written by Port's chronicle::Writer
    config.json            # merged config from all CLI-listed sources
    metadata.json          # cmdline + resource map (originalPath -> flatFilename)
    topics.txt             # one line per topic that has had at least one message
    <flat resource files>  # files added via port.addResource(...)
```

`metadata.json` is written when Port is destroyed (clean shutdown). Its shape:

```json
{
  "cmdline": ["myRobot", "--config", "/etc/myrobot.json", "--recordingsRoot", "/tmp/runs"],
  "resources": {
    "/etc/myrobot/urdf/robot.urdf":      "robot.urdf",
    "/etc/myrobot/calibration/imu.yaml": "imu.yaml"
  }
}
```

Resources are copied **flat** into `dir()` under their original basename. The map preserves
provenance so a playback binary can call `port.acquireResource(originalPath)` and get back the
in-recording path. Basename collisions throw at `addResource()` time — the caller must rename one
of their inputs.

### SignalCatcher

RAII type constructed with the driver stop_source. Blocks `SIGINT`/`SIGTERM`/`SIGHUP` process-wide,
spawns a `sigwait` thread that translates them into `request_stop()` on the driver source. On
destruction, joins the sigwait thread. Repeated signals escalate to a hard exit.

## Channel IDs and topic metadata

Channel ID = 64-bit hash of `(topicName, msgType)`. Same inputs produce the same ID across runs.
`msgType` is the capnp `typeId` (stable across builds and compilers).

Topic names come from the unit config — the logical sensor ID is the topic name. There is no
separate topic name registry beyond Port's internal map.

`topics.txt` is **reactive and fault-tolerant**. When Port observes the first publish on a
previously-unseen topic, it atomically appends one line to `<port.dir()>/topics.txt` containing
the topic name. The append is enthusiastic — done before the dispatch returns — so a crash
immediately after the publish leaves the topic recorded on disk.

Two things keep this off the hot path for the steady-state case: the "first publish per topic"
check is a hash-set lookup that hits the unknown branch exactly once per topic per process
lifetime, and the append is a single small write to a file opened once at Port construction.
After the first publish, the topic is in the known-set and subsequent publishes pay the lookup
only.

The file is not needed for dispatch — consistent hashing means subscribers re-derive the same
channel IDs without metadata. `topics.txt` exists for human inspection and as the playback
binary's enumeration of "what's actually in this recording."

## Data flow

```
Publisher (driver or component)
        |
        v
   Port::Dispatcher  ----->  chronicle::Writer  ----->  <port.dir()>/chronicle/
        |
        |  fan-out to subscribers
        v
   +----+----+----+
   v         v    v
 sub1      sub2  ...
```

Live mode: hardware drivers publish; Port routes to subscribers *and* hands the message to the
internal `chronicle::Writer`.

Playback mode: a `LogReader` driver is the publisher; subscribers see the same messages they
would have seen live. The playback-mode Port has no chronicle writer.

Chronicle on the publish path makes the future zero-copy upgrade tractable: `Port::allocator()`
can return a handle that hands out chronicle-backed writable memory, drivers build capnp messages
directly into it, and the same memory becomes the read-only view subscribers receive.

## What costs are paid where

Terminus is infrastructure. Every cycle it consumes is a cycle the actual robot software does not
get. The publish/dispatch path runs at sensor rate — potentially kHz for IMUs — and is kept
straight-line by construction.

**Per publish (hot path):**

- One channel-id lookup.
- One pass over that channel's subscriber list.
- One virtual call per subscriber.
- One enqueue into `chronicle::Writer`'s lock-free queue (live mode only).
- The "first time this topic has been seen" check (hash-set lookup, hits the slow branch exactly
  once per topic per process lifetime).

Notably *not* on the hot path: `weak_ptr.lock()`, reference-count manipulation, counter updates,
timestamp reads, health-flag toggles, exception machinery.

**Subscribe / unsubscribe (cold path):** Pays for the safety the hot path skips. `Subscription`
destruction blocks until any in-flight dispatch on that subscriber completes. This happens once
per subscriber per lifetime, so it can afford to be careful.

**Iteration boundary:** Between Routine iterations, the Runner checks the stop_token and the
previous return status. Per-work-item, not per-message — amortizes over whatever the iteration did.

**Application diagnostics:** Message counts, last-seen times, health labels, dashboards — the
application's call. Wire a separate subscriber on the channels you care about; the framework does
not impose it.

## Application lifecycle

1. Parse CLI with boost program_options.
2. Create `std::stop_source`s — typically one for drivers, one for components.
3. `SignalCatcher{driverStop}` — block signals; sigwait thread translates them to `request_stop`.
4. Construct `Port` (live or playback):
   - Live: creates `<recordingsRoot>/<isoTimestamp>_<uuid>/`, loads and merges configs,
     opens chronicle writer at `<dir()>/chronicle/`.
   - Playback: opens existing recording read-only.
5. Live only: `port.addResource(...)` for any extra files the application wants logged alongside
   the recording.
6. Construct components — held as `shared_ptr`. Their constructors take `port.config().at("name")`
   slices, acquire `port.publisher<T>(topic)` handles, and call `port.subscribe<T>(...)` to bind
   inputs. Launch on the component Runner.
7. Construct drivers — held as `shared_ptr`. Acquire publishers from Port. Launch on the driver
   Runner.
8. **Kick off.** Drivers begin producing data the moment they're launched. Main thread blocks on
   `driverRunner.waitUntilAllStopped()`.
9. Drivers return `State::Done` when:
   - SIGINT/SIGTERM fires (SignalCatcher requests stop; driver's next iteration returns `Done`).
   - end of log (LogReader's read returns nothing more).
   - configured time limit elapses (driver checks its deadline at iteration boundary).
   - upstream error (driver throws; Runner marks Failed).
10. Teardown — `componentStop.request_stop()`, `componentRunner.join()`. Components drain in
    `step()`. Port's destructor flushes chronicle and writes `metadata.json`.
11. `exitStatus(signals, runners...)` returns 0 on clean exit, `128+signum` on signal-triggered,
    non-zero if any runner reports `failedCount() > 0`.

## Shutdown semantics

- `std::stop_source` and `std::stop_token` are used directly. No framework wrappers.
- Components decide their own drain policy inside `step()`: drain queue, abandon queue,
  timeout-bounded drain. Because the loop is in Runner, "drain" just means "keep returning
  `Continue` until the queue is empty, then return `Done`."
- **Phased shutdown via two stop sources.** Drivers stop first so downstream sees a clean
  end-of-stream; components stop next and drain.
- Signal handling: `SIGINT`/`SIGTERM`/`SIGHUP` blocked process-wide; a dedicated `sigwait` thread
  translates them into `request_stop` on the driver source. Repeated signals escalate to a hard
  exit.
- Runners catch exceptions from `run()`, log to the console, and increment `failedCount()`.

## Module layout

Everything is in one module:

```
modules/terminus/
    include/nioc/terminus/
        port.hpp            ← Port, Publisher, Subscription
        topicRegistry.hpp   ← (Port internal; header for tests)
        dispatcher.hpp      ← (Port internal; header for tests)
        configManager.hpp   ← (Port internal; header for tests)
        routine.hpp         ← Routine (the base unit of work)
        component.hpp       ← Component (subscribing Routine base)
        driver.hpp          ← Driver (publishing Routine base)
        runner.hpp          ← Runner, ThreadRunner, ThreadPoolRunner
        signalCatcher.hpp
        logReader.hpp       ← Driver impl; included in user main()s for playback
        msg.hpp             ← (existing) typed capnp message wrapper
        msgBase.hpp         ← (existing)
    src/
        ...
    test/
        ...
```

The chronicle writer, the config manager, and the resource bookkeeping have no separate public
header — they live inside Port. The internal headers (`topicRegistry.hpp`, `dispatcher.hpp`,
`configManager.hpp`) are exposed only because tests want to construct them in isolation.

Namespace: `nioc::terminus`. No sub-namespaces unless they earn their keep.

## Design choices and reasoning

### Why the chronicle writer is inside Port but `LogReader` is a Driver

The chronicle writer is the recording-side endpoint that every published message must reach in
order: it's effectively a process-level singleton that touches every publish. Treating it as a
Component would mean every Component-style subscriber-of-everything has to be reattached for each
new channel, and the future allocator-for-zero-copy interface would have to be discovered from the
outside. Keeping the writer inside Port lets it sit naturally on the publish path and lets
`Port::allocator()` be a single method when zero-copy lands.

`LogReader` has no such constraints. It's a producer of messages — exactly what a hardware Driver
is. Modeling it as a normal `Driver` means playback main()s differ from live main()s only in which
driver they construct, and re-log mode falls out for free.

### Why the iteration loop lives in Runner

If every Routine wrote its own `while(!stop_requested()) { ... }` we'd duplicate the same boilerplate
in every component, every driver. Worse, exception handling and stop-token plumbing would end up
in the implementor's hands — easy to get wrong, hard to audit. By making `step()` one iteration and
giving Runner the loop, components and drivers say only what they do per step. The framework
handles "do this until told to stop" and exception capture in one place. A Routine that wants
natural completion returns `State::Done`; one that wants to keep going returns `State::Continue` (or
`State::Waiting` when it has nothing to do right now); one that wants to fail throws.

### Why `Subscription` RAII instead of `weak_ptr` dispatch

An earlier sketch had Port store `weak_ptr` per subscription and `lock()` on every dispatch. Safe,
but every published message would pay an atomic operation on the control block — for an IMU at
1 kHz with five subscribers, that's 5000 atomics/second of pure overhead, scaling linearly with
fan-out. Terminus is infrastructure: that budget belongs to the components.

`Subscription` RAII moves the cost to teardown. Components store their `Subscription`s as members;
the destructor unsubscribes (synchronizing with in-flight dispatches) once per lifetime, never on
the hot path. Forgetting to keep the token is a compile-time `[[nodiscard]]` warning, not a
runtime hazard. The invariant still lives in the type — just expressed via destruction order
instead of weak references.

### Why `main()` owns the wiring

The barrier to entry for a new component author should be: read `main`, see the data flow, add
your component. Hiding wiring inside an `App` class makes the framework a thing to learn before you
can be productive. An explicit `main` lets tools, devices, and pipelines all be read top to bottom
from one file. Length is fine; opacity is not.

### Why `std::stop_token` directly

C++20 already provides cooperative cancellation. A custom controller would duplicate it, add
another concept to learn, and complicate composition. Using `stop_source` / `stop_token` / `jthread`
directly means less code we maintain, less onboarding, and interoperability with anything else
following the standard.

### Why no per-Routine status struct

A previous version had a `Vitals` struct on every Routine — per-input message counts, last-seen
timestamps, expected intervals, a tri-state terminal flag. None of it is on the framework's
critical path, and all of it costs cycles per message to maintain. Worse, a state enum or health
flag invites an implicit state machine whose transition rules drift as new states get added, and
there's no honest single state for "IMU fine, camera silent."

The framework now tracks the minimum it actually needs: running / stopped / failed counts at the
Runner level. Applications that want detail wire it as an explicit subscriber. If experience shows
the framework should carry more, that decision will be made against real data rather than
preemptively.

### Why `Driver` is a plain base, not `Driver<Wire>`

An earlier sketch templated a driver on a single wire type plus a `wire→msg` conversion lambda. But
a real source carries *several* message types: a ROS bag has Imu on one topic and PointCloud2 on
another; a multi-sensor device is the same. A one-wire template forces either one driver per type
(and an extra layer to multiplex a single bag) or a fight against the template. So `Driver` is a
plain `Routine` base; a concrete driver holds the typed `Publisher`s it needs and does the
conversion inside `step()`, routing each input to the right publisher. The driver still knows only
its source's wire format, not the application's wider schema set, and the conversion stays the
natural seam for the future zero-copy path (a publisher handing out a writable, recording-backed
region from `Port::allocator()`).

### Why config is plain JSON for now

The merge step is the diagnostic-heavy part of config — readable format, merge tracing, useful
error messages. nlohmann/json gives us all of that for free, and the frozen `config.json` in the
recording is grep-friendly. Live tuning (mutating values at run time from a remote tool) would
want a different mechanism — flat layout, mmap, snapshot semantics — but we don't have a concrete
need yet, and bolting it in preemptively forces every read site to think about double-buffering.
Bootstrap first; live tuning is on the TODO list.

### Why the Runner abstraction

Different applications have different execution profiles. A robot binary with five drivers and four
pipelines wants dedicated threads. An analysis tool processing a recording with two hundred
parallel components wants a thread pool. A high-fan-out IO-bound tool may eventually want fibers.
The Runner abstraction lets the application choose without the components or drivers caring.

## Non-goals (for now)

- **GUI.** No GUI components in the initial design. A GUI surface is a future module.
- **Config merge policy design** — opaque; separate problem.
- **Schema storage in recordings** — desirable but not critical yet.
- **Chronicle modifications** — chronicle is unchanged for now; all new complexity lives in
  Terminus.
- **Component restart policies** — components die and stay dead; restart is a future enhancement.

## Coordinated change: `chronicle::Writer` constructor

`chronicle::Writer` currently takes a *parent* directory and creates `<iso8601>_<uuid>/` inside it
automatically. Terminus needs chronicle to write directly into the exact path Port supplies. The
constructor changes from:

```cpp
explicit Writer(std::filesystem::path logRoot = ...);   // auto-creates <logRoot>/<iso_uuid>/
```

to:

```cpp
explicit Writer(std::filesystem::path chronicleDir, ...);  // writes directly into chronicleDir
```

Contract: the directory must exist and must be empty. Throws otherwise. Terminus enforces both by
being the only thing that creates the path.

Blast radius: chronicle is used by terminus (Port internally) and chronicle's own tests. No
external callers in this tree.

## TODOs and open questions

**Publisher type design.** A bare lambda may not be enough. Open dimensions: ownership of the
message (`std::unique_ptr<Msg<...>>` promoted to `std::shared_ptr<const ...>` for fan-out, vs.
ownership-transfer-into-Port, vs. by-reference with caller-managed lifetime); namespaced outputs so
a Component publishes under its own logical prefix; whether Components capture `Port*` directly
instead of typed handles. **Iterate when a real component is being written.**

**`acquireResource` and relative paths.** The current sketch keys resources by their absolute
original path. That's fragile across machines and across users (`/home/alice/...` vs.
`/home/bob/...`). The right answer probably involves a base path (e.g. `$NIOC_RESOURCES`) and
storing the relative path inside the manifest, so the same recording is portable. Defer until the
shape of resource discovery in real apps is clearer. Until then, `acquireResource(originalPath)`
takes whatever absolute path the app passed to `addResource()` and looks it up in the map.

**Playback semantics in detail.** `Port::playback(dir)` is sketched but not specified: how does
it interact with `addResource` (forbidden? no-op?), how does it expose `topics.txt` (read-only
list?), and what is `port.publisher<T>(topic)` in playback mode (it has to exist so `LogReader`
can push, but the chronicle writer is inactive). Nail this down when we start on the playback
binary.

**Re-log mode.** Sketched as "LogReader + live Port writing a fresh recording." Open: does the
new recording carry the resource map from the old recording, or does the app re-declare? Does
`acquireResource` see the union? Defer.

**Zero-copy via `Port::allocator()`.** Concrete shape: a handle that hands out a writable region
sized for `Msg<Schema>`, backed by chronicle's mmap. Driver writes capnp into it; commit (or
handle destruction) promotes the region to const; the same memory is what subscribers see. Out of
scope for the first cut but the public API of Port should leave room for it — `Port::allocator()`
shouldn't be a breaking addition.

**Live-tuning config (capnp-backed mmap).** Earlier sketch had a double-buffered capnp mmap region
alongside `config.json` for run-time tuning by a GUI/remote tool. Cut for now; revisit once a real
need shows up.

**Status monitoring from main.** Today `main` waits with `waitUntilAllStopped()`. Periodic
inspection of `runner.runningCount()` / `failedCount()` would let `main` print a status line or
abort early on failure. Design later; the framework deliberately doesn't carry per-Routine
detail, so anything richer than counts is an application-side subscriber.

**Failure policy patterns.** Independent / fail-fast / dependency-aware cascade — pick one default
and a clean way for applications to override. Dependency graph is derivable from the subscription
topology.

**Efficient per-variable access to live config.** Capnp getter cost is roughly one bounds check +
one load per access — fine for most code, possibly not for tight inner loops. Options: raw pointer
for known-layout fields; cache locally and re-sync via a version counter; bypass capnp for hot
scalars using a fixed-layout sidecar. Benchmark on target hardware before deciding.

**Phased shutdown — two sources vs. one.** Current design commits to two stop_sources (drivers +
components) for explicit phasing. A single shared source plus careful join ordering also works.
Decide once the first real application exists.

**Termination ordering with pool runners.** When a `ThreadPoolRunner` shuts down, the wrapper
around `run()` increments `failedCount` on exception. Verify this is clean when one routine in the
pool throws while others are still running.

**Naming sweeps still open.** None pressing — `Vitals` no longer exists; `Subscription` is the
returnable RAII token.

## Setup ergonomics (next iteration)

The per-Routine block in `main()` repeats — `make_shared` → `port.subscribe(...)` calls → acquire
publishers → `runner.launch(...)`. Candidate ways to compress this while keeping the wiring
visible:

- A small `attach()` helper that takes the Runner, Port, and the Routine's
  declared subscriptions/publishers, and returns the shared_ptr it just wired and launched.
- A declarative trait on the Component class enumerating its channels, with a helper that walks it.
- A `Wiring` builder that accumulates registrations and launches at the end.

Defer until a working binary is in hand and the actual pain points are visible.
