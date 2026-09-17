# Wiring

## Per-node: Nano to MCP2515 (SPI)

Six wires per node. Identical for all three.

| MCP2515 pin | Arduino Nano pin |
|---|---|
| VCC | 5V |
| GND | GND |
| CS | D10 |
| SO | D12 (MISO) |
| SI | D11 (MOSI) |
| SCK | D13 |
| INT | *(unused — leave disconnected)* |

**Easy mistake:** SO and SI are simple to swap. SO ("serial out," data leaving
the MCP2515) goes to D12. SI ("serial in," data entering the MCP2515) goes to
D11. Swapping them makes the loopback test print `Loopback FAIL` (or nothing at
all) rather than point at the wiring directly.

`INT` is an interrupt line that lets the MCP2515 signal the Nano when a frame
arrives, instead of the Nano polling. Not used here; the sketches poll.

---

## The bus itself

CAN-H and CAN-L are on the **blue screw terminal** (`J3`) of each MCP2515
module, not on the yellow SPI header. They're labeled `H` and `L` on the
silkscreen.

All H terminals wire together. All L terminals wire together. That shared pair
is the entire bus.

```
[Node 1]────[Node 2]────[Node 3]────[SH-C31A]
   H ─────────── H ─────────── H ─────────── CAN_H
   L ─────────── L ─────────── L ─────────── CAN_L
  120Ω                                       120Ω
```

Order is electrical, not physical — any device can tap in anywhere along the
pair. What matters is which two devices sit at the **ends**, because only those
get termination.

---

## Termination

CAN needs 120Ω across CAN-H/CAN-L at each of the two physical ends of the bus,
and nowhere else. Without it, signals reflect off the open ends. With too much
of it, the effective resistance drops below spec.

- **MCP2515 modules:** jumper `J1`, immediately beside the `J3` screw terminal.
  Cap on = termination enabled.
- **SH-C31A:** the `R120` slide switch.

With four devices on the bus, **two** should have termination enabled and
**two** should not. A common mistake when adding a third node is leaving
termination on all of them — three or four resistors in parallel instead of two.

⚠️ On the SH-C31A, the **boot-mode switch sits directly beside the R120
switch**. Bumping it puts the device into DFU mode. See fault 1 in the catalog.

---

## SH-C31A connector

Three-pin detachable spring-clip terminal. Left to right:

```
CAN_L  |  GND  |  CAN_H
```

The block pulls off the adapter, which makes wiring it easier — do it detached,
then plug it back in.

GND is optional on a short bench setup where every device already shares a
ground through the host's USB ports. Wire it if the bus gets longer or noisier.

To insert a wire: push the orange lever open, insert, release. It clamps
automatically. A male jumper pin's tip works without stripping anything, though
it grips less firmly than stranded wire — tug-test it.

---

## Injecting a wrong-value termination

For the "termination present but wrong" variant of fault 5, disable the built-in
termination at one end and wire an incorrect resistor across H and L externally.

Using a breadboard, with H on one row and L on another:

- **60Ω** — two 120Ω resistors in *parallel*: both bridging the H row to the L row
- **240Ω** — two 120Ω resistors in *series*: H row → middle row, middle row → L row

60Ω (over-termination) is the better first test; it degrades signal integrity
more than under-termination does.
