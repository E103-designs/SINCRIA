# SINCRIA

### Evolving rhythms. Pixel-sized worlds.

**A Eurorack cellular-automaton sequencer, dual gate sequencer and pocket-sized arcade.** Choose from five GoL rules, program two independent rhythmic loops, or reboot into **EUROBOY** for Snake and interactive Life on the same 8 × 8 matrix.

<p align="center"><img src="images/front.png" height="500" alt="SINCRIA front view"> <img src="images/angled.png" height="500" alt="SINCRIA three-quarter view"></p>

[User guide](docs/USER_GUIDE.md) · [Manuale italiano](docs/USER_GUIDE_IT.md) · [PDF EN](docs/SINCRIA-User-Manual-EN.pdf) · [PDF IT](docs/SINCRIA-Manuale-Utente-IT.pdf) · [Firmware](firmware/SINCRIA_Scansione_Righe/SINCRIA_Scansione_Righe.ino) · [Build and modify](docs/DEVELOPMENT.md) · [Schematics](hardware/schematics/README.md)

## One module, four modes

| Mode | What it does | Enter |
| --- | --- | --- |
| **SINCRIA GoL** | A 64-step scan with five selectable cellular-automaton rules | Power on normally |
| **Dual Gate Sequencer** | Two gate patterns with independent 1-32 step lengths | **ALT MODE + ROTATE** during GoL |
| **EUROBOY: Snake** | Steer, collect food and fill the 8 × 8 world | Hold **ALT MODE + SPAWN** during power-on or reset |
| **EUROBOY: Life** | Draw a colony, animate it, pause and reshape it | Hold **ALT MODE + KILL** during power-on or reset |

Release the buttons after the game entry animation. To return to the sequencer, reboot without a game combination. Holding ALT MODE + SPAWN + KILL also selects the sequencer.

## Let the pattern write the rhythm

SINCRIA scans the physical display from left to right, one row at a time from top to bottom. The inverted cursor marks the **next step**. CLOCK emits a pulse on every step, LIVE/GATE responds to living cells and END marks automatic regeneration.

- Internal clock: **30–300 BPM**, four steps per beat, or an external clock.
- Five GoL rules, with evolution after a complete scan; the musical grid keeps its original one-axis wrapping.
- **NEW GAME** creates a new world; **LOCK** freezes evolution and regeneration.
- **ROTATE**, **SPAWN** and **KILL** reshape the pattern while it runs.
- Hold **ALT MODE alone**, then release, to latch scan reset, grid shifts and live-cell painting with external clock.
- **ALT MODE + NEW GAME** opens the rule selector; **ALT MODE + KILL** held there clears the field and keeps it empty until edited.
- Empty patterns and short repeating cycles can regenerate into new material; LOCK keeps a chosen pattern in place.

[Read the sequencer controls and timing details](docs/USER_GUIDE.md).

## Choose how the colony evolves

| Rule | Code |
| --- | --- |
| Classic Conway | **B3/S23** |
| SINCRIA | **B234/S01** |
| Extended survival | **B3/S234** |
| Mazectric | **B3/S1234** |
| Coagulations | **B378/S235678** |

Open the selector with **ALT + NEW GAME**. Five LEDs across the top row identify the rules; the selected one blinks. Navigate with LOCK/SPAWN or KILL/ROTATE, then confirm with NEW GAME for a fresh random colony. Hold ALT+KILL for about a second to start from silence instead. [Rule selection and clearing](docs/USER_GUIDE.md#choosing-the-gol-rule).

## Two rhythms, one clock

The upper four rows become sequence 1 on **GateOut**; the lower four become sequence 2 on **GameEnd**. Each channel has 32 gate steps and its own **1-32 step length**, sharing the existing internal/external clock. Program a steady loop or let different lengths produce changing phase relationships.

Dual enters paused. **NEW GAME** toggles a step, **ALT+LOCK** plays/pauses, **ALT+SPAWN** sets the endpoint at the cursor and **ALT+NEW GAME** realigns both playheads. LOCK/KILL move up/down, SPAWN/ROTATE left/right, leaving GATE IN dedicated to clock. **ALT+ROTATE** returns to the previous GoL colony. [Dual controls and a 5-against-7 patch](docs/USER_GUIDE.md#dual-gate-sequencer).

## EUROBOY: the arcade inside your rack

**Snake** turns the LED matrix into a tiny playfield. Steer with the panel buttons, set the speed with the clock knob and wrap across the edges. Fill all 64 cells to win; press SPAWN after the end screen to restart.

**Interactive Life** gives you an editable pixel colony. Move the cursor, add cells with NEW GAME, erase with ROTATE and switch between INT to evolve and EXT to pause. Unlike the musical grid, this mode wraps across **both axes** and keeps still lifes and oscillators running without random regeneration.

| Direction | Panel control |
| --- | --- |
| Up | LOCK |
| Down | KILL |
| Left | SPAWN |
| Right | GATE IN |

Musical outputs remain low throughout both games. Use the module's normal power arrangement; USB powers the Nano but does not necessarily power the complete display/control circuit. [Full arcade guide](docs/EUROBOY.md).

## Install the firmware

Current firmware: **GoL rules + Dual Gate Sequencer, 3 November 2026**, sketch `SINCRIA_Scansione_Righe`. The project owner confirmed successful testing on the module on **10 November 2026**.

1. Download and extract the repository.
2. Install **LedControl 1.0.6** in Arduino Library Manager.
3. Open `firmware/SINCRIA_Scansione_Righe/SINCRIA_Scansione_Righe.ino`.
4. Select **Arduino Nano → ATmega328P**, using Old Bootloader when required by the installed Nano.
5. Upload, then start normally for sequencing or hold a game combination at reset.

The default input profile is **1**, for the inverting transistor inputs used by Rev2.2/Rev2.3. Profile 0 supports the earlier active-high wiring. Check the [pin map and board profile](docs/DEVELOPMENT.md) before flashing a different revision. The older `SINCRIA_Rev2_OK` sketch remains available for historical reference.

## Open source

Full Arduino source, current application HEX, English/Italian PDF manuals, regression tests, development instructions and circuit schematic are included. Change the automaton, scan order, timing or games using the [development guide](docs/DEVELOPMENT.md). [Validation results](docs/PUBLICATION_CHECKS.md) describe software coverage and resource use.

<details><summary>Side view</summary>
<p align="center"><img src="images/side.png" height="360" alt="SINCRIA side view"></p>
</details>

**GNU GPL-3.0.** [License](LICENSE) · [Attribution](NOTICE.md) · [Contribute](CONTRIBUTING.md).

Other E103 projects: [faun](https://github.com/E103-designs/faun) · [ALTERIA](https://github.com/E103-designs/ALTERIA) · [euroTrack](https://github.com/E103-designs/euroTrack).
