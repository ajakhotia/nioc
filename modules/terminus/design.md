# Terminus — Design

## What Terminus is

Terminus is the data hub and application scaffolding for nioc binaries. It does three things:

1. **Routes data** between hardware drivers, software components, and the recording.
2. **Owns the recording directory** — creates it, merges config into it, copies in logged
   resources, writes `metadata.json`, and records the chronicle time-series stream.
3. **Provides lifecycle primitives** so an application has a small, readable `main()`.

Live operation and log playback are the same binary: in playback the log reader is *itself a
driver*, so downstream components are mode-agnostic. Everything lives in `modules/terminus/`,
namespace `nioc::terminus`.

## Two layers

The execution machinery and the data hub are separate modules, and the dependency points one way.

- **`nioc::concurrent`** — generic execution: `Routine` (a unit of work) and `Runner` (the loop that
  drives it). Knows nothing about `Port`, recording, or messages.
- **`nioc::terminus`** — the data hub: `Port`, and the `Driver` / `Component` bases that bind a
  routine to a Port and carry the typed message API.

```
        nioc::concurrent                         nioc::terminus
   ┌──────────────────────┐               ┌─────────────────────────────┐
   │  Routine   Runner     │  ◄─depends──  │  Port                       │
   │  (step loop, parking) │               │  Driver : Routine (produce) │
   └──────────────────────┘               │  Component : Routine (consume) │
                                          └─────────────────────────────┘
```

## Port — the hub

Port is the central object: data router and owner of the recording directory. One Port instance
owns one run's recording.

Public surface (see `port.hpp` for contracts):

- `workingDir()` — the recording directory (`<logRoot>/<iso8601>_<uuid>/`).
- `config()` — the merged JSON config (files merged left-to-right, later wins).
- `addResource(source)` / `acquireResource(source)` — copy a file flat into the recording under its
  basename, recorded in `metadata.json`; `acquireResource` returns the in-recording path.
- `subscribe(channelId, weak_ptr<const ConsignmentCallback>)` — register a callback for a channel;
  held **weakly**, so the subscriber controls its own lifetime by keeping the `shared_ptr` alive.
- `publish(channelId, msgBasePtr)` — fan out to live subscribers **synchronously on the caller's
  thread** and hand a copy to the chronicle writer.

Routines do not call `subscribe` / `publish` directly — they reach them through the typed
`Component::subscribe<Schema>` / `publish<Schema>` wrappers (see below).

### In-flight accounting

`Port` holds one atomic, `mPendingConsignments`. Each published message is wrapped in a
`Consignment` (`consignment.hpp`) whose construction increments the counter and whose destruction
decrements it. A message handed to a subscriber lives — and stays counted — inside that subscriber's
inbox until it is popped and processed; the chronicle copy is counted the same way.

So **`mPendingConsignments == 0` means every published message has been consumed by every subscriber
and drained to the chronicle** — i.e. the system is quiescent. This one counter is the basis of
clean shutdown (below); no separate inbox inspection is needed.

## Routine, Driver, Component

`Routine` (`nioc::concurrent`) is the base unit of work. The **loop lives in the Runner**:
`step()` performs one iteration and returns a `State`; the Runner calls it until it ends.

```cpp
enum class State : std::uint8_t { Continue, Waiting, Done };

State tick() noexcept;          // calls step(), stores State for cross-thread observation
const std::string& name() const noexcept;
void attachTrigger(std::function<void()> trigger);   // Runner installs its wake hook
protected:
  void triggerRunner() const;   // routine fires this when idle→work, to wake a parked Runner
private:
  virtual State step() noexcept = 0;   // one iteration; never throws (self-catches → Done)
```

- `Continue` — more work now; run again immediately.
- `Waiting` — nothing right now; park until woken (an empty inbox is the canonical case).
- `Done` — finished; stop scheduling. Failure is reported by an implementation catching its own
  exception and returning `Done` (the framework relies on `step()` being `noexcept`).

`Driver : Routine` — a **producer**. It holds `Port&`, and a subclass implements `run()` (wrapped by
a `final` `step()`) to generate and `publish<Schema>(topic, msgPtr)` the next message(s). A driver
owns no inbox. A real source carries several message types, so a driver holds whatever typed
publishing it needs and routes each input itself — `Driver` is a plain base, not `Driver<Wire>`.

