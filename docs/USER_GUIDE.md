# SINCRIA user guide

This guide describes the published `SINCRIA_Rev2_OK` sketch, not unimplemented panel possibilities. The [updated schematic PDF](../hardware/schematics/README.md) supplied on 28 October 2026 is available. Electrical input/output limits and compatibility with a particular assembled PCB still require checking and measurement.

## First patch

Select internal clock and adjust Tempo. It maps the pot to 60–200 BPM; step interval is integer `15000 / BPM` milliseconds, a sixteenth note. A clock event is generated every step even when its cell is dead. The live-cell signal is a separate 10 ms event when the scanned cell is alive. Connect outputs only after confirming the board's jack conditioning, polarity and levels.

Select external clock to advance once per debounced rising edge on D7. The input must stay stable more than 5 ms; short triggers can be missed. External stepping is one event per step, with no independent multiplication or swing. The Tempo pot is still read but does not drive external stepping.

The highlighted cell is inverted temporarily on the display. A highlighted dark-looking cell can therefore be alive; this is scan feedback, not a second state layer.

## Shape the world

NewGame randomises all eight row bytes. Lock toggles evolution: it retains the current world while the scan continues. Rotate turns the pattern clockwise. Spawn selects one currently dead cell and makes it live; Kill selects one live cell and removes it. Full/empty worlds simply offer no candidate for the corresponding action.

Hold AltMode about **200 ms** to toggle alternate controls. In alternate mode NewGame restarts the scan, Lock shifts cells left with horizontal wrap, and Rotate shifts rows upwards with wrap. With external clock selected, holding Spawn/Kill writes/erases at successive scan positions. The alternative is not a second stored pattern bank.

## Evolution

The rules are Conway B3/S23, with horizontal wrapping only. Generation evolution is evaluated at step 63; `currentStep` advances before cell reading, so the first normal tick after startup reads step 1. A blank next generation causes pre-emptive reseeding; empty and stationary current generations also trigger regeneration at a scan boundary. The stationary test compares only the immediately preceding generation; it is not a general detector of period-2 or longer oscillators. GAME END targets a 10 ms marker at regeneration.

## Known limitations

- There is no dedicated `GateIn` input handler in the supplied sketch, although the photographed panel has that label. Do not promise a GateIn action without a code/hardware update.
- `LIVE_CELL_LED_PIN` D3 and `GAME_END_LED_PIN` D4 drive the software event paths. How each is connected to a physical gate jack, and any output buffering/protection, must be confirmed from the corrected PCB schematic.
- There is no pattern EEPROM/flash persistence, MIDI or pitch CV DAC in this source. Power-on generates a new random world using an ADC seed; repeatability/entropy is not guaranteed.
- Inputs use `INPUT` with external pulldowns assumed. Replacing this with `INPUT_PULLUP` changes polarity and wiring expectations.
- Ordinary controls use about 50 ms debounce; the clock uses more than 5 ms stable level; alternate-mode hold is 200 ms.
- Timing uses polling and `millis()`. LED-matrix writes and control handling add scheduling delay. Ten milliseconds is a requested duration, not a measured pulse guarantee.
- `shiftCellsLeft()` shifts row bits. Visual direction depends on matrix orientation/bit order; confirm against the fitted display.
- Compatibility with the latest REV2.2 PCB and all electrical limits requires a physical check. This guide does not silently patch the existing source.
