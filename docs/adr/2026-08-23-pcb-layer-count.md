# The first PCB spin is four layers: Top signal / GND / Power / Bottom signal

**Date:** 2026-08-23 **Status:** Accepted
**Issue:** #30
**Supersedes:** the Decision Log row *"2-layer vs 4-layer PCB: Evaluate after the
breadboard. 2-layer is cheaper for early iterations"* in
`todos/prototyping_todo.md`.

## Context

`todos/prototyping_todo.md` held both an open decision and its outcome. The
Decision Log carried the row above, dated TBD; Phase 8 opened with *"4-layer
stackup: Top / GND (L2) / Power (L3) / Bottom"* and went on to *"50Ω RF trace
impedance: calculator, verified against the vendor stackup"*. One of the two was
wrong, and layout is where reversing the wrong one costs the most.

### State of the board when this was decided

Read from `hardware/fortunelabs_mainboard_v0/fortunelabs_mainboard_v0.kicad_pcb`
rather than assumed:

- The file declares **two copper layers**, `(0 "F.Cu" signal)` and
  `(2 "B.Cu" signal)`. No `stackup` block, so the fab's default applies.
  Board thickness 1.6 mm.
- **116 footprints are placed. Zero track segments exist.** One `Edge.Cuts`
  outline. Placement has been done; routing has not started.

Issue #30 said the PCB file was empty and called that the cheapest possible
moment to decide. That is no longer true, but with nothing routed the layer
count is still nearly free to change. This is the last cheap moment rather than
the cheapest one.

### Two premises in issue #30 that the tree contradicts

**There is one switcher, not two.** Issue #30 argued that *"the two MPS
switchers put their return currents through a plane that is not there"*. The
board has a single **U101, TPS62162DSG**, with **L101, 2.2 µH**. The MP2393 and
MP2388 were retired on sourcing grounds by
[`2026-07-29-power-rail-is-tps62162.md`](./2026-07-29-power-rail-is-tps62162.md),
and issue #30 was written against the superseded design.

**The RF argument is weaker than the issue assumed, and the issue said so.**
Issue #30 named the deciding question: *"whether the ESP32-S3 RF section is on
this board."* It is not. **U201 is an ESP32-S3-WROOM-2**, a module carrying its
own antenna and matching network behind a shield. There is no U.FL, no SMA and
no antenna connector anywhere in the schematic, and therefore **no RF trace on
this board for a 50 Ω target to apply to**. The Phase 8 controlled-impedance
line grades a net that does not exist.

With RF removed, the case for four layers rests on the switcher return path, on
distributing `+3V3` to 39 nodes across 116 footprints, and on routing headroom.
That is a weaker case than the one Phase 8 was written on, and the decision is
recorded as having been taken with the weaker case known.

## Alternatives

- **Two layers.** For: cheapest per spin, and the first spin of a first hardware
  project is the one most likely to be thrown away, so cheap iterations buy real
  learning. The module retires the RF objection almost entirely, and one
  switcher at 2.2 µH is a far smaller return-path problem than two. A two-layer
  board also makes ground mistakes audible instead of hiding them, which on a
  first board is worth something. Against: with 116 footprints and the I²C
  fanout across ADS1115 pair, MCP23017, DS3232M, TPL5010 and the SD card, the
  bottom layer goes to signal escape, and the ground pour becomes a patchwork
  interrupted by every crossing trace. The TPS62162 return current then takes
  whatever path the pour happens to leave it. Not fatal at this power level, but
  not calculable either, and `+3V3` reaches 39 nodes as traces rather than as a
  plane.
- **Four layers, Top / GND / GND / Bottom.** Both signal layers reference a
  ground plane, so every return path is directly beneath its trace. Defensible
  here precisely because the board is essentially single-rail: `+3V3` plus a
  `+12V` corner feeding U101 and J5. A dedicated power plane on a one-rail board
  is a plane spent on very little. Rejected because `+12V` would then cross the
  board as a trace, the Phase 8 widths (≥25 mil main, ≥20 mil VDD3P3) are easier
  to satisfy from a plane, and deviating from the stackup Phase 8 already names
  costs a specification edit for a benefit this board is unlikely to be able to
  measure.
- **Four layers, Top / GND / Power / Bottom.** Chosen. A continuous L2 ground
  under the top layer gives the switcher node and the module's supply pins a
  return path that exists by construction rather than by pour luck; L3 delivers
  `+3V3` and the `+12V` corner as planes; and the arrangement is the default
  build at every low-cost fab, so the stackup is a stocked one and needs no
  special order. Its honest cost: **bottom-layer signals reference the power
  plane, not ground.** Their return current crosses the PWR/GND pair through
  decoupling capacitance rather than flowing directly beneath the trace. This is
  standard practice and adequate at these edge rates, but it is a real property
  of this stackup and not a free one.
- **A two-layer bring-up board now, four layers for the real one.** From issue
  #30. Rejected there and rejected here: two spins, two BOMs, and the power
  question gets answered on a board whose stackup does not resemble the one that
  ships. It buys a cheap mistake and postpones the expensive one.

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

## What this decision does not claim

It does not claim four layers was shown to be necessary. The measurement that
would show it has not been taken, and the argument Phase 8 was written on, the
50 Ω RF trace, turned out to grade a net this board does not have. Four layers
was bought as insurance against an unmeasured risk, at a known and small price.
Recorded that way so it is not read later as a considered technical conclusion
that never happened.

## Consequences

- **The `.kicad_pcb` declares two copper layers and must be changed to four** in
  Board Setup before routing. Nothing is routed, so this costs nothing beyond
  re-pouring. That edit is the user's to make in KiCad.
- **The module antenna needs a keepout on all four copper layers, and the
  footprint does not carry one.** The `RF_Module:ESP32-S3-WROOM-2` footprint as
  placed in the tree defines no copper keepout zone over its antenna. On two
  layers the pours were placed by hand and the omission was survivable. With L2
  and L3 poured board-wide, **copper will land under the antenna** unless a
  keepout spanning F.Cu, In1.Cu, In2.Cu and B.Cu is drawn over the antenna
  region, with the module set at the board edge. **DRC will not report this.**
  It is the one way this decision can make WiFi worse rather than better, and it
  is owed an issue before the Phase 8 DRC gate is opened.
- **The Phase 8 line *"50Ω RF trace impedance: calculator, verified against the
  vendor stackup"* grades nothing** and should be struck or re-scoped rather
  than ticked. Not edited in this commit: a specification change and the result
  it grades never share a commit, and that line is a grading criterion.
- **The Phase 8 DRC gate now checks a four-layer board.** Per issue #30 this
  decision had to close before that gate was opened, and it does.
- **Cost falls entirely on the bare-board line** of a five-piece run, which
  stays small against the BOM and assembly. That was the argument.

## What would reopen this

A later spin that drops the module for a bare ESP32-S3 plus a discrete antenna
and matching network. The RF argument returns in full, controlled impedance
becomes a real requirement rather than an inherited checklist line, and the
stackup question changes shape: it would then be about dielectric height and
trace geometry, not about layer count, which would no longer be in doubt.