`Component : Routine` — a **consumer** that may also publish. A subclass calls
`subscribe<Schema>(topic, callback)`; matching messages are queued into a bounded **inbox**
(`concurrent::BufferMode` fixes the full-inbox discipline: overwrite oldest vs. block the producer),
and each `step()` pops one entry and runs its callback on the Runner's thread, so callbacks never
run concurrently. An empty inbox returns `Waiting`.

`subscribe<Schema>` wires two hops: Port → inbox (a `ConsignmentCallback` the Component owns as a
`shared_ptr` and Port holds weakly), and inbox → handler (a type-erasing dispatcher that down-casts
to `Msg<Schema>`). One handler per channel; duplicate subscription throws. Because Port holds the
callback weakly, the Component dropping its `shared_ptr` (at destruction) ends the subscription —
no per-message lifetime check on the publish path.

## Runner

`Runner` (`nioc::concurrent`) owns the loop a Routine deliberately lacks. Each Runner drives **one**
routine, held weakly:

- `launch(weak_ptr<Routine>)` — start driving; installs a wake trigger on the routine.
- `waitUntilStopped()` — block until the loop ends.
- `requestStop()` — ask the loop to stop after the current iteration.

`ThreadedRunner` is the thread-backed implementation: one `std::jthread` per routine. It calls
`tick()` in a loop; `Continue` runs again, `Done` (or an expired routine) ends the loop, and
`Waiting` **parks** on a `condition_variable_any` using the interruptible
`wait(lock, stopToken, pred)`. That park wakes on either of two events:

- `triggerRunner()` → `wake()` sets the ready flag → the loop runs one more `step()`;
- the jthread's own `stop_token` (via `requestStop()`) → the loop exits **without** another `step()`.

This two-way wake is what clean shutdown builds on. `ThreadPoolRunner` (readiness-scheduled, shared
pool) is deferred.

## Shutdown

Shutdown is a two-stage, cooperative wind-down driven by **two `std::stop_source`s owned by Port**,
tripped through `Port::shutdown()` / `Port::halt()` and observed through `shutdownToken()` /
`haltToken()`:

- **shutdown** (`mShutdownToken`) — the normal way down, like shutting down a machine: producers
  stop, in-flight work drains.
- **halt** (`mHaltToken`) — the harsh stage: stop now, abandon whatever is queued.

### Where the tokens live, and why

The token a class carries mirrors what its role can decide on its own — a **producer has a natural
end, a consumer does not**:

```
concurrent::Routine
    mShutdownToken            ← every unit of work can be told to wind down
    shutdownRequested()
        │
        ├── terminus::Driver       (producer) — shutdown ⇒ stop producing ⇒ Done. Complete.
        │
        └── terminus::Component    (consumer) — drains until told to abandon
                mHaltToken         ← the consumer-only "stop now" switch
                haltRequested()    ← the Component's actual Done trigger
```

A Driver shutting down has an obvious meaning: stop producing → `Done`. One signal suffices, and it
never needs halt — a Driver is already winding down off `mShutdownToken` before halt ever fires. A
Component has no natural stopping point — left alone it would drain forever, because more data
*might* arrive — so it needs a second signal to stop when it is not naturally finished. Hence
`mShutdownToken` lives on the base `Routine` (a plain `std::stop_token`, no Port coupling), and
`mHaltToken` is added by `Component`.

### Quiescence

Quiescence is `mPendingConsignments == 0`. Port runs a monitor that, once **armed** (shutdown has
been requested, so producers are winding down), polls the counter; when it reaches zero, Port fires
the halt source itself. Polling cadence bounds the worst-case shutdown latency.

### The choreography

