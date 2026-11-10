# Build and modify SINCRIA

Current sketch: `firmware/SINCRIA_Scansione_Righe/SINCRIA_Scansione_Righe.ino`, GoL rules + Dual Gate Sequencer firmware dated 3 November 2026. Target: classic Arduino Nano, **ATmega328P / 16 MHz**.

## Build

Install Arduino AVR Boards **1.8.6** and **LedControl 1.0.6**. LedControl is the only external Arduino library; Arduino and string utilities come with the core.

From repository root:

```sh
arduino-cli core install arduino:avr@1.8.6
arduino-cli lib install "LedControl@1.0.6"
arduino-cli compile --warnings all --fqbn arduino:avr:nano:cpu=atmega328 firmware/SINCRIA_Scansione_Righe
arduino-cli board list
arduino-cli upload --port PORT --fqbn arduino:avr:nano:cpu=atmega328 firmware/SINCRIA_Scansione_Righe
```

Replace PORT with the Nano's serial port. Use `cpu=atmega328old` when required by its bootloader. In Arduino IDE open the `.ino` inside its matching sketch directory; keep the historical sketch in a separate directory.

## Pin map

| Function | Nano pin |
| --- | --- |
| NEW GAME | D2 |
| LIVE/GATE output | D3 |
| END output | D4 |
| CLOCK output | D5 |
| LOCK | D6 |
| ROTATE | D7 |
| SPAWN | D8 |
| ALT MODE | D9 |
| MAX7219 CLK / CS / DIN | D10 / D11 / D12 |
| Clock potentiometer | A0 |
| INT / EXT selector | A1 / A2 |
| KILL | A3 |
| GATE IN / external clock input | A4 |
| ALT LED | A5 |

Old schematic net names differ from current panel labels. Use the source constants above when following the control logic and compare with the [circuit drawing](../hardware/schematics/README.md).

## Compile-time settings

- `SINCRIA_INVERTED_INPUTS=1` is the default for the six inverting transistor inputs on Rev2.2/Rev2.3. NEW/LOCK/ROTATE/SPAWN/KILL and CLOCK IN are active low; ALT and the clock selector remain active high.
- `SINCRIA_INVERTED_INPUTS=0` selects the earlier active-high command wiring with external pulldowns.
- `SINCRIA_SCAN_BY_ROWS=1` scans physical rows from left to right, top to bottom. Set 0 for a physical column scan.
- `DISPLAY_INTENSITY=2` sets MAX7219 intensity.

Choose the input profile for the actual circuit. The firmware does not infer wiring or supply source. D0/D1, D13 and A6/A7 are unused. GPIO values do not define safe external jack voltage limits.

## Source map

| Area | Entry points |
| --- | --- |
| Mode dispatch | `setup`, `loop`, `serviceBootSelection`, `initializeEurorack` |
| Musical scan | `loopEurorack`, `advanceSequencer`, `finishScan` |
| Musical automaton | `golBirthMask`, `golSurvivalMask`, `golRuleResult`, `computeNextGenerationInto`, history and oscillation helpers |
| Rule menu and persistent empty field | `openRuleMenu`, `loopRuleMenu`, `commitRuleMenu`, `holdEmptyGrid` |
| Dual gate sequencer | `toggleDualMode`, `loopDualSequencer`, `advanceDualSequencers`, `setDualLengthAtCursor` |
| Chords and release interlock | `handleRuntimeChords`, `syncRuntimeInputs`, `serviceRuntimeRelease` |
| Buttons and clock | `handleInputs`, `updateButtons`, `updateClockMode`, `serviceExternalClock` |
| Output events | `startPulse`, `servicePulses` |
| Musical display | `getCellDisp`, `setCellDisp`, `refreshDisplayAtCursor` |
| Games | Search the `snake`, `life` and `handheld` function families |

The musical cursor always represents the next step. Keep display transforms consistent with physical matrix orientation. Musical evolution and arcade Life intentionally use different boundary/regeneration rules.

## Tests and resource constraints

```sh
python3 tools/test_host.py
```

The runner compiles the actual sketch with simulated Arduino/LedControl I/O under Clang C++11, ASan and UBSan. It runs musical, handheld and rules/dual suites for both input profiles; temporary binaries are removed automatically. Set `CXX` to select another compatible compiler.

Keep the firmware non-blocking. Use rollover-safe unsigned timing; preserve the independent command/game debounce paths, output pulse service, boot-only game selection and runtime GoL/Dual switching. Game modes must leave all three musical outputs low. The sketch uses static buffers, with no EEPROM or dynamic allocation. Rebuild AVR and check static SRAM/stack after changes.

The rule table is Classic B3/S23, SINCRIA B234/S01, B3/S234, Mazectric B3/S1234 and Coagulations B378/S235678. Musical boundary handling remains unchanged; arcade Life stays B3/S23 and fully toroidal. Rule changes reset musical history. Explicit clearing holds the empty field until an actual cell creation or random generation.

Dual uses two 32-bit patterns, independent lengths and playheads, with the existing clock/pulse infrastructure. A4 remains clock-only in the new musical screens; ROTATE supplies rightward navigation. The games retain A4 for rightward movement.

Validate all 64 display positions, scan order, edits and rotations, short pattern detection, clock switching, pulses/retrigger, game boot combinations, Snake collision/wrapping and Life pause/editing. Also validate combo order/bounce, menu confirmation, empty-field hold, independent lengths, dual gate routing and source-clock changes. Physical validation covers supply routing, input polarity, real switch bounce, output loading and worst-case stack.

See [validation results](PUBLICATION_CHECKS.md), [game controls](EUROBOY.md) and [contribution guidelines](../CONTRIBUTING.md).
