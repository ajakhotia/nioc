# Terminus Module — PortIo Design

PortIo is the central data hub for a robotic platform. It routes data between drivers, software
pipelines, and persistent storage. It provides a uniform interface for both live operation and log
playback — downstream components are mode-agnostic.

## Modes of Operation

### Live Mode

1. Load configs (unit, application, calibration, URDFs) via an opaque merge policy.
2. Create a recording directory and write loaded configs into it.
3. Initialize chronicle for data logging.
4. Drivers and pipelines come online, push and receive data through PortIo.

### Playback Mode

1. Load configs from the recording directory.
2. Merge configs from the source/install tree using an opaque merge policy (forwards compatibility —
   new parameters get defaults, recorded parameters are preserved).
3. Initialize chronicle for reading.
4. Read data from chronicle and dispatch to subscribers.
5. Default playback speed is as-fast-as-possible. Real-time replay is an option.

## Recording Directory Layout

```
recording/
    chronicle/          # Chronicle's own data files
    metadata/
        topics.txt      # Topic table (topicName, msgType, channelId)
    config/             # Application and unit configs
    calibration/        # Sensor calibration files
    urdf/               # Robot descriptions
```

## Core Concepts

**Channel ID** — A 64-bit hash of `(topicName, msgType)`. The hash is consistent: same inputs always
produce the same ID. Collisions are detected at registration time and treated as fatal errors.

**Topic name** — Derived from the unit config, which maps sensor serial numbers to logical IDs (frame
IDs). The logical ID doubles as the topic name. No separate topic name registry is needed.

**Topic metadata** — The `topics.txt` file maps `(topicName, msgType) → channelId`. It exists for
human inspection and for `availableTopics()` queries. It is not required for dispatch — consistent
hashing makes dispatch automatic.

## Component Decomposition

The original PortIo design combined too many responsibilities in a single object. The system is
decomposed into five focused components:

```
┌──────────────────┐
│  Topic Registry   │  Maps (topicName, msgType) → channelId
├──────────────────┤     Detects hash collisions
│                  │     Writes/reads topics.txt metadata
│                  │     Serves availableTopics() queries
└────────┬─────────┘
         │ channelIds
         ▼
┌──────────────────┐          ┌──────────────────┐
│    Dispatcher     │◀─────── │   Data Logger     │  Live mode only
├──────────────────┤  publish ├──────────────────┤
│ Thread-safe       │         │ Owns chronicle    │
│   pub/sub         │         │   writer          │
│ RAII subscription │         │ Lock-free enqueue │
│   tokens          │         │ Dedicated write   │
│ Dispatch on       │         │   thread          │
│   publisher thread│         └──────────────────┘
└──────────────────┘
         ▲
         │ feeds
┌────────┴─────────┐          ┌──────────────────┐
│   Data Player     │         │  Config Manager   │
├──────────────────┤         ├──────────────────┤
│ Owns chronicle   │         │ Loads/merges      │
│   reader         │         │   configs         │
│ Playback speed   │         │ Manages recording │
│   control        │         │   directory layout│
│ Playback mode    │         │ Runs at startup   │
│   only           │         │   in both modes   │
└──────────────────┘         └──────────────────┘
```

### Topic Registry

- Maps `(topicName, msgType)` → `channelId` via consistent 64-bit hashing.
- Detects collisions at registration time (throws).
- Writes `topics.txt` eagerly at registration (startup), not on the hot path.
- Serves `availableTopics()` queries.
- During playback, subscribers re-derive the same channelId. Chronicle does not need metadata to
  dispatch.

### Dispatcher

- Thread-safe publish/subscribe on channelIds.
- `subscribe()` returns an RAII token. Destroying the token removes the subscription, preventing
  dispatch to dead components.
- Dispatch happens on the publisher's thread. Subscribers must be fast and non-blocking.
- Pure in-memory — no I/O, no buffer management. Just routes read-only views to callbacks.

### Data Logger (live mode only)

