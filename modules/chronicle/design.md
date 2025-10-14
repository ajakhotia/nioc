# Chronicle Module Design

## Purpose

Chronicle is a high-performance data recording and playback system. It records data from multiple
sources while preserving the order of arrival. During playback, this order is replicated exactly,
enabling accurate reconstruction of recorded events.

The system treats all data as opaque binary frames and does not interpret the contents.

## Storage Layout

Chronicle organizes data using channels. A channel represents a logical stream of data from a
source and is uniquely identified by a 64-bit number called a `ChannelID`. Each channel has its own
directory within the log directory, which allows for efficient playback of only a subset of channels
if
needed. Each message or event produced by a channel is called a frame. Frames are stored
sequentially
in roll files. When a roll file exceeds a certain size threshold, a new roll file is created to
store
subsequent frames.

```
Chronicle Directory
│
├── sequence                                   # Stores channel IDs in arrival order
│   ├── [channelId]
│   ├── [channelId]
│   ├── [channelId]
│   └── ...
│
├── 0x8f3a2b1c4d5e6789/                        # Channel directory
│   ├── index                                  # Maps frame # to [roll, position, size]
│   ├── roll00000000000000000000.nioc          # Data roll files
│   ├── roll00000000000000000001.nioc
│   └── ...
│
├── 0x1e4d3c2b1a098765/                        # Another channel directory
│   ├── index
│   ├── roll00000000000000000000.nioc
│   └── ...
│
└── 0x9a7b6c5d4e3f2a1b/                        # Additional channels...
    └── ...
```

## Basic Usage

### Recording Data

```cpp
// Create a writer
nioc::chronicle::Writer writer("/path/to/log/directory");

// Write data to channels as it arrives
writer.write(cameraChannelId, imageBytes);
writer.write(imuChannelId, imuBytes);
writer.write(cameraChannelId, nextImageBytes);

// Writer handles ordering and storage automatically
```

### Playing Back Data

```cpp
// Create a reader
nioc::chronicle::Reader reader("/path/to/log/directory");

// Read entries in recorded order
while(true)
{
  try
  {
    auto entry = reader.read();
    // Process entry.mChannelId and entry.mMemoryCrate.span()
  }
  catch(const std::runtime_error&)
  {
    break; // End of chronicle
  }
}
```

The reader delivers frames in the exact order they were written, interleaving channels as they
appeared during recording.

## Architecture

### Layered Design

Chronicle uses a three-layer architecture:

1. **Application Layer** (Writer/Reader): High-level API for recording and playback
2. **Channel Layer** (ChannelWriter/ChannelReader): Per-channel storage operations
3. **Storage Layer** (I/O Mechanisms): Pluggable backend implementations

This separation allows the API to remain stable while storage strategies evolve.

### I/O Mechanisms

Chronicle supports multiple I/O mechanisms through a plugin architecture. Each mechanism represents
a different strategy for reading and writing data to storage.

**Design principles:**

- Mechanisms are pluggable and independent
- Different mechanisms can be optimal for different use cases
- The choice of mechanism is made at Writer/Reader construction
- Mechanisms can be optimized separately without API changes

### Ordering Guarantee

Chronicle maintains strict ordering of all recorded frames:

- Frames are recorded in the order they arrive at the Writer
- Playback delivers frames in the same order
- This ordering is independent of which channel produced the frame

This guarantee enables accurate replay of system behavior, even when multiple channels produce
frames concurrently.

## Design Principles

### Thread Safety

The Writer is thread-safe:

- Multiple threads may call `write()` simultaneously
- Frames are recorded atomically
- The global ordering reflects the actual arrival sequence at the Writer

### Simplicity

The API is minimal and focused. Users interact primarily with Writer and Reader, which handle
complexity internally.

### Flexibility

The architecture supports future evolution:

- New I/O mechanisms can be added without API changes
- Storage format is independent of the programming interface
- Channel structure can be optimized independently

These features can be added without changing the core abstractions or fundamental architecture.
