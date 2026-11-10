# Changelog

## GoL rules + Dual Gate Sequencer - firmware 3 November 2026

Published 10 November 2026 after successful testing reported by the project owner.

- Added five musical GoL rules: Classic B3/S23, SINCRIA B234/S01, B3/S234, Mazectric B3/S1234 and Coagulations B378/S235678.
- ALT+NEW GAME opens the five-LED selector; NEW GAME confirms and randomises.
- ALT+KILL held in the selector applies the rule and clears the colony. The matrix, including cursor, stays dark until cells are created or a random colony is requested.
- ALT+ROTATE switches between musical GoL and a dual gate sequencer. GoL state and dual patterns remain separate.
- Dual provides two 1-32 step patterns, upper/lower matrix halves, shared INT/EXT clock and independent lengths/playheads. GateOut carries sequence 1; GameEnd carries sequence 2.
- Dual controls: NEW GAME toggles a step; ALT+LOCK play/pause; ALT+SPAWN sets the endpoint at the cursor; ALT+NEW GAME resets both playheads.
- ROTATE is rightward navigation in the new musical screens, reserving GATE IN for clock. The two games retain their earlier control mapping.
- Chord handling preserves the previous musical ALT latch and consumes entry/confirmation presses through a release interlock.
- Existing scan orientation, input profile, musical timing and both EUROBOY games are preserved.
- Updated English and Italian user guides, PDF manuals, developer instructions, current HEX, checksums and host-test runner.

The source uploaded with this publication is the owner-tested file, without additional firmware edits. [Validation](PUBLICATION_CHECKS.md).
