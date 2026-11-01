# Build and modify SINCRIA

Current sketch: `firmware/SINCRIA_Scansione_Righe/SINCRIA_Scansione_Righe.ino`, Handheld Games firmware dated 28 October 2026. Target: classic Arduino Nano, **ATmega328P / 16 MHz**.

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
| Musical automaton | `computeNextGenerationInto`, `countLiveNeighborsIn`, history and oscillation helpers |
| Buttons and clock | `handleInputs`, `updateButtons`, `updateClockMode`, `serviceExternalClock` |
| Output events | `startPulse`, `servicePulses` |
| Musical display | `getCellDisp`, `setCellDisp`, `refreshDisplayAtCursor` |
| Games | Search the `snake`, `life` and `handheld` function families |

The musical cursor always represents the next step. Keep display transforms consistent with physical matrix orientation. Musical evolution and arcade Life intentionally use different boundary/regeneration rules.

## Tests and resource constraints

```sh
python3 tools/test_host.py
```

The runner compiles the actual sketch with simulated Arduino/LedControl I/O under Clang C++11, ASan and UBSan. It runs musical and handheld suites for both input profiles; temporary binaries are removed automatically. Set `CXX` to select another compatible compiler.

Keep the firmware non-blocking. Use rollover-safe unsigned timing; preserve the independent command/game debounce paths, output pulse service and boot-only mode selection. Game modes must leave all three musical outputs low. The sketch uses static buffers, with no EEPROM or dynamic allocation. Rebuild AVR and check static SRAM/stack after changes.

Validate all 64 display positions, scan order, edits and rotations, short pattern detection, clock switching, pulses/retrigger, game boot combinations, Snake collision/wrapping and Life pause/editing. Physical validation covers supply routing, input polarity, real switch bounce, output loading and worst-case stack.

See [validation results](PUBLICATION_CHECKS.md), [game controls](EUROBOY.md) and [contribution guidelines](../CONTRIBUTING.md).
