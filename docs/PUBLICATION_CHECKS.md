# Handheld Games validation

Target: Arduino Nano ATmega328P, 16 MHz; Arduino AVR Boards 1.8.6; LedControl 1.0.6.

The current validation includes AVR compilation for both Nano bootloader configurations and four host suites covering musical and arcade operation under both input profiles. Run `python3 tools/test_host.py` to reproduce the host tests.

## Coverage

- All 64 scan/display positions, cursor mapping, edits, transformations and LOCK behavior.
- Conway rules, exact short-cycle detection and automatic musical regeneration.
- Internal/external clock, command filtering, pulse overlap and timer rollover.
- All eight ALT/SPAWN/KILL start-up combinations, selection qualification, entry animation and release handling.
- Snake steering, collision, departing-tail handling, wrapping, growth, food placement and full-field victory.
- Toroidal arcade Life, cursor-only inversion, editing, pause/resume and independent reference grids.
- Musical outputs remain low during game entry, play and end states.

Host suites use the actual sketch with simulated electrical I/O, AddressSanitizer and UndefinedBehaviorSanitizer. They do not measure the physical board. Hardware qualification includes power distribution, switch behavior, input levels, matrix orientation, output loading and maximum stack use.

## Results - 1 November 2026

| Build or suite | Result |
| --- | --- |
| Nano ATmega328P | Pass: 8,966 bytes flash; 435 bytes static SRAM |
| Nano ATmega328P Old Bootloader | Pass: 8,966 bytes flash; 435 bytes static SRAM |
| Musical suite, input profile 1 | Pass: 6,358 assertions |
| Arcade suite, input profile 1 | Pass: 79,112 assertions |
| Musical suite, input profile 0 | Pass: 6,358 assertions |
| Arcade suite, input profile 0 | Pass: 79,112 assertions |

Total: **170,940 assertions**. Both AVR builds produce the same application HEX, SHA-256 `a55111d2db317e1b19182100608fbd3bd8d66e621e11bc0cfbc99ebac96bb331`. The only compiler warnings are unused `tag` parameters in Arduino core `new.cpp`. Static SRAM excludes runtime stack.
