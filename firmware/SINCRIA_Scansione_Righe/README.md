# SINCRIA - GoL rules and Dual Gate Sequencer

Firmware dated **3 November 2026**, published with updated documentation on **10 November 2026**. The project owner has confirmed successful operation on the module.

Classic Arduino Nano ATmega328P, 16 MHz; LedControl 1.0.6. Open the .ino in Arduino IDE and select the processor/bootloader fitted to the Nano. Both Nano bootloader configurations compile to the same application HEX; the HEX here contains the application, not a bootloader.

Five GoL rules, persistent empty-field start, two independent 1-32 step gate sequences and the two EUROBOY games are included. See the manuals for the control combinations.

[English guide](../../docs/USER_GUIDE.md) · [Italian guide](../../docs/USER_GUIDE_IT.md) · [English PDF](../../docs/SINCRIA-User-Manual-EN.pdf) · [Italian PDF](../../docs/SINCRIA-Manuale-Utente-IT.pdf) · [EUROBOY](../../docs/EUROBOY.md) · [Build and tests](../../docs/DEVELOPMENT.md)

SHA256SUMS.txt identifies the source and application HEX. Tests include the actual sketch with simulated Arduino/LedControl I/O.
