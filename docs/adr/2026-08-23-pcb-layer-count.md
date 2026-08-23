# The first PCB spin is four layers: Top signal / GND / Power / Bottom signal

**Date:** 2026-08-23 **Status:** Accepted
**Issue:** #30
**Supersedes:** the Decision Log row *"2-layer vs 4-layer PCB: Evaluate after the
breadboard. 2-layer is cheaper for early iterations"* in
`todos/prototyping_todo.md`.

## Context

The board is designed for two layers and has to be built on one number or the
other before routing starts. `todos/prototyping_todo.md` had not settled which:
the Decision Log carried the row above at TBD, while Phase 8 already opened with
*"4-layer stackup: Top / GND (L2) / Power (L3) / Bottom"*. Layout is where
reversing this costs the most, so it is settled here first.

Read from
`hardware/fortunelabs_mainboard_v0/fortunelabs_mainboard_v0.kicad_pcb` rather
than assumed, the board today is:

- **Two copper layers**, `F.Cu` and `B.Cu`. No `stackup` block, so the fab
  default applies. 1.6 mm finished.
- **116 footprints placed, zero track segments.** Placement is done; routing has
  not started. Changing the layer count now costs a re-pour and nothing else.

Two properties of the design bear on the choice:

- **One switching regulator.** U101, TPS62162DSG, with L101 at 2.2 µH, per
  [`2026-07-29-power-rail-is-tps62162.md`](./2026-07-29-power-rail-is-tps62162.md).
  Its return current is the only fast loop on the board.
- **No controlled-impedance net.** U201 is an ESP32-S3-WROOM-2, a module with
  its own antenna and matching behind a shield. The schematic has no U.FL, no
  SMA and no antenna connector, so there is no RF trace and nothing needs a
  50 Ω target.

The second point matters because it removes the argument Phase 8 was written
on. What is left is the switcher return path, `+3V3` distribution to 39 nodes
across 116 footprints, and routing headroom.

## Alternatives

- **Two layers.** Cheapest per spin, and the first spin of a first hardware
  project is the one most likely to be thrown away. With the antenna inside the
  module and only one switcher, the usual technical objections are weak. Against
  it: the I²C fanout across the ADS1115 pair, MCP23017, DS3232M, TPL5010 and the
  SD card spends the bottom layer on signal escape, leaving the ground pour a
  patchwork interrupted by every crossing trace. The switcher return then takes
  whatever path the pour happens to leave it, which is probably adequate at this
  power level but is not calculable. `+3V3` reaches 39 nodes as traces.
- **Four layers, Top / GND / GND / Bottom.** Both signal layers reference
  ground, so every return path runs directly beneath its trace. Defensible here
  because the board is close to single-rail: `+3V3`, plus a `+12V` corner
  feeding U101 and J5. A dedicated power plane on a one-rail board is a plane
  spent on very little. Rejected because `+12V` would cross the board as a
  trace, the Phase 8 widths (≥25 mil main, ≥20 mil VDD3P3) are easier to satisfy
  from a plane, and it departs from the stackup Phase 8 already names for a
  benefit this board is unlikely to be able to measure.
- **Four layers, Top / GND / Power / Bottom.** Chosen. A continuous L2 ground
  gives the switcher node and the module supply pins a return path that exists
  by construction rather than by pour luck; L3 carries `+3V3` and the `+12V`
  corner as planes. It is the default build at every low-cost fab, so it is a
  stocked stackup needing no special order. Its cost: **bottom-layer signals
  reference the power plane, not ground**, so their return crosses the PWR/GND
  pair through decoupling capacitance instead of running beneath the trace.
  Standard, and adequate at these edge rates, but a real property of this
  stackup rather than a free one.
- **A two-layer bring-up board now, four layers for the real one.** Two spins,
  two BOMs, and the power question gets answered on a board whose stackup does
  not resemble the one that ships. It buys a cheap mistake and postpones the
  expensive one.

## Decision

The board is **four copper layers on 1.6 mm finished thickness**, in the order:

| Layer | KiCad name | Use |
|---|---|---|
| L1 | `F.Cu` | signal, component side |
| L2 | `In1.Cu` | GND, continuous |
| L3 | `In2.Cu` | power: `+3V3`, with `+12V` local to U101 and J5 |
| L4 | `B.Cu` | signal |

Built on the fab's stocked four-layer stackup. **No controlled-impedance order
is placed**, because no net on this board requires one.

The decision is taken on the asymmetry issue #30 identified, which survives the
loss of the RF argument: being wrong about two layers is discovered at bring-up
as switcher noise on the 3.3 V rail and is not fixable without a re-layout, a
full spin plus the assembly already bought. Being wrong about four layers costs
money per spin and nothing else.

## Consequences

- **The `.kicad_pcb` declares two copper layers and must be set to four** in
  Board Setup before routing starts.
- **The module antenna needs a keepout spanning all four copper layers.** The
  `RF_Module:ESP32-S3-WROOM-2` footprint as placed defines no copper keepout
  over its antenna. On two layers the pours were placed by hand and the omission
  was survivable; with L2 and L3 poured board-wide, copper lands under the
  antenna unless a keepout is drawn and the module set at the board edge. **DRC
  does not report this**, so it is owed an issue before the Phase 8 DRC gate is
  opened.
- **The Phase 8 line *"50Ω RF trace impedance: calculator, verified against the
  vendor stackup"* grades a net this board does not have** and should be struck
  or re-scoped rather than ticked. Not edited here: a specification change and
  the result it grades never share a commit.
- **The Phase 8 DRC gate now checks a four-layer board**, which is why this
  closes before that gate opens.
- **The cost falls entirely on the bare-board line** of a five-piece run, which
  stays small against the BOM and assembly. That was the argument.

## What would reopen this

A later spin that drops the module for a bare ESP32-S3 with a discrete antenna
and matching network. Controlled impedance then becomes a real requirement, and
the question turns into dielectric height and trace geometry rather than layer
count, which would no longer be in doubt.
