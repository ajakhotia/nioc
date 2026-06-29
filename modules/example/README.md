# nioc by example — a Catan supply chain

If you've played Settlers of Catan, you already know the shape of this program. Producers make
resources; builders spend resources to make roads, settlements, and cities, and to buy development
cards. This example wires that up and runs it, so you can watch how nioc connects **producers** and
**consumers**.

## How nioc sees Catan

- A resource **producer** (hills, forest, pasture, fields, mountains) runs on its own and emits one
  kind of resource at a steady rate. In nioc a producer is a **driver**.
- A **builder** waits for the inputs it needs, then makes something new — so it is a consumer that is
  also a producer. In nioc that is a **component**.
- Producers and consumers never refer to each other. Each kind of message has a **topic** — a name
  like `grain`. A producer *publishes* to a topic; every consumer that *subscribes* to that topic
  gets a copy. nioc delivers the messages. So a producer doesn't know who uses its grain, and a
  builder doesn't know where its grain came from.

That decoupling is the whole point: you can add or remove producers and consumers of `grain` without
changing any of them.

## The supply chain

```
Producers                    Builders (consumers that also produce)
  hills      -> brick          road builder        brick, lumber                    -> road
  forest     -> lumber         settlement builder  road, brick, lumber, wool, grain -> settlement
  pasture    -> wool           city builder        settlement, ore, grain           -> city
  fields     -> grain          dev-card builder    ore, wool, grain                 -> development card
  mountains  -> ore
```

These follow the Catan building costs (with one addition: a settlement also needs two completed
roads). Inputs are shared and chained: `grain` feeds three builders; `brick`, `lumber`, `wool`, and
`ore` feed two each. And builders feed builders — roads are an input to the settlement builder, and
settlements to the city builder. Every consumer of a topic receives every message on it: that is the
bus delivering one message to many subscribers.

## Where to look

- `hills.hpp` — a producer (driver). The smallest routine: wait, then publish.
- `roadBuilder.hpp` / `.cpp` — a consumer (component): subscribe to inputs, publish an output.
- `catanMain.cpp` — how every producer and consumer is created and connected.
- `config/catanConfig.capnp` — the settings: topic names, produce times, recipe sizes.

## Settings you can change at runtime

Each routine reads its settings from config every time it acts — not once at startup — so a running
program can be retuned: how fast a producer emits (`miningTimeMs`) or how much a recipe costs
(`orePerCity`, `roadPerSettlement`, ...). Settings layer, with the last winning:
built-in defaults → `--append-config <file>` → `--config-override key=value`.

## Run

```shell
cmake --build <BUILD_DIR> --target catanMain
<INSTALL_DIR>/bin/catanMain                                  # built-in defaults
<INSTALL_DIR>/bin/catanMain --append-config <INSTALL_DIR>/config/nioc/example/strippedCatan.json
<INSTALL_DIR>/bin/catanMain --config-override fields.miningTimeMs=250
```

`config/nioc/example/defaultCatan.json` is the whole config written out (a reference you can copy and
edit); `strippedCatan.json` overrides just a couple of values to show that a config file only needs
to name what it changes.

Every finished piece prints as it is built. Press Ctrl-C to stop.