- Sits between drivers and the dispatcher.
- Enqueues data (lock-free) to a dedicated logging thread that writes to chronicle. Disk I/O never
  stalls the publisher.
- Owns the chronicle writer.

### Data Player (playback mode only)

- Owns the chronicle reader.
- Reads frames and feeds them into the dispatcher.
- Controls playback speed (as-fast-as-possible or real-time).

### Config Manager

- Loads and merges configs via the opaque merge policy.
- In live mode: writes configs into the recording directory.
- In playback mode: reads configs from the recording, merges with source tree.
- Manages the recording directory layout.
- Runs at startup in both modes, then steps aside.

### Composition

The Logger and Player are mutually exclusive — only one is active based on mode. The Topic Registry
and Dispatcher are always present. The Config Manager runs once at startup in either mode.

PortIo itself becomes a thin facade that composes these pieces and exposes the user-facing
`publish()` / `subscribe()` API. The wiring layer only sees PortIo.

## Component Coupling — Lambda-Based Wiring

Components (drivers, pipelines) do not know about PortIo. Coupling is minimized through lambdas and
an external wiring layer.

Drivers receive a push callback at construction:

```cpp
auto imuDriver = ImuDriver(unitConfig, /*onData=*/[&](auto& msg) {
    portIo.publish(channelId, msg);
});
```

Consumers provide a receive callback via the wiring layer:

```cpp
auto token = portIo.subscribe(channelId, [&](auto& msg) {
    fusionPipeline.onImu(msg);
});
```

Multi-output drivers invoke the callback per output. Multi-input consumers register multiple
subscriptions. Sync policies and connector abstractions live outside PortIo.

Only the wiring layer (e.g. `main` or an app builder) imports PortIo. Drivers and pipelines are
decoupled from it entirely.

## Data Flow Direction

A critical architectural decision: chronicle is on the write path, not a side-channel.

```
Driver ──▶ Chronicle ──▶ Dispatcher ──▶ Subscribers
```

Data goes to chronicle **first**, then to subscribers. Even before the zero-copy write path is
implemented (see below), the ordering must be: driver → chronicle → subscribers. This ensures the
data flow direction is correct from day one. The zero-copy upgrade then becomes an optimization
within chronicle's boundary without touching the dispatcher or drivers.

This also means the logger and player are not fundamentally different — both feed the dispatcher from
chronicle. In live mode, drivers write into chronicle and subscribers tail it. In playback mode,
nobody writes — subscribers read from the start. Same path.

## Future Direction: Zero-Copy from Wire to Consumer

Today a driver serializes data into its own buffer, chronicle copies it into a roll file, and the
dispatcher copies it again to consumers. For large payloads (images, point clouds) this is wasteful.

The target architecture:

1. Driver requests a writable memory region. Behind the scenes, this region is backed by chronicle's
   memory-mapped roll file — but the driver does not know that.
2. Driver builds a Cap'n Proto message directly into that region. Wire data goes straight to
   disk-backed memory.
3. The region is promoted to `const`. Cap'n Proto messages are inherently views over a byte segment,
   so the message becomes read-only automatically. No special commit protocol is needed.
4. Subscribers receive this read-only view. Zero copies from wire to consumer.

### Dependency Constraint

Drivers must not depend on chronicle or PortIo. They depend only on capnp and message schemas. The
memory allocation interface ("give me N writable bytes") must live in a lower-level module so that
chronicle implements it without drivers ever importing chronicle. The wiring layer connects the two
at runtime via dependency injection.

## Non-Goals (for now)

- **Config merge policy design** — handled opaquely, separate design problem.
- **Schema storage in recordings** — desirable but not critical yet.
- **Chronicle modifications** — chronicle is unchanged for now. All new complexity lives in PortIo.

## Metadata Fault Tolerance

- The topic table is written eagerly at registration time (startup), not on the hot path.
- Append-friendly format — each registration appends one entry. Atomic for small writes.
- On crash, the metadata file contains all topics registered up to that point.
- Metadata writes never touch the critical data path.
