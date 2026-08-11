# can-bus-fault-injector

A three-node CAN bus built on a breadboard for around $50, plus a real SocketCAN
monitoring interface — built specifically to induce and diagnose hardware faults
that don't show up in software logs.

Companion to the write-up: *Simulating Real Robot Failures on a Breadboard*
`https://imjbassi.medium.com/simulating-real-robot-failures-a3c62fbfc62d`

---

## What this is

Three Arduino Nanos, each with an MCP2515 CAN controller, sharing a two-wire CAN
bus. A CANable 2.0-based USB adapter taps the same bus and presents it to Linux
as `can0`, so the whole thing is diagnosable with standard tooling:

```bash
sudo ip link set can0 up type can bitrate 125000
candump can0
ip -details -statistics link show can0
```

Each node transmits a one-byte heartbeat on its own CAN ID once per second.
Node 1 = `0x036`, Node 2 = `0x037`, Node 3 = `0x038`.

The firmware is deliberately minimal. The point of the project is the fault
catalog, not the nodes.

---

## Bill of materials

| Part | Qty | Approx. cost |
|---|---|---|
| Arduino Nano clone (ATmega328P, CH340) | 3 | ~$14 |
| MCP2515 CAN module (HW-184, with TJA1050 transceiver) | 3 | ~$9 |
| DSD TECH SH-C31A (CANable 2.0 / `gs_usb`) | 1 | ~$18 |
| Breadboard, jumper wires, 120Ω resistors | — | ~$12 |
| **Total** | | **~$53** |

---

## Repo contents

```
firmware/
  loopback_test/     Verifies one Nano + MCP2515 in isolation, before any bus wiring
  node/              The heartbeat node sketch (set NODE_ID per board)
docs/
  wiring.md          Pin tables, bus topology, termination
  fault-catalog.md   Every fault: method, capture, bus state, analysis
  img/               Photos
```

---

## Quick start

1. **Verify one node in isolation** — flash `firmware/loopback_test` to a single
   Nano wired to its MCP2515. It should print `Loopback OK` once per second with
   no bus wiring at all. Do this for each board before connecting anything
   together; it isolates SPI wiring problems from bus problems.

2. **Wire the bus** — see [`docs/wiring.md`](docs/wiring.md). H to H, L to L,
   120Ω termination at the two physical ends only.

3. **Flash the nodes** — `firmware/node`, changing `NODE_ID` for each board.

4. **Bring up the interface** (Linux, or WSL2 with `usbipd-win` passthrough):
   ```bash
   sudo ip link set can0 up type can bitrate 125000
   candump can0
   ```
   All three IDs should appear, cycling once per second.

5. **Break it** — work through [`docs/fault-catalog.md`](docs/fault-catalog.md).

---

## Running this on Windows

Windows has no native SocketCAN. WSL2 works, with the adapter passed through
using [`usbipd-win`](https://github.com/dorssel/usbipd-win):

```powershell
usbipd list                          # find the canable2 gs_usb entry
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>  # requires a WSL2 session already running
```

Then inside WSL:

```bash
ip link show          # can0 should appear
dmesg | grep -i can   # confirms gs_usb bound
```

The default WSL2 kernel had `CONFIG_CAN` compiled in and needed no custom build.

**Gotcha:** the SH-C31A has a boot-mode switch immediately beside the
termination switch. Bumping it puts the device into DFU mode, where it
enumerates as `0483:df11` instead of `1d50:606f` and no CAN interface appears.
See the fault catalog for the full write-up — it's a useful failure to have
seen.

---

## Faults documented

| # | Fault | Discovered |
|---|---|---|
| 1 | Device in DFU mode (present but not functional) | Accidentally |
| 2 | Degraded bus wire | Accidentally |
| 3 | Node disconnect | Deliberately |
| 4 | Duplicate CAN ID | Deliberately |
| 5 | Missing and wrong-value termination | Deliberately |

Each entry in the catalog includes the method, raw `candump` capture, the
`ip -details -statistics link show can0` state before and after, and analysis of
what distinguishes it from the others.

Fault 5 produced a **null result** on a bus this short. That's documented as-is.
