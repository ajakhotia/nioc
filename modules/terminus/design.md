So # Terminus Module — PortIo Design

## Overview

PortIo is the central data hub for a robotic platform. It manages data routing between
drivers, software pipelines, and persistent storage. It provides a uniform interface for
both live operation and log playback — downstream components are mode-agnostic.

## Modes of Operation

### Live Mode

1. PortIo loads configs (unit, application, calibration, URDFs) via an opaque merge policy.
2. Creates a recording directory and writes loaded configs into it.
3. Initializes a chronicle writer for data logging.
4. Drivers and pipelines come online, push and receive data through PortIo.

### Playback Mode

1. PortIo loads configs from the recording directory.
2. Merges configs from source/install tree using an opaque merge policy (forwards
   compatibility — new parameters get defaults, recorded parameters are preserved).
3. Initializes a chronicle reader.
4. Reads data from chronicle and dispatches to subscribers.
5. Default playback speed is as-fast-as-possible. Real-time replay is an option.

## Recording Directory Layout

```
recording/
  chronicle/           # Chronicle's own data files
  metadata/
    topics.txt         # Topic table (topicName, msgType, channelId)
  config/              # Application and unit configs
  calibration/         # Sensor calibration files
  urdf/                # Robot descriptions
```

## Channel ID and Topic Mapping

- Channel IDs are 64-bit hashes of `(topicName, msgType)`.
- Hashing is consistent — the same inputs always produce the same channelId.
- Hash collisions are detected at registration time and treated as errors (throw).
- During playback, subscribers re-derive the same channelId from `(topicName, msgType)`.
  Chronicle does not need metadata to dispatch. Consistent hashing makes it automatic.
- The topic metadata file exists for human inspection and for `availableTopics()` queries.

## Topic Names from Unit Config

- Unit configs map sensor serial numbers to logical IDs (frame IDs).
- The logical ID doubles as the topic name. One mapping serves both purposes.
- No separate topic name registry is needed.

## Component Coupling — Lambda-Based Wiring

Components (drivers, pipelines) do not know about PortIo. Coupling is minimized through
lambdas and an external wiring layer.

**Drivers** receive a push callback at construction:

```cpp
auto imuDriver = ImuDriver(unitConfig, /*onData=*/[&](auto& msg) {
    portIo.publish(channelId, msg);
});
```

**Consumers** provide a receive callback via the wiring layer:

```cpp
auto token = portIo.subscribe(channelId, [&](auto& msg) {
    fusionPipeline.onImu(msg);
});
```

**Multi-output** drivers simply invoke the callback per output. **Multi-input** consumers
register multiple subscriptions. Sync policies and connector abstractions live outside
PortIo.

Only the wiring layer (e.g. main or an app builder) imports PortIo. Drivers and pipelines
are decoupled from it entirely.

## Subscription Lifecycle — RAII Tokens

`portIo.subscribe()` returns a lightweight RAII token. When the token is destroyed, the
subscription is automatically removed. This prevents dispatching to dead components.

The wiring layer owns both the component and its tokens. Tearing down a component destroys
its tokens, cleanly removing subscriptions.

## Threading Model

- Drivers push data from their own threads (or a thread pool).
- PortIo must be thread-safe for concurrent publishes.
- **Dispatch happens on the publisher's thread.** Subscribers must be fast and non-blocking.
- **Logging is decoupled from dispatch.** Data is enqueued (lock-free) to a dedicated
  logging thread that writes to chronicle. Disk I/O never stalls the publisher.

## Metadata Fault Tolerance

- The topic table is written eagerly at topic registration time (startup), not on the
  hot path.
- Append-friendly format — each registration appends one entry. Atomic for small writes.
- On crash, the metadata file contains all topics registered up to that point.
- Metadata writes never touch the critical data path.

## Non-Goals (for now)

- **Config merge policy design** — handled opaquely, separate design problem.
- **Schema storage in recordings** — desirable but not critical yet.
- **Chronicle modifications** — chronicle is unchanged. All new complexity lives in PortIo.
