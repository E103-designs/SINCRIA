# EUROBOY arcade modes

EUROBOY brings two pixel games to SINCRIA's existing 8 × 8 display and controls: Snake and interactive Conway's Game of Life. Both remain included in the GoL rules + Dual Gate Sequencer firmware dated 3 November 2026. The musical rule selector does not change the arcade Life rule.

## Choose a mode

Hold the combination while powering on or resetting the Nano:

| Combination | Mode |
| --- | --- |
| ALT MODE + SPAWN | Snake |
| ALT MODE + KILL | Interactive Life |
| ALT MODE + SPAWN + KILL | Eurorack sequencer |
| No valid game combination | Eurorack sequencer |

Mode selection takes 140–240 ms after setup, in addition to bootloader time. The game entry animation lasts 320 ms, followed by a wait for all controls to be released. The initial button press does not move the player. Mode selection is only available at boot; reboot without a game combination to return to sequencing.

## Movement

| Direction | Button | Nano pin |
| --- | --- | --- |
| Up | LOCK | D6 |
| Down | KILL | A3 |
| Left | SPAWN | D8 |
| Right | GATE IN | A4 |

This game mapping uses GATE IN for right. In Dual and the new musical rule screen, ROTATE is right and GATE IN remains clock-only.

Each press produces one direction command, with 30 ms debounce and no automatic button repeat. ALT does not invoke sequencer functions inside a game.

## Snake

The snake starts at length three, moving right near the centre. Collect the lit food pixel to grow; new food appears only in an unoccupied cell.

- The field wraps across all four edges.
- Reverse turns are rejected; one turn is queued per movement step.
- The clock knob sets movement from **700 ms to 100 ms per step**.
- INT/EXT does not affect Snake.
- Hitting the body ends the game. Entering the departing tail cell is allowed.
- Occupying all 64 cells wins.

After a win or collision, the display flashes for 600 ms and then holds the result. A **fresh SPAWN press** restarts the game. A press during the flashing phase is not queued for later.

## Interactive Game of Life

Life begins with an empty field and a cursor near the centre. Select EXT to draw while paused.

| Action | Control |
| --- | --- |
| Add a cell | NEW GAME |
| Erase a cell | ROTATE |
| Evolve | INT CLOCK |
| Pause | EXT CLOCK |
| Generation interval | Clock knob: **2,000–100 ms** |

The cursor wraps in both directions. Its pixel briefly inverts for 120 ms, followed by 600 ms in its normal state; the cursor does not change the colony. Editing is available during evolution. Erase takes priority over add when pressed together; simultaneous directions use up, down, left, right priority.

Life uses Conway B3/S23 rules with wrapping on **both axes**. Empty worlds, still lifes and oscillators remain intact rather than triggering random regeneration. The INT/EXT switch has a 50 ms qualification period; intermediate switch states retain the previous choice. Resuming waits one full generation interval. GATE IN moves the cursor; it does not advance the simulation.

Try drawing three adjacent cells while paused, then select INT: the blinker alternates between horizontal and vertical orientations.

## Rack connections and power

LIVE/GATE, CLOCK and END remain low during entry animations, gameplay and end screens. Jack inputs and panel buttons share electrical inputs, so a sufficiently long external gate can also act as a game command.

Use the board's normal power configuration. On the documented circuit, the Nano USB supply and the display's regulated supply are separate; USB-only operation requires a board arrangement that also powers the matrix and controls. Consult the [schematic](../hardware/schematics/README.md) before changing supply wiring.
