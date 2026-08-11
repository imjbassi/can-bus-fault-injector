# Fault Catalog

Every fault: method, raw capture, bus state, and what distinguishes it from the
others.

Baseline for all comparisons is a healthy three-node bus at 125 kbit/s.

---

## Baseline (healthy)

```
can0  036   [1]  AA
can0  037   [1]  AA
can0  038   [1]  AA
can0  036   [1]  AA
can0  037   [1]  AA
can0  038   [1]  AA
```

```
can0: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc pfifo_fast state UP mode DEFAULT group default qlen 10
    link/can
    can state ERROR-ACTIVE (berr-counter tx 0 rx 0)
          bitrate 125000 sample-point 0.875
          tq 125 prop-seg 6 phase-seg1 7 phase-seg2 2 sjw 1
          gs_usb: tseg1 6..16 tseg2 2..8 sjw 1
          clock 48000000
          re-started bus-errors arbit-lost error-warn error-pass bus-off
          0          0          0          0          0          0
```

All three IDs present. `ERROR-ACTIVE`, all counters zero.

---

## Fault 1 — Device in DFU mode

**Discovered:** accidentally, while flipping the termination switch.

**Symptom:**

```
$ candump can0
SIOCGIFINDEX: No such device
```

The interface disappeared entirely. Nothing had been unplugged.

**Diagnosis:** the device was still enumerated — but as something else.

```
BUSID  VID:PID    DEVICE                 STATE
7-2    0483:df11  DFU in FS Mode         Not shared
```

VID:PID had changed from `1d50:606f` (canable2, `gs_usb`) to `0483:df11` — an
STM32 in Device Firmware Update mode. The boot-mode switch sits directly beside
the R120 termination switch and had been bumped.

**Root cause:** boot-mode switch in the wrong position.

**Fix:** flip it back, replug to force re-enumeration, re-attach to WSL2, bring
the interface up.

**Why it's worth cataloguing:** the device was physically connected,
successfully enumerated, visible to the OS, and completely unable to do its job.
That *present-but-not-functional* shape is the same signature as a wedged
USB-serial adapter — the port exists, `lsusb` lists it, a device node sits in
`/dev`, nothing works.

A diagnostic habit that stops at "is it connected?" returns a false negative
here. The question has to be: connected **as what**, and answering?

---

## Fault 2 — Degraded bus wire

**Discovered:** accidentally, after repeatedly loosening and re-tightening screw
terminals while adding a third conductor.

**Symptom:**

```
can0  037   [1]  AA
can0  037   [1]  AA
can0  037   [1]  AA
```

Node 2 present and perfect. Node 1 absent entirely — not intermittent, not
corrupted, gone.

**Diagnosis, cheapest test first:**

| Test | Cost | Result | Eliminated |
|---|---|---|---|
| Check Node 1's serial output | Free — window already open | Still printing `Node 1: sent` on schedule | Power, firmware crash, upload failure |
| Move the monitor's tap from Node 1's terminal to Node 2's | ~1 min | No change, still only `037` | Listener-side connection |
| Replace both bus wires | ~2 min | Both IDs return | — |

The second test returned **no change**, which felt like a dead end and wasn't:
it eliminated the entire listener-side hypothesis and proved Node 1's frames
weren't reaching the bus at all, making the monitor's position irrelevant.

**Root cause:** a wire that carried signal when first installed and stopped
after being unscrewed, re-seated, and clamped alongside a second conductor
several times. Likely a fatigued strand inside the insulation, or one that
backed out enough to lose contact under the clamp.

**Fix:** replaced both bus wires.

```
can0  036   [1]  AA
can0  037   [1]  AA
can0  036   [1]  AA
can0  037   [1]  AA
```

**Why it's worth cataloguing:** the wire looked perfect. This is the signature
of strain-relief and connector-wear failure on real robots — intermittent,
correlated with handling or arm pose, invisible on inspection. It's also the
class of fault a power cycle appears to "fix," because jostling the hardware
momentarily reseats a marginal contact. Those recur weeks later and nobody
connects them to the earlier incident.

---

## Fault 3 — Node disconnect

**Method:** with all three nodes running, physically disconnect one node's CAN-H
wire while `candump` runs.

**Capture:**

```
can0  036   [1]  AA
can0  037   [1]  AA
can0  036   [1]  AA
can0  037   [1]  AA
can0  036   [1]  AA
can0  037   [1]  AA
```

**Bus state during the fault:**

```
can0: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc pfifo_fast state UP mode DEFAULT group default qlen 10
    link/can
    can state ERROR-ACTIVE (berr-counter tx 0 rx 0)
          bitrate 125000 sample-point 0.875
          tq 125 prop-seg 6 phase-seg1 7 phase-seg2 2 sjw 1
          gs_usb: tseg1 6..16 tseg2 2..8 sjw 1
          clock 48000000
          re-started bus-errors arbit-lost error-warn error-pass bus-off
          0          0          0          0          0          0
```

**Result:** the disconnected node vanished from `candump`. The bus stayed
`ERROR-ACTIVE` with zero tx/rx errors and no bus-errors. Completely clean.

**Signature:** one ID missing, bus metrics unchanged. This is a **local** fault —
that device, its connector, or its controller. Nothing is corrupting the shared
medium.

In a real fleet this is the cheapest and most useful distinction available: it
tells you whether to go look at one joint or at the whole harness.

