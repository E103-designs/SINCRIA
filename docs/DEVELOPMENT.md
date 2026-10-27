# Build and modify SINCRIA

## Files and dependencies

`firmware/SINCRIA_Rev2_OK/SINCRIA_Rev2_OK.ino` is the unmodified source found locally, internally dated 4 August 2025. Keep the `.ino` basename and directory name identical for Arduino. It includes **LedControl 1.0.6** by Eberhard Fahle, installed separately; the library retains its own license. The original upstream is https://github.com/wayoda/LedControl.

Build baseline: Arduino AVR core **1.8.6**, Arduino Nano **ATmega328P, 16 MHz**. This validates that target's compilation, not the identity of the processor fitted to every SINCRIA board.

```sh
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.6
arduino-cli lib install "LedControl@1.0.6"
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 --build-path /tmp/sincria-build firmware/SINCRIA_Rev2_OK
arduino-cli board list
# After confirming board revision, MCU and port:
arduino-cli upload --port PORT --fqbn arduino:avr:nano:cpu=atmega328 firmware/SINCRIA_Rev2_OK
```

For IDE users select Arduino Nano / ATmega328P; use Old Bootloader only if required by the board (`cpu=atmega328old` on CLI). No pattern restore is available after a power cycle in this sketch.

## Firmware pin map

| Function | Nano pin |
| --- | --- |
| MAX7219 DIN / CLK / CS | D12 / D10 / D11 |
| Tempo | A0 |
| Internal / external clock selector | A1 / A2 |
| NewGame / live-cell event / game-end event | D2 / D3 / D4 |
| Clock output | D5 |
| Lock / external clock / Rotate / AltMode | D6 / D7 / D8 / D9 |
| Spawn / Kill / Alt LED | A3 / A4 / A5 |
| Random seed | A6, assumed unconnected |

This MAX7219 driver uses software signals through LedControl; do not replace it with ALTERIA's hardware SPI mapping. The source assumes **external pulldowns and active-HIGH controls**. Pin values are firmware facts; safe jack voltages and physical jack wiring require the corrected schematic. GateIn has no separate handler in this source.

## Function map

| Area | Functions / state | Useful changes |
| --- | --- | --- |
| World storage | `currentGen`, `nextGen`, `previousGen`: eight bytes each | Change grid dimensions only with coordinated display/scan changes |
| Rules | `countLiveNeighbors`, `computeNextGeneration`, `willNextGenerationBeGameOver` | Change birth/survival rules in both generation calculations; preserve matching end detection |
| Traversal | `advanceSequencer`, `getCurrentRow`, `getCurrentCol`, `currentStep` | Change scan order, generation boundary or startup step explicitly |
| Clock | `loop`, `updateClockMode`, `updateBPM`, `stepInterval` | Tempo range, debounce, trigger timing, external synchronization |
| Controls | `handleInputs` | Short/held gestures, generation lock, scan restart, write/erase |
| Pattern edits | `rotateGeneration`, `shiftCellsLeft`, `shiftCellsUp`, `addRandomCell`, `killRandomCell` | Transformations and placement distributions |
| Display | `updateDisplay`, `lc` | Brightness, bit orientation and scan highlighting |
| Reseeding | `generateRandomGeneration`, `isGameOver`, `isGameStalled` | Density and stationary-state policy |

The world wraps horizontally, not vertically. `computeNextGeneration()` and the preflight `willNextGenerationBeGameOver()` duplicate the rule calculation; changing only one creates inconsistent behavior. The first normal tick advances from 0 to 1. Decide explicitly if altering that, because reset and boundary behavior depend on the same state.

On AVR SRAM is limited. Random add/remove each allocate an `int[64]` candidate array on the stack: 128 bytes with 16-bit AVR ints. Do not infer heap/stack safety solely from the static SRAM report. Keep display writes and timing non-blocking when introducing features. Use unsigned subtraction for rollover and test both clock edges and mode-switch transitions.

## Verification for a pull request

Record MCU/core/library versions, build result, flash and static SRAM; then test internal tempo, external clocks above/below debounce, step order, boundary evolution, lock, all transforms, alternate gestures, blank/full grids and reseeding. Check trigger duration and jitter with real instruments, and board conditioning before applying external signals. New deterministic rule tests are useful for a behavior change; this publication does not claim an existing host test suite or hardware validation.

Publish a new firmware version/date and changelog when changing behavior. The unmodified source hash is recorded in `SOURCE_MANIFEST.json`. Corrected schematics and editable CAD belong in `hardware/schematics/` with the exact PCB revision.