```
EVENTS                       GUARDS / DERIVED                ACTIONS
  sig#1  first signal          QUIESCENT ≜ armed             SHUTDOWN := fire shutdown source
  sig#2  second signal                   ∧ pending==0        HALT     := fire halt source
  sig#3  third signal          armed ≜ shutdown requested    FORCE    := fire BOTH
  drvDone last driver Done

                          ┌───────────────────────────────────────────┐
                          │                 RUNNING                    │
                          │  invariants:                               │
                          │   • drivers produce · components drain     │
                          │   • pending ≥ 0 (free to rise & fall)      │
                          │   • neither source fired                   │
                          └───────────────────────────────────────────┘
                             │                                    │
       sig#1 → SHUTDOWN      │            drvDone                 │  sig#2 | crit → FORCE
   (or drvDone converges)    │          (no action)               │  ───────────────────┐
                             ▼                                    ▼                       │
                          ┌───────────────────────────────────────────┐                  │
                          │                WINDDOWN                    │                  │
                          │  invariants:                               │                  │
                          │   • shutdown fired ⇒ drivers → Done        │                  │
                          │   • no new external data enters            │                  │
                          │   • components keep draining (not Done)    │                  │
                          │   • pending monotonically → 0              │                  │
                          │   • Port monitor polls QUIESCENT           │                  │
                          │   • never returns to RUNNING               │                  │
                          └───────────────────────────────────────────┘
                             │                                    │
      QUIESCENT → HALT       │                                    │  sig#2 | crit → FORCE
   (Port fires halt source)  │                                    │  ───────────────────┤
                             ▼                                    ▼                       ▼
                          ┌──────────────────────────────────────────────────────────────────┐
                          │                            HALTING                                 │
                          │  invariants:                                                       │
                          │   • halt fired ⇒ every routine returns Done on next wake            │
                          │   • a parked component is woken (stop_callback → triggerRunner)      │
                          │   • no routine re-enters Continue                                   │
                          │   • queues may be empty (graceful) or not (FORCE) — both OK         │
                          │   • a late publish is safe (Port alive) but undelivered (dropped)   │
                          └──────────────────────────────────────────────────────────────────┘
                             │
        all runners joined   │
                             ▼
                          ┌───────────────────────────────────────────┐
                          │                  EXITED                    │
                          │   • all runner threads joined              │
                          │   • THEN Port dtor: flush chronicle +      │
                          │     refresh metadata.json                  │
                          │   • exit: 0 clean · 128+sig · ≠0 on crit   │
                          └───────────────────────────────────────────┘

   sig#3, from ANY state → std::_Exit(128 + sig)   — hard exit, no cleanup, bypasses Port
```

Per-routine reaction:

| source   | Driver (`mShutdownToken`)        | Component (`mHaltToken`)                       |
|----------|----------------------------------|------------------------------------------------|
| shutdown | `step()` → `Done` (stop producing) | — (keeps draining; not observed this iteration) |
| halt     | (already `Done`)                 | wake → `step()` → `Done` (inbox abandoned, OK)  |
| FORCE    | `step()` → `Done`                | wake → `step()` → `Done`                        |

Two properties this pins down:

- **Both the signalled path and natural end-of-data converge on WINDDOWN**, and the only graceful
  way out is Port observing quiescence and firing halt. FORCE is the shortcut that fires both
  sources and jumps straight to HALTING.
- **`halt` is the Component's single `Done` switch** — a Component never self-terminates on "inbox
  empty"; it waits to be woken by halt. Queue state is an *observation* of which path was taken,
  never a decision input.

### Waking a parked routine

Each routine registers a `std::stop_callback` that calls `triggerRunner()` when its `Done`-bearing
token fires (`Routine` on `mShutdownToken`, `Component` on `mHaltToken`). The callback fires on
Port's request thread; `triggerRunner()` sets the ready flag under the Runner's lock, so there is no
lost-wakeup race whether the routine is parked or between iterations. The woken `step()` checks the
token first and returns `Done`. This reuses the existing wake handshake — no new mechanism, and
`requestStop()` stays the destruction backstop, not the primary path.

### Signals and escalation

Catching Ctrl+C is a separate, reusable utility — `nioc::common::SignalCatcher` — not a Port
concern. It installs a `SIGINT` handler (`std::signal`) whose only async-signal-safe work is bumping
an atomic counter and waking a dedicated thread; that thread runs a caller-supplied action with the
running signal count, in ordinary context. The counter is 4 bytes so the handler's wake is a bare
`FUTEX_WAKE` (async-signal-safe in practice).

`main` owns the catcher and wires the policy against Port's triggers: **1st** Ctrl+C →
`port.shutdown()`, **2nd** → `port.halt()`, **3rd** → `std::_Exit(130)`. Port itself has no signal
knowledge — it only owns the two sources and exposes `shutdown()` / `halt()` and their tokens.

