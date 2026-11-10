# SINCRIA

User manual - GoL rules, Dual Gate Sequencer and EUROBOY

Firmware: 3 November 2026. Manual: 10 November 2026.

SINCRIA is an 8 x 8 cellular-automaton sequencer for Eurorack. It also contains a dual gate sequencer, Snake and interactive Conway's Game of Life. The same controls take different roles in each mode.

## Quick start

1. Start normally, without holding a game combination.
2. Select INT CLOCK and turn the tempo knob. Press NEW GAME for a new colony.
3. Patch GateOut to an envelope or percussion trigger. CLOCK OUT supplies the step clock; GameEnd marks automatic colony regeneration.
4. Hold ALT MODE and press NEW GAME to choose a rule, or ALT MODE and ROTATE to enter the dual sequencer.

The scan starts at the **top left**, reads a complete row left to right and continues on the row below. The inverted pixel indicates the **next** cell to be read. A dark cursor can therefore indicate a living cell underneath it.

For combinations, hold ALT MODE first, then press the second button. Release the controls after a screen or mode change. A combination preserves the previous musical ALT latch, even when ALT was pressed early.

| Where to go | Page |
| --- | --- |
| GoL sequencing and ALT controls | 2 |
| Five rules and an empty starting field | 3 |
| Dual Gate Sequencer | 4 |
| EUROBOY games | 5 |
| Patch examples | 6 |
| Installation and troubleshooting | 7 |

