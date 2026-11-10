# Firmware validation

Current firmware: **GoL rules + Dual Gate Sequencer, 3 November 2026**. Publication checks: **10 November 2026**. Target: classic Arduino Nano ATmega328P, 16 MHz, Arduino AVR Boards 1.8.6 and LedControl 1.0.6.

The project owner reported successful testing of this revision on the module on 10 November 2026. The results below are the reproducible build and simulated-I/O checks; no additional electrical measurements were performed for this publication.

## Published package checks

The published source is byte-for-byte identical to the firmware confirmed by the owner. SHA-256: `573f99bfd9ff26ea6c6b0ee718f7232e3f48f60cf9cb507a7c48c6378793c272`.

| Build or suite | Result |
| --- | --- |
| Nano ATmega328P | Pass: 12,182 bytes Flash; 488 bytes static SRAM |
| Nano ATmega328P Old Bootloader | Pass: 12,182 bytes Flash; 488 bytes static SRAM |
| Musical suite, profile 1 | 6,358 assertions |
| Arcade suite, profile 1 | 79,112 assertions |
| Rules/Dual suite, profile 1 | 10,509 assertions |
| Musical suite, profile 0 | 6,358 assertions |
| Arcade suite, profile 0 | 79,112 assertions |
| Rules/Dual suite, profile 0 | 10,509 assertions |

Total: **191,958 assertions**. Both AVR builds use the default input profile 1 and produce the same application HEX, SHA-256 `75b518e5ee53c3e815203008c09531c57192d4a65b34694691ec5b68f036dac0`. Static SRAM leaves 1,560 bytes for stack/local use; it is not a measured maximum-stack result. No sketch warnings were emitted in these publication builds.

Run `python3 tools/test_host.py` from repository root. Clang C++11 compiles the actual sketch with simulated Arduino/LedControl I/O, AddressSanitizer and UndefinedBehaviorSanitizer. Temporary host binaries are removed automatically.

## Coverage

- Original musical scan/display mapping, edits, ALT controls, transformations, LOCK, pattern history and regeneration.
- Internal/external clock, fast command filters, pulse expiry/retrigger and rollover-safe timers.
- All five rule masks; all 512 local configurations per rule; 500 further boards per rule against an independent reference.
- Musical rule selection independent of fully toroidal, classic arcade Life.
- ALT chord order, early ALT press, bounce and held controls without duplicate actions or unwanted latch changes.
- Five-LED menu, clock input excluded from navigation, confirmation without click-through and reset/history handling.
- Long clear command and 128 subsequent clocks with no random repopulation, LIVE or END; first actual cell creation releases the hold.
- Every dual display position, lengths 1-32, hidden-but-preserved steps, editing during playback and separate GoL state.
- 150 clocks with lengths 3 and 5: independent playheads and gate values checked at each tick.
- Dual pause/resume with an already-active clock, common reset, 10 ms pulses, mode changes and timer rollover.
- Existing game boot combinations, release handling, Snake wrapping/collision/growth/victory and interactive Life editing/pause/evolution.
- Musical outputs remain low throughout arcade operation.

Hardware details, polarity and supply routing are documented in the [schematic](../hardware/schematics/README.md). The owner report does not replace measurements of levels, loading or worst-case stack under every patch.
