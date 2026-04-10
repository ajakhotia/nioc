# Chronicle Module Design

Chronicle records and replays data from multiple sources. It preserves the exact order in which data
arrived so that playback faithfully reconstructs what happened during recording.

Chronicle does not interpret the data it stores. Every piece of data is an opaque binary blob called
a **frame**.

## Core Concepts

**Channel** — A logical stream of frames from a single source. Identified by a 64-bit `ChannelId`.
Example: one camera produces one channel of image frames.

**Frame** — A single unit of data within a channel. Chronicle does not know or care what is inside a
frame.

**Sequence** — The global ordering of all frames across all channels. When three sources produce
frames concurrently, the sequence records which frame arrived first, second, third, and so on.

**MemoryCrate** — A read-only container returned during playback. It holds a
`std::span<const std::byte>` pointing into a memory-mapped file. Copying a MemoryCrate is cheap — it
shares the same backing memory.

## How Recording Works

```cpp
auto writer = nioc::chronicle::Writer("/path/to/logs");

writer.write(cameraChannelId, imageBytes);
writer.write(imuChannelId, imuBytes);
writer.write(cameraChannelId, nextImageBytes);
```

Each call to `write()` does two things:

1. Appends the ChannelId to the **sequence file** — a flat binary file that records arrival order.
2. Appends the frame data to the channel's **roll file** and records its location in the channel's
   **index file**.

The Writer is thread-safe. Multiple threads may call `write()` concurrently. The sequence file
determines the global order.

### Channel Storage

Each channel gets its own directory named by its hex ChannelId (e.g. `0x8f3a2b1c/`). Inside:

- **index** — A flat binary file. Each 24-byte entry records which roll file holds the frame, at what
  byte offset, and how many bytes long it is.
- **roll files** — Named `roll00000000000000000000.nioc`, `roll...0001.nioc`, etc. Raw concatenated
  frame bytes with no internal framing. The index is the only way to find frame boundaries. When a
  roll exceeds 128 MB (configurable), a new one is created.

## How Playback Works

```cpp
auto reader = nioc::chronicle::Reader("/path/to/logs");

while (true)
{
    try
    {
        auto entry = reader.read();
        // entry.mChannelId  — which channel produced this frame
        // entry.mMemoryCrate.span() — read-only view of the frame data
    }
    catch (const std::runtime_error&)
    {
        break;  // End of chronicle
    }
}
```

The Reader memory-maps the sequence file and walks through it entry by entry. For each entry, it
looks up the corresponding channel reader, which memory-maps the index and roll files to locate and
return the frame data as a MemoryCrate.

Roll files are mapped lazily and cached in a circular buffer (capacity 5) to limit open file
descriptors. A MemoryCrate keeps its backing roll file mapped (via `shared_ptr`) for as long as it
exists.

## Storage Layout on Disk

```
chronicleDirectory/
├── sequence                                  # Global arrival order (8 bytes per entry)
│
├── 0x8f3a2b1c4d5e6789/                      # Channel directory
│   ├── index                                 # Frame locations (24 bytes per entry)
│   ├── roll00000000000000000000.nioc         # Frame data
│   ├── roll00000000000000000001.nioc
│   └── ...
│
├── 0x1e4d3c2b1a098765/                      # Another channel
│   ├── index
│   ├── roll00000000000000000000.nioc
│   └── ...
```

### Binary Format

All multi-byte integers are stored in little-endian byte order.

| File       | Entry size | Contents                           |
|------------|------------|------------------------------------|
| sequence   | 8 bytes    | ChannelId                          |
| index      | 24 bytes   | rollId (8) + offset (8) + size (8) |
| roll*.nioc | variable   | Raw frame bytes, concatenated      |

## Architecture

Chronicle is organized in three layers:

```
┌──────────────────────────────────────────┐
│  Application Layer   (Writer / Reader)   │  ← Public API
├──────────────────────────────────────────┤
│  Channel Layer       (ChannelWriter /    │  ← Per-channel operations
│                       ChannelReader)     │
├──────────────────────────────────────────┤
│  I/O Mechanisms      (Stream / Mmap)     │  ← Pluggable storage backends
└──────────────────────────────────────────┘
```

- **Application layer**: Writer and Reader. The only classes most code touches.
- **Channel layer**: Abstract interfaces (`ChannelWriter`, `ChannelReader`) that handle per-channel
  storage. Created lazily by the application layer — one per channel.
- **I/O mechanisms**: Concrete implementations. Currently `StreamChannelWriter` for recording
  (file-stream writes) and `MmapChannelReader` for playback (memory-mapped reads). New mechanisms
  can be added without changing the public API.

## Cap'n Proto Schema

Chronicle defines a generic `Frame(Msg)` wrapper that pairs any message type with an arrival
timestamp:

```capnp
struct Frame(Msg) {
    arrivalTimestamp @0 : Timestamp;
    message          @1 : Msg;
}
```

Published as the `nioc::chronicleIdl` build target. Modules that define domain-specific messages use
`Frame` to wrap them before recording.

## Dependencies

| Dependency            | Visibility | Purpose                            |
|-----------------------|------------|------------------------------------|
| Boost.Iostreams       | public     | Memory-mapped file I/O             |
| nioc::common          | public     | `Locked<T>` wrapper, `makeBimap`   |
| Boost.Endian          | private    | Little-endian binary serialization |
| Boost.CircularBuffer  | private    | LRU cache of mapped roll files     |
| Boost.UUID            | private    | Unique log directory naming        |
| nioc::commonIdl       | private    | Timestamp schema for Frame         |
| spdlog                | private    | Logging                            |

## Future Direction: Zero-Copy Write Path

Today a driver serializes a Cap'n Proto message into its own buffer, then `Writer::write()` copies
those bytes into a roll file. For large payloads (images, point clouds) this means at least two full
copies of the data.

The long-term goal is to eliminate these copies:

1. A driver requests a writable memory region. Behind the scenes, this region is backed by a
   memory-mapped roll file — but the driver does not know that.
2. The driver builds a Cap'n Proto message directly into that region. Wire data goes straight to
   disk-backed memory.
3. The region is promoted to `const`. Because Cap'n Proto messages are views over a byte segment,
   the message is now inherently read-only. No special commit or ownership transfer is needed.
4. Subscribers receive this read-only view. Zero copies from wire to consumer.

### Dependency Constraint

Drivers must not depend on chronicle. They depend on capnp and message schemas only. The memory
allocation interface (something like "give me N writable bytes") must live in a lower-level module so
that chronicle can implement it without drivers importing chronicle. The wiring layer connects the
two at runtime.