[Project, source and schematics](https://github.com/E103-designs/SINCRIA).

<!-- PAGE -->
## GoL sequencing

Each accepted clock reads one of the 64 cells. An active cell produces a GateOut pulse. At the end of the scan the colony may evolve according to the selected rule. The clock continues through silent cells.

| Control | Normal action |
| --- | --- |
| NEW GAME | Create a non-empty random colony; reset the scan |
| LOCK | Freeze/unfreeze evolution and automatic regeneration |
| ROTATE | Rotate the colony |
| SPAWN | Add one randomly selected cell |
| KILL | Remove one randomly selected cell |
| ALT MODE alone | Hold about 250 ms including debounce to toggle the ALT control set |

The ALT LED indicates the latched ALT control set in GoL. Release ALT after toggling; it is a latch, not a modifier that must stay held for the older alternate functions.

| Control with ALT latched | Alternate action |
| --- | --- |
| NEW GAME | Reset scan; preview GateOut if its first cell is alive, without a clock pulse |
| LOCK | Circular shift of grid bits |
| ROTATE | Circular shift of grid rows |
| SPAWN with EXT selected | Make the cursor cell alive; hold to paint at each clock |
| KILL with EXT selected | Erase the cursor cell; hold to erase at each clock |

KILL takes priority over SPAWN during simultaneous painting. Outside painting, SPAWN/KILL act once per press. Editing remains available with LOCK engaged.

## Clock and outputs

INT provides **30-300 BPM**, four steps per beat. EXT accepts the GATE IN/ClkIn clock channel. The panel GATE IN button can also advance one step in EXT. CLOCK OUT pulses on every step; GateOut pulses for a living cell; GameEnd pulses when a new random colony replaces the old one.

GameEnd is **not** a pulse on every 64th step. Extinction and exact repetitions of period 1-4 can trigger regeneration; LOCK suppresses this. A repetition is scanned before replacement. The musical grid retains its original one-axis wrapping; EUROBOY Life wraps both axes.

Output pulses target 10 ms. A closely spaced retrigger can extend an existing pulse. The external clock requires stable active/inactive levels of about 5 ms. Switching to an already-active external clock does not create a false step.

<!-- PAGE -->
## Choosing the GoL rule

During GoL sequencing, hold **ALT MODE + NEW GAME**. The scan and musical outputs pause. Five LEDs on the top row identify the rules, from left to right; the selected LED blinks while the other four remain on.

| LED | Rule | Code |
| --- | --- | --- |
| 1 | Classic Conway | B3/S23 |
| 2 | SINCRIA | B234/S01 |
| 3 | Extended survival | B3/S234 |
| 4 | Mazectric | B3/S1234 |
| 5 | Coagulations | B378/S235678 |

**B** lists neighbour counts that make a dead cell become alive. **S** lists counts that keep an already-live cell alive. SINCRIA therefore creates cells with 2, 3 or 4 neighbours and preserves live cells with 0 or 1. Its code is B234/S01, not B3/S234.

| In the rule screen | Action |
| --- | --- |
| LOCK or SPAWN | Previous rule |
| KILL or ROTATE | Next rule |
| NEW GAME | Apply the rule, create a random colony and restart at step 1 |
| ALT MODE + KILL, about one second | Apply the rule, clear the colony and return to GoL |

ROTATE is the rightward navigation control in the new screens. GATE IN remains a clock input and cannot change the selected rule. Release the controls after confirming. Internal timing restarts with a full interval; external timing waits for a valid new edge.

## Starting with silence

The long clear command leaves **all 64 LEDs off, including the cursor**. Automatic repopulation stays suspended while the colony is empty. CLOCK OUT can continue, while GateOut and GameEnd remain inactive for that empty colony.

Create a cell with SPAWN or alternate painting to resume normal behaviour. A normal NEW GAME generates another random colony. NEW GAME with the old ALT set latched still performs its alternate scan-reset action.

More permissive survival does not guarantee a longer evolving sequence: stable or short-period colonies can still regenerate. Use LOCK to keep a chosen musical pattern. Rule selection is held in RAM and returns to Classic at power-on. The selected musical rule does not alter EUROBOY Life.

<!-- PAGE -->
## Dual Gate Sequencer

Hold **ALT MODE + ROTATE** to switch between GoL and Dual. Dual enters **paused**. The upper four rows form sequence 1, routed to **GateOut**; the lower four rows form sequence 2, routed to **GameEnd**. Each half contains 32 ON/OFF steps, read by rows from its own top-left corner.

| Dual control | Action |
| --- | --- |
| LOCK / KILL | Cursor up / down |
| SPAWN / ROTATE | Cursor left / right |
| NEW GAME | Toggle the step under the cursor |
| ALT MODE + LOCK | Play/pause both sequences |
| ALT MODE + SPAWN | Set this sequence's last step at the cursor |
| ALT MODE + NEW GAME | Reset both playheads to step 1 without changing patterns |
| ALT MODE + ROTATE | Return to GoL |
| GATE IN | External clock only |

Navigate across the middle of the display to select the other sequence. The cursor wraps on both axes. Each press gives one movement or toggle; held direction buttons do not repeat. Editing is available while playing.

The editor cursor gives a short inverted flash. During playback, two additional inverted playheads indicate the next steps to be read. If a cursor lies on an active step its flash darkens that pixel. The display overlay never changes the stored step.

## Independent lengths, shared clock

Both channels use the same INT/EXT clock and tempo knob. Each length is independently **1-32**. Move to the desired last step and press ALT+SPAWN. For about 700 ms the display shows the positions included in both lengths. Steps outside the length are hidden and silent but remain stored; extending the length restores them.

If a playhead lies beyond a newly shortened length, that channel returns to step 1. The other channel keeps its position. Use ALT+NEW GAME to align both starts. A 5-step sequence and a 7-step sequence realign after **35 clocks**.

The ALT LED is on while Dual plays. CLOCK OUT provides the common clock. GameEnd is exclusively sequence 2's gate here, with no extra end-of-cycle event. Pause or a mode change brings the outputs low. GoL colony, rule, scan position and ALT state are retained separately. Dual data remain in RAM across mode changes but are lost at power-off.

<!-- PAGE -->
## EUROBOY games

Hold a game combination during power-on or reset. Release all controls after the short entry animation. To leave a game, reboot without a game combination.

| Boot combination | Mode |
| --- | --- |
| ALT MODE + SPAWN | Snake |
| ALT MODE + KILL | Interactive Life |
| ALT MODE + SPAWN + KILL | Musical GoL |
| No valid game combination | Musical GoL |

In **both games**, movement is LOCK up, KILL down, SPAWN left and **GATE IN right**. This differs from the new musical screens, where ROTATE is right and GATE IN remains clock.

## Snake

The snake starts with three segments and moves right. Collect the lit food pixel to grow. Food appears only on a free cell. All four edges wrap. Immediate reverse turns are rejected and only one turn is queued for each movement.

The tempo knob sets **700-100 ms per movement**. INT/EXT does not affect Snake. Hitting the body ends the game; moving into the departing tail cell is allowed. Fill all 64 cells to win.

After collision or victory the matrix flashes for 600 ms and then holds the result. Press SPAWN again to restart. A press during the flashing phase is not queued for later.

## Interactive Life

Life starts empty, with a cursor near the centre. Use EXT to draw while paused. NEW GAME creates a cell; ROTATE erases one. INT starts evolution. The knob sets **2,000-100 ms per generation**. Editing remains possible while running.

The cursor inverts its pixel for 120 ms, then restores it for 600 ms. A live cell briefly darkens; an empty cell briefly lights. This overlay does not change the colony. The cursor and Conway B3/S23 field wrap on both axes. Still lifes, oscillators and empty fields remain intact without random regeneration.

The switch needs 50 ms to qualify a change; resuming waits a full generation interval. GATE IN moves right in this game and does not clock the simulation. Musical outputs stay low throughout both games. Use the module's normal power arrangement: USB power for the complete display/control circuit depends on the fitted hardware.

<!-- PAGE -->
## Patch examples

### An evolving percussion voice

Connect GateOut to an envelope or percussion trigger and CLOCK OUT to another module's clock input. Start GoL on INT and choose a rule with ALT+NEW GAME. SPAWN/KILL adjust density; LOCK preserves a useful groove while the cursor continues scanning. GameEnd can reset another sequencer when SINCRIA regenerates a colony, creating longer changing phrases.

An empty cell is a rest. The selected rule affects how the pattern changes after each scan, not the timing or pitch of individual steps. GameEnd follows regeneration, not every scan boundary.

### Two percussion lines in 5 and 7

1. Enter Dual with ALT+ROTATE. Patch GateOut to a kick trigger and GameEnd to a second percussion voice.
2. In the upper half, turn on steps 1, 3 and 5 with NEW GAME. Put the cursor on step 5 and press ALT+SPAWN.
3. In the lower half, turn on steps 1, 4 and 6. Put the cursor on step 7 and press ALT+SPAWN.
4. Use INT or patch a clock into GATE IN and select EXT. Press ALT+NEW GAME to align, then ALT+LOCK to play.

The two loops meet every 35 clocks. Change a step while playing with NEW GAME. To shorten just one loop, place the cursor on its new endpoint and use ALT+SPAWN. Returning to GoL restores the previous colony.

### Draw a musical pattern from silence

Open the rule screen and hold ALT+KILL for about one second. Release the controls; the colony stays empty. Select EXT. Toggle the old ALT control set by holding ALT alone and releasing it. Hold SPAWN while sending clock edges to paint living cells at the cursor; use KILL to erase. The first actual creation releases the empty-field hold.

To keep the result, leave painting and engage LOCK with the normal control set. CLOCK OUT continues and GateOut plays the living cells; automatic regeneration is frozen. In the latched ALT set, LOCK shifts the grid instead, so check the ALT LED before locking.

### Draw without any musical outputs

Boot EUROBOY Life with ALT+KILL. Select EXT, draw three adjacent cells, then select INT. The blinker alternates orientation indefinitely. This is the fully toroidal interactive game, independent of the musical rule and regeneration mechanism.

<!-- PAGE -->
## Installation and reference

Install **LedControl 1.0.6**. Open the current `firmware/SINCRIA_Scansione_Righe/SINCRIA_Scansione_Righe.ino` in Arduino IDE. Select **Arduino Nano / ATmega328P** and the bootloader fitted to the board; Old Bootloader is supported. The historical Rev2 sketch is separate.

The default input profile is 1 for inverting transistor inputs. The firmware preserves the existing pinout and does not detect board revision or power source. Schematics and build instructions are linked from the repository.

| Output | Musical GoL | Dual | EUROBOY |
| --- | --- | --- | --- |
| GateOut | Living cell | Sequence 1 | Low |
| GameEnd | Automatic regeneration | Sequence 2 | Low |
| CLOCK OUT | Every accepted step | Common step clock | Low |

## Common questions

**The pattern lasts only a few generations.** Empty colonies and exact repeats of period 1-4 can regenerate. Try another rule or engage LOCK to preserve a groove. More survival conditions can also produce a stable colony sooner.

**The empty field does not repopulate.** This is the intended result of the long clear command. Create a cell or use normal NEW GAME. With ALT latched, NEW GAME resets the scan instead of randomising.

**ROTATE does not rotate.** It is rightward navigation in Dual and the rule screen, and erase in EUROBOY Life. Return to musical GoL for rotation.

**GATE IN does not move right.** In the two new musical screens it is reserved for the clock. It is still rightward navigation in both EUROBOY games.

**Dual is silent on entry.** It starts paused and initially empty. Turn on steps, set lengths, then use ALT+LOCK. Also check INT/EXT and the source clock.

**A jack acts like a button.** Command jacks and panel buttons share electrical inputs. The software cannot distinguish those sources. GATE IN is excluded from navigation in the new musical screens.

**Settings disappeared after power-off.** Rules and dual patterns are volatile. There is no automatic EEPROM save.

Firmware resources: **12,182 bytes Flash, 488 bytes static SRAM**. The project owner reported successful operation on the module on 10 November 2026. See [validation](PUBLICATION_CHECKS.md), [development](DEVELOPMENT.md), [schematics](../hardware/schematics/README.md) and the [Italian manual](USER_GUIDE_IT.md).