---

## Fault 4 — Duplicate CAN ID

**Method:** flash two nodes with the same CAN ID (`0x036`) but different data
bytes, so both transmit identical arbitration fields.

**Why this differs from a collision:** CAN arbitration is resolved
electrically — a dominant bit overrides a recessive one, the loser backs off and
retries, no data lost. That works when IDs differ. With identical IDs and
different data, neither node loses arbitration; the conflict surfaces later, in
the data field.

**Capture:**

```
can0  036   [1]  AA
can0  036   [1]  AA
can0  036   [1]  55
can0  036   [1]  AA
can0  036   [1]  55
can0  036   [1]  AA
```

**Bus state during the fault:**

```
can0: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc pfifo_fast state UP mode DEFAULT group default qlen 10
    link/can
    can state ERROR-ACTIVE (berr-counter tx 18 rx 0)
          bitrate 125000 sample-point 0.875
          tq 125 prop-seg 6 phase-seg1 7 phase-seg2 2 sjw 1
          gs_usb: tseg1 6..16 tseg2 2..8 sjw 1
          clock 48000000
          re-started bus-errors arbit-lost error-warn error-pass bus-off
          0          7          0          0          0          0
```

**Result:** transmit error counter climbed to 18, bus-errors to 7. The state
remained `ERROR-ACTIVE` — the errors accumulated but never crossed the
error-passive threshold.

**The counter that *didn't* move matters:** `arbit-lost` stayed at 0. Both nodes
transmit the same identifier, so neither loses arbitration to the other. The
intuitive guess — "duplicate IDs cause arbitration loss" — is wrong, and the
counters say so directly.

**Signature vs. fault 3:** both produce "expected traffic is missing." Fault 3
leaves the bus completely clean. Fault 4 shows the controller actively detecting
transmission errors while nodes are still trying to send. Same surface symptom,
opposite counter behavior.

---

## Fault 5 — Missing and wrong-value termination

**Method:** two variants —
1. Termination removed from one physical end entirely
2. Termination replaced with an effective 60Ω (two 120Ω resistors in parallel)

**Capture — termination removed:**

```
can0  036   [1]  AA
can0  037   [1]  AA
can0  038   [1]  AA
can0  036   [1]  AA
can0  037   [1]  AA
can0  038   [1]  AA
```

**Capture — 60Ω substituted:**

```
can0  036   [1]  AA
can0  037   [1]  AA
can0  038   [1]  AA
can0  036   [1]  AA
can0  037   [1]  AA
can0  038   [1]  AA
```

**Bus state (checked after both variants; neither produced a change):**

```
can0: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc pfifo_fast state UP mode DEFAULT group default qlen 10
    link/can
    can state ERROR-ACTIVE (berr-counter tx 0 rx 0)
          bitrate 125000 sample-point 0.875
          tq 125 prop-seg 6 phase-seg1 7 phase-seg2 2 sjw 1
          gs_usb: tseg1 6..16 tseg2 2..8 sjw 1
          clock 48000000
          re-started bus-errors arbit-lost error-warn error-pass bus-off
          0          0          0          0          0          0
```

### Result: null

Neither variant caused frame loss or moved any counter. The interface stayed
`ERROR-ACTIVE` throughout, with all three nodes transmitting normally.

**This is documented as-is rather than escalated until something broke.**

The bus here is a few feet of bench wiring at 125 kbit/s. Signal reflections
that would destroy a long deployed harness are simply below the failure
threshold at this scale and bitrate. The experiment exposed a limitation of
benchtop reproduction, not a property of the protocol.

That's a useful finding in its own right: **some physical-layer faults require
the physical scale of the real system before their consequences appear.** It's
precisely why termination problems surface in deployed robots and not in
testing.

---

## Summary table

| Fault | `candump` symptom | Bus state | Counters | Distinguishing signal |
|---|---|---|---|---|
| Node disconnect | One ID missing | `ERROR-ACTIVE` | All zero | Missing traffic, bus completely clean |
| Duplicate ID | One ID, alternating data bytes | `ERROR-ACTIVE` | `berr tx 18`, `bus-errors 7`, `arbit-lost 0` | Errors accumulating while nodes still transmit |
| Bad termination | No change | `ERROR-ACTIVE` | All zero | No software-visible signal at this bus length |
| Bad wire / connector | One ID missing | `ERROR-ACTIVE` | All zero | Indistinguishable from node disconnect by counters — separated by a physical swap test |
| Device in DFU mode | Interface gone entirely | n/a | n/a | Enumerates under a different VID:PID |

### What the table actually shows

Two pairs are **not** distinguishable by counters alone:

- **Node disconnect vs. bad wire** produce identical signatures. Both are one ID
  missing on a clean bus. Separating them required a physical test — checking
  the node's serial output to confirm it was still transmitting, then moving the
  monitor's tap point.
- **Bad termination vs. healthy** are identical at this scale.

So the counters aren't a fault classifier. They narrow the search space, and
they're decisive for fault 4, but the resolution comes from combining them with
cheap physical tests:

- **`candump`** — is expected traffic present?
- **Bus state and counters** — is the shared medium unhappy, or just quiet?
- **Node serial output** — is the sender still running?
- **A controlled physical change** — does the fault follow the hardware?

That's not a lookup table. It's a progressively narrower search — which is
closer to how real robot debugging actually works.