### Why `std::stop_token` directly

C++ already provides cooperative cancellation. Using `stop_source`/`stop_token`/`jthread` directly
means less code, nothing new to learn, and interoperability with anything standard. The framework
adds no wrapper.

## Recording directory layout

```
<workingDir>/
    chronicle/       # chronicle files, written by Port's AsyncChronicleWriter (when recording)
    config.json      # merged config from all CLI-listed sources
    console.log      # the run's log output (a file sink Port attaches at construction)
    metadata.json    # { "cmdline": ..., "resources": { originalPath -> flatBasename } }
    <flat resources> # files added via addResource()
```

`metadata.json` is written at construction and refreshed in Port's destructor (resources may grow
via `addResource()`). Resources are copied flat under their basename; a basename collision throws at
`addResource()` time.

## Channel IDs

Channel ID = 64-bit hash of `(topicName, msgType)`, where `msgType` is the capnp `typeId` (stable
across builds). Same inputs → same ID across runs, so subscribers re-derive IDs without any
registry. Topic names come from the unit config.

## Design choices

- **The loop lives in Runner.** If every routine wrote its own `while(!stop) { ... }`, stop plumbing
  and exception handling would land in every implementor's hands. `step()` says only what one
  iteration does; the framework owns "repeat until told to stop" and exception capture in one place.
- **Weak-pointer dispatch, subscriber-owned lifetime.** Port holds each subscription as a
  `weak_ptr<const ConsignmentCallback>`; the Component owns the `shared_ptr`. Dropping it (at
  destruction) ends the subscription. The publish path locks the weak pointer per subscriber — the
  cost lives on the (cold) dispatch-setup and teardown sides, and lifetime stays expressed in
  ownership, not in a manual unregister call.
- **`main()` owns the wiring.** The barrier to entry for a new component should be: read `main`, see
  the data flow, add yours. No hidden `App` class. Length is fine; opacity is not.
- **`Driver` is a plain base, not `Driver<Wire>`.** A real source carries several message types on
  several topics; a one-wire template forces one driver per type or a fight against the template. A
  concrete driver holds the typed publishing it needs and converts inside `run()`.
- **`shutdown` vs `halt` naming.** `shutdown` is the normal, expected way down (orderly, drains);
  `halt` is the abrupt escalation — the words carry the distinction the way the Unix commands do.
- **Port owns the two stop sources.** Routines already take `Port&`, so Port is the natural place to
  hand each routine its token at construction and to fire halt itself once it observes quiescence.

## Deferred / open

- **Natural-completion convergence.** How Port observes "all drivers done" to arm the monitor on the
  no-signal path (e.g. drivers reporting completion to Port); the monitor then fires the halt source
  through `Port::halt()`.
- **Critical-error → FORCE.** A failing routine returns `Done` indistinguishably from natural
  completion today; the Runner records no failure disposition, so nothing yet drives FORCE on error
  or a non-zero exit code. Needs a failure signal (runner-side disposition vs. routine→Port).
- **Component aux-thread wind-down.** A Component that spawns worker threads / long compute will
  observe its inherited `mShutdownToken` to wind them down and report "settled" so quiescence
  accounts for side-channel work. Out of scope this iteration: a Component in `Waiting` at
  quiescence is taken to be safe to stop.
- **`ThreadPoolRunner`** — readiness-scheduled shared pool for many mostly-idle components.
- **Playback mode** — `Port` opening an existing recording read-only; a `LogReader` driver replays
  the chronicle through normal publishing. Not yet built.
- **`Publisher<Schema>` type** — whether typed publishing graduates from the `publish<Schema>`
  wrapper to a first-class handle (ownership/namespacing questions).
- **Zero-copy via `Port::allocator()`** — chronicle-backed writable regions a driver builds capnp
  into; the public API should leave room for it without a breaking change.
- **Live-tuning config** — a capnp-backed mmap region for run-time tuning; `config.json` is the
  bootstrap-only commitment today.

## Non-goals (for now)

GUI; config merge-policy design; schema storage in recordings; chronicle modifications; component
restart policies. Components die and stay dead.
