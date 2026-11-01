# SINCRIA sequencer guide

This guide describes the Eurorack mode of `SINCRIA_Scansione_Righe`, Handheld Games firmware dated 28 October 2026. See [EUROBOY](EUROBOY.md) for the two arcade modes.

## Start a pattern

Power on without a game combination. Select INT CLOCK, turn the clock knob and press NEW GAME. The internal range is **30–300 BPM**, with four steps per beat. For external timing, select EXT CLOCK and use the input identified by `CLOCK_IN_PIN` in the [pin map](DEVELOPMENT.md).

The display reads across each physical row from left to right, starting at the top. The inverted cursor marks the next cell to be evaluated. Each tick edits the cursor cell if requested, emits the step/live events, advances the position and refreshes the cursor. After step 64, the generation can evolve.

## Controls

| Control | Normal | Alternate |
| --- | --- | --- |
| NEW GAME | New non-empty random world and scan reset | Scan reset; LIVE preview if the first cell is alive |
| LOCK | Freeze/unfreeze evolution and automatic regeneration | Circular shift of grid bits |
| ROTATE | Rotate the generation | Circular shift of grid rows |
| SPAWN | Add one randomly selected dead cell | With EXT clock, paint the cursor alive; hold to paint each tick |
| KILL | Remove one randomly selected live cell | With EXT clock, erase the cursor; hold to erase each tick |
| ALT MODE | Hold about 200 ms to toggle alternate controls | ALT LED shows the active control set |

Outside alternate external-clock painting, SPAWN and KILL produce one edit per press, with no automatic repeat. KILL takes priority when both painting commands are held. Manual editing and NEW GAME remain available while the generation is locked.

## Evolution

Conway B3/S23 rules: a dead cell is born with three neighbours; a live cell survives with two or three. The musical grid wraps across columns but not rows. Life arcade mode uses a separate fully toroidal rule implementation.

The sequencer detects exact repeating patterns with periods 1–4. A detected repetition completes a scan before regeneration. Predicted extinction regenerates without scanning an empty world. LOCK suppresses both evolution and automatic regeneration.

## Timing and outputs

- CLOCK: event for every accepted step.
- LIVE/GATE: event when the current cell is alive.
- END: event when automatic regeneration creates a new world.
- Output pulse target: **10 ms**, serviced by a non-blocking loop. Retriggering before expiry extends the pulse, so closely spaced events can merge.
- External clock qualification: **5 ms** stable active and inactive states. A clock already active on entry to EXT does not create a false step.
- Command filter: **2 ms**, with at least **30 ms** between accepted command activations. ALT debounce: **50 ms**.

The INT/EXT selector qualifies changes for 50 ms. At startup an invalid selector state defaults to EXT waiting for clock; during operation an intermediate state retains the current mode. The internal clock starts after a complete interval.

The [schematic](../hardware/schematics/README.md) documents the electrical interface. Firmware timing values are targets; verify actual levels, loading and timing on the board revision in use.
