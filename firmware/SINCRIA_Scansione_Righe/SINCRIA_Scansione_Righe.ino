/*
  S'INCRIA - Regole GoL + Dual Gate Sequencer, 3 novembre 2026
  Base corrente Handheld Games conservata; giochi al boot invariati.
  ALT+NEW: regole GoL. ALT+ROTATE: GoL / Dual. ALT+LOCK in Dual: play/pausa.
  Dual: LOCK su, KILL giu', SPAWN sinistra, ROTATE destra, NEW toggle.
  ALT+SPAWN: lunghezza fino al cursore; ALT+NEW in Dual: reset playhead.
  Nel menu regole ALT+KILL lungo 1s: applica regola, vuota e torna al GoL.
  Base: firmware corrente SINCRIA_Scansione_Righe, scansione e display invariati.
  Boot ALT+SPAWN: Snake; ALT+KILL: Life interattivo; entrambe: Eurorack.
  Boot qualificato 140..240 ms; giochi con animazione 320 ms e attesa rilascio.
  Giochi: LOCK su, KILL giu', SPAWN sinistra, GATE IN destra.
  Life: NEW crea, ROTATE cancella; INT play / EXT pausa; campo toroidale.
  Pot: Snake 700..100 ms; Life 2000..100 ms. Nessuna uscita nei giochi.
  Ingressi jack e pulsanti condivisi: un gate e' indistinguibile da un tasto.
  NOTA USB: negli schemi Rev2.2/2.3 il MAX usa +5V dal regolatore Eurorack,
  separato dai +5V USB del Nano. L'uso solo USB richiede che il montaggio
  alimenti gia' anche matrice e controlli; il firmware non unisce queste reti.
  Derivato dall'allegato effettivo SINCRIA_Rev2_OK.ino (17781 byte).
  Arduino Nano CLASSICO, ATmega328P, 16 MHz / 5 V; LedControl 1.0.6.

  PIN: D2 NEW, D3 LIVE gate+LED, D4 END gate+LED, D5 CLOCK gate+LED,
       D6 LOCK, D7 ROTATE, D8 SPAWN, D9 ALT,
       D10 MAX CLK, D11 MAX CS, D12 MAX DIN,
       A0 pot, A1 INT switch, A2 EXT switch,
       A3 KILL, A4 CLOCK IN (GATE IN), A5 ALT LED.
  D0/D1, D13, A6/A7 non utilizzati. Tre gate distinti conservati.
  Profilo 0: Rev2 giugno, comandi/clock ATTIVI ALTI con pulldown esterni.
  Profilo 1: Rev2.2/2.3, sei ingressi a transistor ATTIVI BASSI.
  ALT e selettore clock sono ATTIVI ALTI in entrambi i profili.
  CLOCK IN deve essere gia' condizionato per un ingresso logico 0..5 V.
  I jack gate richiedono gli stadi di uscita previsti dall'hardware.

  Scansione predefinita: riga per riga, da SX a DX, poi dall'alto in basso.
  Coordinate registri: display(r,c)=(7-step%8, 7-step/8).
  Sul montaggio provato, le righe fisiche sono le colonne dei registri.
  Percorso fisico: prima riga SX->DX, poi riga sotto. Opzione per colonne sotto.
  Il cursore invertito indica SEMPRE la cella del PROSSIMO clock.
  Ogni tick: editing, lettura/gate, avanzamento, display del prossimo step.
  Dopo lo step 63: evoluzione/rigenerazione, salvo LOCK.
  GoL musicale: regola selezionata, default B3/S23; wrap solo sulle colonne.
  Mapping originale mantenuto: display(r,c) -> griglia(c,7-r).

  ALT lungo da solo: toggle dopo 200 ms debounced (debounce ALT 50 ms).
  Comandi NEW/LOCK/ROTATE/SPAWN/KILL: filtro 2 ms, riattivazione minima 30 ms.
  Accetta gate brevi senza richiedere 50 ms alti; tasto tenuto = un solo evento.
  Pulsante e jack condividono lo stesso ingresso: il filtro e' un compromesso
  tra reattivita' ai trigger e soppressione del rimbalzo meccanico.
  Normale: NEW random+reset, LOCK toggle, ROTATE 90 gradi,
           SPAWN/KILL una cella casuale per pressione, niente autorepeat.
  ALT: NEW reset+preview LIVE se viva (senza CLOCK), LOCK shift grid bits,
       ROTATE shift grid righe; con EXT, SPAWN/KILL editano il cursore
       e dipingono al clock se tenuti. KILL prevale su SPAWN simultaneo.
  Le trasformazioni mantengono il verso del firmware originale.

  LOCK congela ANCHE la rigenerazione automatica; editing/NEW restano attivi.
  Periodi 1..4: confronto ESATTO di griglie, nessuna firma/hash.
  La ripetizione rilevata viene scansionata per un intero giro prima di END.
  Previsione estinzione: rigenera senza scansionare una generazione vuota.

  CLOCK interno: 30..300 BPM, 4 step/beat, intervallo 15000/BPM ms.
  CLOCK esterno: fronte alto dopo 5 ms stabili, basso stabile per riarmare.
  Al passaggio a EXT un livello gia' alto non produce un tick fantasma.
  Impulsi in uscita: nominalmente 10 ms, polling non bloccante.
  Retrigger prima della scadenza estende l'impulso (eventi ravvicinati
  possono fondersi). Non e' un ingresso per clock audio o impulsi <5 ms.
  All'avvio selettore valido letto subito; invalido -> EXT in attesa.
  In marcia stato invalido/intermedio mantiene la modalita' corrente.
  Nessun uso di Serial, EEPROM, delay(), interrupt custom o heap nel firmware.
*/

// 0 = schema Rev2 16/06/2026; 1 = ingressi invertenti Q1..Q6 Rev2.2/2.3.
#ifndef SINCRIA_INVERTED_INPUTS
#define SINCRIA_INVERTED_INPUTS 1
#endif
#if SINCRIA_INVERTED_INPUTS != 0 && SINCRIA_INVERTED_INPUTS != 1
#error "SINCRIA_INVERTED_INPUTS deve essere 0 oppure 1."
#endif

// 1: riga per riga SX->DX e dall'alto in basso (predefinito).
// 0: prima ogni colonna dall'alto in basso, poi la colonna a destra.
#ifndef SINCRIA_SCAN_BY_ROWS
#define SINCRIA_SCAN_BY_ROWS 1
#endif
#if SINCRIA_SCAN_BY_ROWS != 0 && SINCRIA_SCAN_BY_ROWS != 1
#error "SINCRIA_SCAN_BY_ROWS deve essere 0 oppure 1."
#endif

#include <Arduino.h>
#include <LedControl.h>
#include <string.h>

#if !defined(__AVR_ATmega328P__) || !defined(ARDUINO_AVR_NANO)
#error "Selezionare Arduino Nano classico con processore ATmega328P."
#endif

const uint8_t DIN_PIN = 12;
const uint8_t CLK_PIN = 10;
const uint8_t CS_PIN = 11;
LedControl lc(DIN_PIN, CLK_PIN, CS_PIN, 1);

const uint8_t POT_PIN = A0;
const uint8_t SWITCH_INTERNAL_CLOCK_PIN = A1;
const uint8_t SWITCH_EXTERNAL_CLOCK_PIN = A2;
const uint8_t NEW_GAME_PIN = 2;
const uint8_t LIVE_CELL_OUT_PIN = 3;
const uint8_t GAME_END_OUT_PIN = 4;
const uint8_t CLOCK_OUT_PIN = 5;
const uint8_t LOCK_GENERATION_PIN = 6;
const uint8_t CLOCK_IN_PIN = A4; // scritta GATE IN: prima eseguiva KILL
const uint8_t ROTATE_GENERATION_PIN = 7; // scritta ROTATE: prima era CLOCK IN
const uint8_t ALT_MODES_PIN = 9;
const uint8_t SPAWN_CELL_PIN = 8; // scritta SPAWN: prima eseguiva ROTATE
const uint8_t KILL_CELL_PIN = A3; // scritta KILL: prima eseguiva SPAWN
const uint8_t ALT_MODE_LED_PIN = A5;

const uint32_t ALT_DEBOUNCE_MS = 50;
const uint32_t COMMAND_FILTER_MS = 2;
const uint32_t COMMAND_RETRIGGER_MS = 30;
const uint32_t CLOCK_SWITCH_DEBOUNCE_MS = 50;
const uint32_t CLOCK_IN_FILTER_MS = 5;
const uint32_t PULSE_MS = 10;
const uint32_t ALT_LONG_PRESS_MS = 200;
const uint32_t POT_SAMPLE_MS = 20;
const uint8_t DISPLAY_INTENSITY = 2;

enum ClockMode : uint8_t { CLOCK_INTERNAL, CLOCK_EXTERNAL };
enum ButtonIndex : uint8_t { BTN_NEW, BTN_LOCK, BTN_ROTATE, BTN_SPAWN,
                             BTN_KILL, BTN_ALT, BUTTON_COUNT };
enum PulseIndex : uint8_t { PULSE_CLOCK, PULSE_LIVE, PULSE_END, PULSE_COUNT };

const uint8_t buttonPins[BUTTON_COUNT] = {
  NEW_GAME_PIN, LOCK_GENERATION_PIN, ROTATE_GENERATION_PIN,
  SPAWN_CELL_PIN, KILL_CELL_PIN, ALT_MODES_PIN
};
const uint8_t pulsePins[PULSE_COUNT] = {
  CLOCK_OUT_PIN, LIVE_CELL_OUT_PIN, GAME_END_OUT_PIN
};
bool buttonRaw[BUTTON_COUNT] = {};
bool buttonDown[BUTTON_COUNT] = {};
bool buttonPressed[BUTTON_COUNT] = {};
uint32_t buttonChangedAt[BUTTON_COUNT] = {};
uint32_t buttonAcceptedAt[BUTTON_COUNT] = {};
bool buttonHasTriggered[BUTTON_COUNT] = {};
bool pulseActive[PULSE_COUNT] = {};
uint32_t pulseStartedAt[PULSE_COUNT] = {};

byte currentGen[8] = {};
byte nextGen[8] = {};
byte history[4][8] = {}; // [0]=generazione precedente, ... [3]=gen-4
uint8_t historyCount = 0;
uint8_t oscillationPeriod = 0;
uint8_t currentStep = 0; // prossimo step da suonare e cella visibile/editabile
bool gameIsOver = false;
bool generationLocked = false;
bool altControlsActive = false;
uint32_t altPressedAt = 0;
bool altLongHandled = false;

uint8_t clockMode = CLOCK_EXTERNAL;
int8_t switchCandidate = -1;
uint32_t switchChangedAt = 0;
uint32_t lastStepTime = 0;
uint16_t BPM = 60;
uint16_t stepInterval = 250;
uint32_t lastPotSample = 0;
bool externalRaw = false;
bool externalStable = false;
bool externalArmed = false;
uint32_t externalChangedAt = 0;

byte displayedRows[8] = {};
bool displayDirty = true;
bool displayCacheValid = false;

// Handheld: selezione solo al boot, senza rilevamento dell'alimentazione.
enum OperatingMode : uint8_t { MODE_EURORACK, MODE_SNAKE, MODE_GAME_OF_LIFE, MODE_DUAL };
OperatingMode operatingMode = MODE_EURORACK;
bool bootSelecting = true;
uint32_t bootStartedAt = 0;
uint32_t bootChangedAt = 0;
uint8_t bootCandidate = 0xFF;
const uint16_t BOOT_SETTLE_MS = 40;
const uint16_t BOOT_MIN_MS = 140;
const uint16_t BOOT_MAX_MS = 240;
const uint16_t HANDHELD_DEBOUNCE_MS = 30;
const uint16_t HANDHELD_ENTRY_MS = 320;
const uint16_t HANDHELD_FLASH_MS = 600;

// Debounce separato per i giochi: non cambia il filtro veloce dei jack Eurorack.
enum HandheldButton : uint8_t { HH_UP, HH_DOWN, HH_LEFT, HH_RIGHT,
                                HH_CREATE, HH_ERASE, HH_BUTTON_COUNT };
const uint8_t handheldPins[HH_BUTTON_COUNT] = {
  LOCK_GENERATION_PIN, KILL_CELL_PIN, SPAWN_CELL_PIN, CLOCK_IN_PIN,
  NEW_GAME_PIN, ROTATE_GENERATION_PIN
};
struct HandheldInput {
  uint32_t changedAt;
  bool raw;
  bool down;
  bool pressed;
};
HandheldInput handheldInputs[HH_BUTTON_COUNT] = {};
enum HandheldPhase : uint8_t { HH_ENTRY, HH_WAIT_RELEASE, HH_PLAY,
                               HH_LOST, HH_WON };
HandheldPhase handheldPhase = HH_ENTRY;
uint32_t handheldPhaseAt = 0;
uint32_t handheldReleaseAt = 0;
bool handheldReleaseTracking = false;

// Coordinate fisiche: x verso destra, y verso il basso; posizione = y*8+x.
enum SnakeDirection : uint8_t { SNAKE_UP, SNAKE_RIGHT, SNAKE_DOWN, SNAKE_LEFT };
uint8_t snakeBody[64] = {};
uint8_t snakeLength = 3;
uint8_t snakeDirection = SNAKE_RIGHT;
uint8_t snakeNextDirection = SNAKE_RIGHT;
uint8_t snakeFood = 64; // 64 = nessun cibo, matrice piena
bool snakeTurnQueued = false;
uint8_t lifeCursorX = 3;
uint8_t lifeCursorY = 3;
bool lifeCursorInverted = true;
uint32_t lifeCursorAt = 0;


// Regole selezionabili: B/S = maschere dei conteggi 0..8, nessuna stringa in RAM.
enum GolRule : uint8_t { RULE_CLASSIC, RULE_SINCRIA, RULE_SURVIVAL_234,
                         RULE_MAZECTRIC, RULE_COAGULATIONS, GOL_RULE_COUNT };
uint8_t golRuleIndex = RULE_CLASSIC;
bool holdEmptyGrid = false;
bool ruleMenuActive = false;
uint8_t ruleMenuCandidate = RULE_CLASSIC;
uint32_t ruleMenuBlinkAt = 0;
bool menuClearTracking = false;
uint32_t menuClearAt = 0;
const uint16_t MENU_CLEAR_HOLD_MS = 1000;

// Interlock di rilascio per cambi schermata/modalita'; il CLOCK IN e' escluso.
bool runtimeReleaseRequired = false;
bool runtimeReleaseTracking = false;
uint32_t runtimeReleaseAt = 0;
uint8_t runtimeChordKey = BUTTON_COUNT;
bool runtimeAltConsumed = false;
bool altStateAtPress = false;

// Due pattern di gate da 32 bit, indipendenti dalla colonia GoL.
uint32_t dualPattern[2] = {};
uint8_t dualLength[2] = {32, 32};
uint8_t dualPosition[2] = {};
uint8_t dualCursorX = 0;
uint8_t dualCursorY = 0;
bool dualRunning = false;
uint32_t dualCursorAt = 0;
bool dualLengthPreview = false;
uint32_t dualLengthPreviewAt = 0;

// Dichiarazioni esplicite anche per il banco C++ nativo.
bool handleRuntimeChords(uint32_t now);
void advanceDualSequencers(uint32_t now);
void loopRuleMenu(uint32_t now);
void loopDualSequencer(uint32_t now);
void renderRuleMenu(uint32_t now);
void renderDualSequencer(uint32_t now);
bool serviceRuntimeRelease(uint32_t now);

uint16_t golBirthMask(uint8_t rule) {
  switch (rule) {
    case RULE_SINCRIA: return (1U << 2) | (1U << 3) | (1U << 4);
    case RULE_COAGULATIONS: return (1U << 3) | (1U << 7) | (1U << 8);
    default: return 1U << 3;
  }
}

uint16_t golSurvivalMask(uint8_t rule) {
  switch (rule) {
    case RULE_SINCRIA: return (1U << 0) | (1U << 1);
    case RULE_SURVIVAL_234: return (1U << 2) | (1U << 3) | (1U << 4);
    case RULE_MAZECTRIC: return (1U << 1) | (1U << 2) | (1U << 3) | (1U << 4);
    case RULE_COAGULATIONS:
      return (1U << 2) | (1U << 3) | (1U << 5) | (1U << 6) | (1U << 7) | (1U << 8);
    default: return (1U << 2) | (1U << 3);
  }
}

bool golRuleResult(bool alive, uint8_t neighbors, uint8_t rule) {
  return ((alive ? golSurvivalMask(rule) : golBirthMask(rule)) & (1U << neighbors)) != 0;
}

// Normalizzazione del livello elettrico: gli algoritmi usano true=attivo.
// Il profilo transistor riguarda NEW/LOCK/ROTATE/SPAWN/KILL e CLOCK IN.
// ALT e selettore non passano da questi transistor e non vanno invertiti.
bool readCommandActive(uint8_t pin) {
  return digitalRead(pin) == (SINCRIA_INVERTED_INPUTS ? LOW : HIGH);
}

bool readButtonActive(uint8_t index) {
  if (index == BTN_ALT) return digitalRead(buttonPins[index]) == HIGH;
  return readCommandActive(buttonPins[index]);
}

// Conferma sul dispositivo: (registro r=7,c=7) e' l'angolo in alto a SX.
// Diminuire r sposta il cursore a DESTRA; diminuire c lo sposta in BASSO.
// Quindi, per righe fisiche, cambia prima r e ogni 8 step cambia c.
// Compensa il solo percorso: disegno e trasformazioni del gioco restano uguali.
uint8_t getCurrentRow() { return 7 - (SINCRIA_SCAN_BY_ROWS ? currentStep % 8 : currentStep / 8); }
uint8_t getCurrentCol() { return 7 - (SINCRIA_SCAN_BY_ROWS ? currentStep / 8 : currentStep % 8); }

bool getCellDisp(uint8_t row, uint8_t col) {
  return bitRead(currentGen[col], 7 - row) != 0;
}

// Ritorna true solo se il contenuto cambia davvero.
bool setCellDisp(uint8_t row, uint8_t col, bool state) {
  if (getCellDisp(row, col) == state) return false;
  bitWrite(currentGen[col], 7 - row, state);
  return true;
}

bool gridsEqual(const byte a[8], const byte b[8]) {
  return memcmp(a, b, 8) == 0;
}

bool anyLiveCell(const byte grid[8]) {
  for (uint8_t row = 0; row < 8; ++row) {
    if (grid[row] != 0) return true;
  }
  return false;
}

void clearHistory() {
  memset(history, 0, sizeof(history));
  historyCount = 0;
  oscillationPeriod = 0;
}

void evaluateGameState() {
  gameIsOver = !anyLiveCell(currentGen);
  if (gameIsOver) oscillationPeriod = 0;
}

void markManualEdit() {
  if (anyLiveCell(currentGen)) holdEmptyGrid = false;
  clearHistory();
  evaluateGameState();
  displayDirty = true;
}

void pushHistory(const byte grid[8]) {
  for (uint8_t k = 3; k > 0; --k) memcpy(history[k], history[k - 1], 8);
  memcpy(history[0], grid, 8);
  if (historyCount < 4) ++historyCount;
}

uint8_t detectOscillationPeriod() {
  // Cerca prima il periodo minimo; uno still life non diventa periodo 2/4.
  for (uint8_t period = 1; period <= historyCount; ++period) {
    if (gridsEqual(currentGen, history[period - 1])) return period;
  }
  return 0;
}

uint8_t countLiveNeighborsIn(const byte grid[8], int8_t row, int8_t col) {
  uint8_t count = 0;
  for (int8_t dr = -1; dr <= 1; ++dr) {
    for (int8_t dc = -1; dc <= 1; ++dc) {
      if (dr == 0 && dc == 0) continue;
      const int8_t rr = row + dr;
      if (rr < 0 || rr > 7) continue; // nessun wrap verticale nella griglia
      const uint8_t cc = (col + dc + 8) % 8;
      if (bitRead(grid[rr], cc)) ++count;
    }
  }
  return count;
}

void computeNextGenerationInto(const byte src[8], byte dst[8]) {
  // src e dst devono essere distinti: aggiornamento simultaneo delle celle.
  for (uint8_t row = 0; row < 8; ++row) {
    byte value = 0;
    for (uint8_t col = 0; col < 8; ++col) {
      const uint8_t neighbors = countLiveNeighborsIn(src, row, col);
      if (golRuleResult(bitRead(src[row], col) != 0, neighbors, golRuleIndex)) {
        bitSet(value, col);
      }
    }
    dst[row] = value;
  }
}

void generateRandomGeneration() {
  // Non lasciare mai una nuova partita completamente vuota.
  do {
    for (uint8_t row = 0; row < 8; ++row) currentGen[row] = (byte)random(256L);
  } while (!anyLiveCell(currentGen));
  markManualEdit();
}

void startPulse(uint8_t index, uint32_t now) {
  digitalWrite(pulsePins[index], HIGH);
  pulseActive[index] = true;
  pulseStartedAt[index] = now;
}

void servicePulses(uint32_t now) {
  for (uint8_t i = 0; i < PULSE_COUNT; ++i) {
    if (pulseActive[i] && (uint32_t)(now - pulseStartedAt[i]) >= PULSE_MS) {
      digitalWrite(pulsePins[i], LOW);
      pulseActive[i] = false;
    }
  }
}

void refreshDisplayAtCursor() {
  if (!displayDirty) return;
  for (uint8_t row = 0; row < 8; ++row) {
    byte value = 0;
    for (uint8_t col = 0; col < 8; ++col) {
      bool state = getCellDisp(row, col);
      if (!holdEmptyGrid && row == getCurrentRow() && col == getCurrentCol()) state = !state;
      if (state) bitSet(value, col); // stessa disposizione bit della Rev2
    }
    if (!displayCacheValid || displayedRows[row] != value) {
      lc.setRow(0, row, value);
      displayedRows[row] = value;
    }
  }
  displayCacheValid = true;
  displayDirty = false;
}

void rotateGeneration() {
  byte rotated[8] = {};
  for (uint8_t row = 0; row < 8; ++row) {
    for (uint8_t col = 0; col < 8; ++col) {
      if (bitRead(currentGen[7 - col], row)) bitSet(rotated[row], col);
    }
  }
  if (!gridsEqual(currentGen, rotated)) {
    memcpy(currentGen, rotated, 8);
    markManualEdit();
  }
}

void shiftCellsRight() {
  // Nome e trasformazione della Rev2: shift circolare dei bit verso il basso.
  bool changed = false;
  for (uint8_t row = 0; row < 8; ++row) {
    const byte old = currentGen[row];
    currentGen[row] = (byte)((old >> 1) | ((old & 1) << 7));
    if (old != currentGen[row]) changed = true;
  }
  if (changed) markManualEdit();
}

void shiftCellsUp() {
  // Nome e trasformazione della Rev2: riga 1->0, ..., riga 0->7.
  byte old[8];
  memcpy(old, currentGen, 8);
  for (uint8_t row = 0; row < 7; ++row) currentGen[row] = old[row + 1];
  currentGen[7] = old[0];
  if (!gridsEqual(old, currentGen)) markManualEdit();
}

void editRandomCell(bool alive) {
  // Estrazione uniforme tra le celle eleggibili senza array int[64] sullo stack.
  uint8_t count = 0;
  for (uint8_t pos = 0; pos < 64; ++pos) {
    if ((bitRead(currentGen[pos / 8], pos % 8) != 0) != alive) ++count;
  }
  if (count == 0) return;
  uint8_t chosen = (uint8_t)random((long)count);
  for (uint8_t pos = 0; pos < 64; ++pos) {
    if ((bitRead(currentGen[pos / 8], pos % 8) != 0) == alive) continue;
    if (chosen == 0) {
      bitWrite(currentGen[pos / 8], pos % 8, alive);
      markManualEdit();
      return;
    }
    --chosen;
  }
}

void paintCursor() {
  if (!altControlsActive || clockMode != CLOCK_EXTERNAL) return;
  // Legge lo stato debounced reale: nessun flag held che possa rimanere attivo.
  if (!buttonDown[BTN_SPAWN] && !buttonDown[BTN_KILL]) return;
  const bool alive = !buttonDown[BTN_KILL]; // KILL ha priorita'
  if (setCellDisp(getCurrentRow(), getCurrentCol(), alive)) markManualEdit();
}

void finishScan(uint32_t now) {
  if (holdEmptyGrid && !anyLiveCell(currentGen)) return;
  // LOCK ha priorita' su estinzione e oscillatori, ma non sugli edit manuali.
  if (generationLocked) return;
  evaluateGameState();
  if (gameIsOver || oscillationPeriod != 0) {
    generateRandomGeneration();
    startPulse(PULSE_END, now);
    return;
  }
  computeNextGenerationInto(currentGen, nextGen);
  if (!anyLiveCell(nextGen)) {
    generateRandomGeneration();
    startPulse(PULSE_END, now);
    return;
  }
  pushHistory(currentGen);
  memcpy(currentGen, nextGen, 8);
  oscillationPeriod = detectOscillationPeriod();
  evaluateGameState();
}

void advanceSequencer(uint32_t now) {
  paintCursor();
  startPulse(PULSE_CLOCK, now);
  if (getCellDisp(getCurrentRow(), getCurrentCol())) startPulse(PULSE_LIVE, now);
  if (currentStep == 63) {
    finishScan(now);
    currentStep = 0;
  } else {
    ++currentStep;
  }
  displayDirty = true;
}

void updateBPM() {
  BPM = (uint16_t)map(analogRead(POT_PIN), 0, 1023, 30, 300);
  stepInterval = 15000U / BPM;
}

int8_t readClockSwitch() {
  const bool internal = digitalRead(SWITCH_INTERNAL_CLOCK_PIN) == HIGH;
  const bool external = digitalRead(SWITCH_EXTERNAL_CLOCK_PIN) == HIGH;
  if (internal && !external) return CLOCK_INTERNAL;
  if (!internal && external) return CLOCK_EXTERNAL;
  return -1;
}

void resetExternalClock(uint32_t now) {
  externalRaw = readCommandActive(CLOCK_IN_PIN);
  externalStable = externalRaw;
  externalArmed = !externalRaw; // se gia' alto, attendi un basso stabile
  externalChangedAt = now;
}

void updateClockMode(uint32_t now) {
  const int8_t reading = readClockSwitch();
  if (reading != switchCandidate) {
    switchCandidate = reading;
    switchChangedAt = now;
  }
  // Gli stati invalidi azzerano anche la qualificazione del candidato.
  if (reading < 0 || (uint32_t)(now - switchChangedAt) < CLOCK_SWITCH_DEBOUNCE_MS) return;
  if ((uint8_t)reading != clockMode) {
    clockMode = (uint8_t)reading;
    lastStepTime = now; // INT riparte da un intervallo completo
    resetExternalClock(now);
  }
}

void updateButtons(uint32_t now) {
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    buttonPressed[i] = false;
    const bool reading = readButtonActive(i);
    if (reading != buttonRaw[i]) {
      buttonRaw[i] = reading;
      buttonChangedAt[i] = now;
    }
    const uint32_t filterMs = i == BTN_ALT ? ALT_DEBOUNCE_MS : COMMAND_FILTER_MS;
    if (reading != buttonDown[i] && (uint32_t)(now - buttonChangedAt[i]) >= filterMs) {
      buttonDown[i] = reading;
      // La salita logica e' rapida per riconoscere anche un trigger di 10 ms.
      // Un livello mantenuto non genera autorepeat. Le nuove attivazioni
      // ravvicinate vengono consumate senza produrre un evento differito.
      if (reading && (i == BTN_ALT || !buttonHasTriggered[i] ||
          (uint32_t)(now - buttonAcceptedAt[i]) >= COMMAND_RETRIGGER_MS)) {
        buttonPressed[i] = true;
        buttonHasTriggered[i] = true;
        buttonAcceptedAt[i] = now;
      }
    }
  }
}

void handleInputs(uint32_t now) {
  updateButtons(now);
  if (buttonPressed[BTN_ALT]) {
    altPressedAt = now;
    altLongHandled = false;
    altStateAtPress = altControlsActive;
  }
  if (handleRuntimeChords(now)) return;
  if (buttonDown[BTN_ALT] && !runtimeAltConsumed && !altLongHandled &&
      (uint32_t)(now - altPressedAt) >= ALT_LONG_PRESS_MS) {
    altLongHandled = true;
    altControlsActive = !altControlsActive;
    digitalWrite(ALT_MODE_LED_PIN, altControlsActive ? HIGH : LOW);
  }
  if (buttonPressed[BTN_NEW]) {
    currentStep = 0;
    lastStepTime = now;
    if (!altControlsActive) generateRandomGeneration();
    else if (getCellDisp(getCurrentRow(), getCurrentCol())) startPulse(PULSE_LIVE, now);
    displayDirty = true;
  }
  if (buttonPressed[BTN_LOCK]) {
    if (altControlsActive) shiftCellsRight();
    else generationLocked = !generationLocked;
  }
  if (buttonPressed[BTN_ROTATE]) {
    if (altControlsActive) shiftCellsUp();
    else rotateGeneration();
  }
  if (altControlsActive && clockMode == CLOCK_EXTERNAL) {
    // Edit immediato sul cursore; il mantenimento viene applicato a ogni tick.
    if (buttonPressed[BTN_SPAWN] || buttonPressed[BTN_KILL]) paintCursor();
  } else {
    if (buttonPressed[BTN_SPAWN]) editRandomCell(true);
    if (buttonPressed[BTN_KILL]) editRandomCell(false);
  }
}

void serviceExternalClock(uint32_t now) {
  const bool reading = readCommandActive(CLOCK_IN_PIN);
  if (reading != externalRaw) {
    externalRaw = reading;
    externalChangedAt = now;
  }
  if ((uint32_t)(now - externalChangedAt) < CLOCK_IN_FILTER_MS) return;
  if (externalStable == externalRaw) return;
  externalStable = externalRaw;
  if (!externalStable) externalArmed = true;
  else if (externalArmed) {
    externalArmed = false;
    if (operatingMode == MODE_DUAL) advanceDualSequencers(now);
    else advanceSequencer(now);
  }
}

void setup() {
  // Uscite basse prima di abilitarne la direzione.
  for (uint8_t i = 0; i < PULSE_COUNT; ++i) {
    digitalWrite(pulsePins[i], LOW);
    pinMode(pulsePins[i], OUTPUT);
  }
  digitalWrite(ALT_MODE_LED_PIN, LOW);
  pinMode(ALT_MODE_LED_PIN, OUTPUT);
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) pinMode(buttonPins[i], INPUT);
  pinMode(SWITCH_INTERNAL_CLOCK_PIN, INPUT);
  pinMode(SWITCH_EXTERNAL_CLOCK_PIN, INPUT);
  pinMode(CLOCK_IN_PIN, INPUT);

  lc.shutdown(0, false);
  lc.setIntensity(0, DISPLAY_INTENSITY);
  lc.clearDisplay(0);

  // Selezione non bloccante nei primi giri di loop; uscite gia' basse.
  bootStartedAt = millis();
  bootChangedAt = bootStartedAt;
  bootCandidate = 0xFF;
  bootSelecting = true;
}

// Inizializzazione musicale preesistente, spostata dopo la selezione al boot.
void initializeEurorack() {
  randomSeed((unsigned long)analogRead(POT_PIN) ^ micros());
  generateRandomGeneration();
  updateBPM();
  const uint32_t now = millis();
  switchCandidate = readClockSwitch();
  clockMode = switchCandidate < 0 ? (uint8_t)CLOCK_EXTERNAL : (uint8_t)switchCandidate;
  switchChangedAt = now;
  resetExternalClock(now);
  lastStepTime = now;
  lastPotSample = now;
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    buttonRaw[i] = readButtonActive(i);
    buttonChangedAt[i] = now;
  }
  refreshDisplayAtCursor();
}

void loopEurorack() {
  uint32_t now = millis();
  servicePulses(now); // spegne gli impulsi scaduti prima di un nuovo tick
  updateClockMode(now);
  handleInputs(now); // gli edit confermati precedono il clock dello stesso giro
  if (ruleMenuActive || operatingMode != MODE_EURORACK || runtimeReleaseRequired) return;
  if ((uint32_t)(now - lastPotSample) >= POT_SAMPLE_MS) {
    lastPotSample = now;
    updateBPM();
  }
  now = millis();
  servicePulses(now);
  if (clockMode == CLOCK_INTERNAL) {
    if ((uint32_t)(now - lastStepTime) >= stepInterval) {
      lastStepTime = now; // niente raffiche di recupero dopo un ritardo
      advanceSequencer(now);
    }
  } else {
    serviceExternalClock(now);
  }
  refreshDisplayAtCursor();
  servicePulses(millis());
}

// --- Handheld: nessuna chiamata a clock, gate o comandi musicali. ---
void updateHandheldInputs(uint32_t now) {
  for (uint8_t i = 0; i < HH_BUTTON_COUNT; ++i) {
    HandheldInput &input = handheldInputs[i];
    input.pressed = false;
    const bool reading = readCommandActive(handheldPins[i]);
    if (reading != input.raw) {
      input.raw = reading;
      input.changedAt = now;
    }
    if (reading != input.down &&
        (uint32_t)(now - input.changedAt) >= HANDHELD_DEBOUNCE_MS) {
      input.down = reading;
      input.pressed = reading; // solo fronte: nessun autorepeat
    }
  }
}

// Unico adattamento per i giochi, coerente col montaggio gia' confermato.
// La funzione di visualizzazione Eurorack rimane integralmente invariata.
void writeHandheldFrame(const byte frame[8]) {
  byte rows[8] = {};
  for (uint8_t y = 0; y < 8; ++y) {
    for (uint8_t x = 0; x < 8; ++x) {
      if (bitRead(frame[y], x)) bitSet(rows[7 - x], 7 - y);
    }
  }
  for (uint8_t row = 0; row < 8; ++row) {
    if (!displayCacheValid || displayedRows[row] != rows[row]) {
      lc.setRow(0, row, rows[row]);
      displayedRows[row] = rows[row];
    }
  }
  displayCacheValid = true;
}

void renderHandheldEntry(uint32_t elapsed) {
  byte frame[8] = {};
  const uint8_t inset = 3 - elapsed / 80; // chiamata solo per elapsed < 320
  for (uint8_t y = inset; y <= 7 - inset; ++y) {
    for (uint8_t x = inset; x <= 7 - inset; ++x) {
      if (y == inset || y == 7 - inset || x == inset || x == 7 - inset) {
        bitSet(frame[y], x);
      }
    }
  }
  writeHandheldFrame(frame);
}

void renderHandheld(bool blank) {
  byte frame[8] = {};
  if (!blank) {
    if (operatingMode == MODE_SNAKE) {
      for (uint8_t i = 0; i < snakeLength; ++i) {
        bitSet(frame[snakeBody[i] / 8], snakeBody[i] % 8);
      }
      if (snakeFood < 64) bitSet(frame[snakeFood / 8], snakeFood % 8);
    } else {
      // getCellDisp(7-x,7-y) = bit x di currentGen[7-y].
      for (uint8_t y = 0; y < 8; ++y) frame[y] = currentGen[7 - y];
      if (lifeCursorInverted) {
        frame[lifeCursorY] ^= (byte)(1U << lifeCursorX);
      }
    }
  }
  writeHandheldFrame(frame);
}

bool snakeOccupies(uint8_t position, uint8_t length) {
  for (uint8_t i = 0; i < length; ++i) {
    if (snakeBody[i] == position) return true;
  }
  return false;
}

void placeSnakeFood() {
  snakeFood = 64;
  if (snakeLength == 64) return;
  // Estrazione uniforme fra le celle libere, anche quando ne rimane una sola.
  uint8_t chosen = (uint8_t)random((long)(64 - snakeLength));
  for (uint8_t position = 0; position < 64; ++position) {
    if (snakeOccupies(position, snakeLength)) continue;
    if (chosen == 0) {
      snakeFood = position;
      return;
    }
    --chosen;
  }
}

void resetSnake(uint32_t now) {
  snakeLength = 3;
  snakeBody[0] = 3 * 8 + 3;
  snakeBody[1] = 3 * 8 + 2;
  snakeBody[2] = 3 * 8 + 1;
  snakeDirection = SNAKE_RIGHT;
  snakeNextDirection = SNAKE_RIGHT;
  snakeTurnQueued = false;
  placeSnakeFood();
  lastStepTime = now;
}

void updateHandheldSpeed() {
  const int pot = analogRead(POT_PIN);
  stepInterval = (uint16_t)(operatingMode == MODE_SNAKE
      ? map(pot, 0, 1023, 700, 100)
      : map(pot, 0, 1023, 2000, 100));
}

void initializeHandheld(uint32_t now) {
  // Difesa esplicita: nessun impulso pendente puo' entrare nelle modalita' gioco.
  for (uint8_t i = 0; i < PULSE_COUNT; ++i) {
    pulseActive[i] = false;
    digitalWrite(pulsePins[i], LOW);
  }
  digitalWrite(ALT_MODE_LED_PIN, LOW);
  randomSeed((unsigned long)analogRead(POT_PIN) ^ micros());
  for (uint8_t i = 0; i < HH_BUTTON_COUNT; ++i) {
    const bool active = readCommandActive(handheldPins[i]);
    handheldInputs[i].raw = active;
    handheldInputs[i].down = active;
    handheldInputs[i].pressed = false;
    handheldInputs[i].changedAt = now;
  }
  switchCandidate = readClockSwitch();
  clockMode = switchCandidate < 0 ? (uint8_t)CLOCK_EXTERNAL : (uint8_t)switchCandidate;
  switchChangedAt = now;
  lastStepTime = now;
  lastPotSample = now;
  updateHandheldSpeed();
  if (operatingMode == MODE_SNAKE) resetSnake(now);
  else {
    memset(currentGen, 0, sizeof(currentGen)); // tela vuota, nessuna rigenerazione
    lifeCursorX = 3;
    lifeCursorY = 3;
    lifeCursorInverted = true;
    lifeCursorAt = now;
  }
  handheldPhase = HH_ENTRY;
  handheldPhaseAt = now;
  handheldReleaseTracking = false;
  renderHandheldEntry(0);
}

void serviceBootSelection(uint32_t now) {
  const uint32_t elapsed = (uint32_t)(now - bootStartedAt);
  if (elapsed < BOOT_SETTLE_MS) return;
  const uint8_t reading = (readButtonActive(BTN_ALT) ? 1 : 0) |
      (readButtonActive(BTN_SPAWN) ? 2 : 0) |
      (readButtonActive(BTN_KILL) ? 4 : 0);
  if (reading != bootCandidate) {
    bootCandidate = reading;
    bootChangedAt = now;
  }
  if (elapsed < BOOT_MIN_MS) return;
  const bool stable = (uint32_t)(now - bootChangedAt) >= BOOT_SETTLE_MS;
  if (!stable && elapsed < BOOT_MAX_MS) return;
  // ALT+SPAWN+KILL, o letture non stabilizzate entro 240 ms: Eurorack.
  operatingMode = stable && reading == 3 ? MODE_SNAKE
      : stable && reading == 5 ? MODE_GAME_OF_LIFE : MODE_EURORACK;
  bootSelecting = false;
  if (operatingMode == MODE_EURORACK) initializeEurorack();
  else initializeHandheld(now);
}

bool handheldControlsReleased() {
  if (readButtonActive(BTN_ALT)) return false;
  for (uint8_t i = 0; i < HH_BUTTON_COUNT; ++i) {
    if (handheldInputs[i].raw || handheldInputs[i].down) return false;
  }
  return true;
}

void queueSnakeDirection(uint8_t direction) {
  if (snakeTurnQueued || direction == snakeDirection ||
      direction == ((snakeDirection + 2) % 4)) return;
  snakeNextDirection = direction;
  // Una sola svolta per movimento impedisce anche l'inversione in due pressioni.
  snakeTurnQueued = true;
}

void moveSnake(uint32_t now) {
  snakeDirection = snakeNextDirection;
  snakeTurnQueued = false;
  uint8_t x = snakeBody[0] % 8;
  uint8_t y = snakeBody[0] / 8;
  if (snakeDirection == SNAKE_UP) y = (y + 7) % 8;
  else if (snakeDirection == SNAKE_DOWN) y = (y + 1) % 8;
  else if (snakeDirection == SNAKE_LEFT) x = (x + 7) % 8;
  else x = (x + 1) % 8;
  const uint8_t head = y * 8 + x;
  const bool growing = head == snakeFood;
  // La vecchia coda e' libera in questo stesso tick, tranne quando si mangia.
  if (snakeOccupies(head, growing ? snakeLength : snakeLength - 1)) {
    handheldPhase = HH_LOST;
    handheldPhaseAt = now;
    return;
  }
  if (growing) ++snakeLength;
  for (uint8_t i = snakeLength - 1; i > 0; --i) snakeBody[i] = snakeBody[i - 1];
  snakeBody[0] = head;
  if (snakeLength == 64) {
    snakeFood = 64;
    handheldPhase = HH_WON;
    handheldPhaseAt = now;
  } else if (growing) placeSnakeFood();
}

void serviceSnake(uint32_t now) {
  if (handheldPhase == HH_LOST || handheldPhase == HH_WON) {
    const uint32_t elapsed = (uint32_t)(now - handheldPhaseAt);
    if (elapsed >= HANDHELD_FLASH_MS && handheldInputs[HH_LEFT].pressed) {
      resetSnake(now); // SPAWN riavvia; non viene interpretato come svolta.
      handheldPhase = HH_PLAY;
      handheldPhaseAt = now;
    }
    renderHandheld(elapsed < HANDHELD_FLASH_MS && (elapsed / 100) % 2 != 0);
    return;
  }
  if (handheldInputs[HH_UP].pressed) queueSnakeDirection(SNAKE_UP);
  if (handheldInputs[HH_DOWN].pressed) queueSnakeDirection(SNAKE_DOWN);
  if (handheldInputs[HH_LEFT].pressed) queueSnakeDirection(SNAKE_LEFT);
  if (handheldInputs[HH_RIGHT].pressed) queueSnakeDirection(SNAKE_RIGHT);
  if ((uint32_t)(now - lastStepTime) >= stepInterval) {
    lastStepTime = now; // niente raffiche di recupero dopo un ritardo
    moveSnake(now);
  }
  renderHandheld(false);
}

// B3/S23 completamente toroidale SOLO per Life handheld.
// Indipendente dalla regola selezionata per il GoL musicale.
void computeToroidalGenerationInto(const byte src[8], byte dst[8]) {
  for (uint8_t row = 0; row < 8; ++row) {
    byte value = 0;
    for (uint8_t col = 0; col < 8; ++col) {
      uint8_t neighbors = 0;
      for (int8_t dr = -1; dr <= 1; ++dr) {
        for (int8_t dc = -1; dc <= 1; ++dc) {
          if (dr == 0 && dc == 0) continue;
          const uint8_t rr = (row + dr + 8) % 8;
          const uint8_t cc = (col + dc + 8) % 8;
          if (bitRead(src[rr], cc)) ++neighbors;
        }
      }
      if (neighbors == 3 || (neighbors == 2 && bitRead(src[row], col))) {
        bitSet(value, col);
      }
    }
    dst[row] = value;
  }
}

void serviceHandheldLife(uint32_t now) {
  updateClockMode(now); // logica INT/EXT e debounce originali; nessun tick esterno
  bool moved = false;
  if (handheldInputs[HH_UP].pressed) {
    lifeCursorY = (lifeCursorY + 7) % 8;
    moved = true;
  } else if (handheldInputs[HH_DOWN].pressed) {
    lifeCursorY = (lifeCursorY + 1) % 8;
    moved = true;
  } else if (handheldInputs[HH_LEFT].pressed) {
    lifeCursorX = (lifeCursorX + 7) % 8;
    moved = true;
  } else if (handheldInputs[HH_RIGHT].pressed) {
    lifeCursorX = (lifeCursorX + 1) % 8;
    moved = true;
  }
  if (moved) {
    lifeCursorInverted = true;
    lifeCursorAt = now;
  }
  if (handheldInputs[HH_CREATE].pressed) {
    setCellDisp(7 - lifeCursorX, 7 - lifeCursorY, true);
  }
  if (handheldInputs[HH_ERASE].pressed) {
    setCellDisp(7 - lifeCursorX, 7 - lifeCursorY, false);
  }
  if ((uint32_t)(now - lifeCursorAt) >= (lifeCursorInverted ? 120U : 600U)) {
    lifeCursorInverted = !lifeCursorInverted;
    lifeCursorAt = now;
  }
  if (clockMode == CLOCK_INTERNAL && (uint32_t)(now - lastStepTime) >= stepInterval) {
    lastStepTime = now;
    computeToroidalGenerationInto(currentGen, nextGen);
    memcpy(currentGen, nextGen, 8);
  }
  // Nessun history, rilevamento oscillatori o generatore casuale in questa via.
  renderHandheld(false);
}

void loopHandheld(uint32_t now) {
  updateHandheldInputs(now);
  if (handheldPhase == HH_ENTRY) {
    const uint32_t elapsed = (uint32_t)(now - handheldPhaseAt);
    if (elapsed < HANDHELD_ENTRY_MS) {
      renderHandheldEntry(elapsed);
      return;
    }
    handheldPhase = HH_WAIT_RELEASE;
    handheldReleaseTracking = false;
  }
  if (handheldPhase == HH_WAIT_RELEASE) {
    if (!handheldControlsReleased()) handheldReleaseTracking = false;
    else if (!handheldReleaseTracking) {
      handheldReleaseTracking = true;
      handheldReleaseAt = now;
    } else if ((uint32_t)(now - handheldReleaseAt) >= HANDHELD_DEBOUNCE_MS) {
      handheldPhase = HH_PLAY;
      lastStepTime = now;
      lifeCursorAt = now;
    }
    renderHandheld(false);
    return; // consuma ogni pressione iniziale prima di abilitare il gioco
  }
  if ((uint32_t)(now - lastPotSample) >= POT_SAMPLE_MS) {
    lastPotSample = now;
    updateHandheldSpeed();
  }
  if (operatingMode == MODE_SNAKE) serviceSnake(now);
  else serviceHandheldLife(now);
}

// --- Selettore regole e dual gate sequencer ---
void stopMusicalPulses() {
  for (uint8_t i = 0; i < PULSE_COUNT; ++i) {
    pulseActive[i] = false;
    digitalWrite(pulsePins[i], LOW);
  }
}

void syncRuntimeInputs(uint32_t now) {
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    const bool active = readButtonActive(i);
    buttonRaw[i] = buttonDown[i] = active;
    buttonPressed[i] = false;
    buttonChangedAt[i] = now;
    buttonAcceptedAt[i] = now;
    buttonHasTriggered[i] = active;
  }
  for (uint8_t i = 0; i < HH_BUTTON_COUNT; ++i) {
    const bool active = readCommandActive(handheldPins[i]);
    handheldInputs[i].raw = handheldInputs[i].down = active;
    handheldInputs[i].pressed = false;
    handheldInputs[i].changedAt = now;
  }
  altLongHandled = true;
  altPressedAt = now;
  runtimeAltConsumed = true;
  runtimeChordKey = BUTTON_COUNT;
}

void requireRuntimeRelease(uint32_t now) {
  runtimeReleaseRequired = true;
  runtimeReleaseTracking = false;
  syncRuntimeInputs(now);
}

bool serviceRuntimeRelease(uint32_t now) {
  if (!runtimeReleaseRequired) return false;
  bool released = true;
  // Non attendere il jack CLOCK IN: un clock esterno continuo deve restare usabile.
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    if (readButtonActive(i)) released = false;
  }
  if (!released) runtimeReleaseTracking = false;
  else if (!runtimeReleaseTracking) {
    runtimeReleaseTracking = true;
    runtimeReleaseAt = now;
  } else if ((uint32_t)(now - runtimeReleaseAt) >= ALT_DEBOUNCE_MS) {
    runtimeReleaseRequired = false;
    runtimeReleaseTracking = false;
    syncRuntimeInputs(now);
    runtimeAltConsumed = false;
    lastStepTime = now;
    resetExternalClock(now);
    displayDirty = true;
  }
  servicePulses(now);
  if (ruleMenuActive) renderRuleMenu(now);
  else if (operatingMode == MODE_DUAL) renderDualSequencer(now);
  else refreshDisplayAtCursor();
  return true;
}

void openRuleMenu(uint32_t now) {
  ruleMenuActive = true;
  ruleMenuCandidate = golRuleIndex;
  ruleMenuBlinkAt = now;
  menuClearTracking = false;
  stopMusicalPulses();
  requireRuntimeRelease(now);
  renderRuleMenu(now);
}

void commitRuleMenu(bool empty, uint32_t now) {
  golRuleIndex = ruleMenuCandidate;
  currentStep = 0;
  if (empty) {
    memset(currentGen, 0, sizeof(currentGen));
    holdEmptyGrid = true;
    markManualEdit();
  } else generateRandomGeneration();
  ruleMenuActive = false;
  menuClearTracking = false;
  lastStepTime = now;
  resetExternalClock(now);
  stopMusicalPulses();
  displayDirty = true;
  requireRuntimeRelease(now);
  refreshDisplayAtCursor();
}

void renderRuleMenu(uint32_t now) {
  byte frame[8] = {};
  frame[0] = (1U << GOL_RULE_COUNT) - 1U;
  // Cinque LED in alto a SX; il selezionato lampeggia, gli altri restano accesi.
  if ((uint32_t)(now - ruleMenuBlinkAt) % 500U < 180U) {
    bitClear(frame[0], ruleMenuCandidate);
  }
  writeHandheldFrame(frame);
}

void loopRuleMenu(uint32_t now) {
  servicePulses(now);
  updateButtons(now); // ALT debounced, senza eseguire le vecchie azioni
  updateHandheldInputs(now);
  const bool alt = buttonDown[BTN_ALT] && readButtonActive(BTN_ALT);
  const bool kill = handheldInputs[HH_DOWN].down && readCommandActive(KILL_CELL_PIN);
  if (alt && kill) {
    if (!menuClearTracking) {
      menuClearTracking = true;
      menuClearAt = now;
    } else if ((uint32_t)(now - menuClearAt) >= MENU_CLEAR_HOLD_MS) {
      commitRuleMenu(true, now);
      return;
    }
  } else menuClearTracking = false;

  if (!readButtonActive(BTN_ALT)) {
    if (handheldInputs[HH_UP].pressed || handheldInputs[HH_LEFT].pressed) {
      ruleMenuCandidate = (ruleMenuCandidate + GOL_RULE_COUNT - 1) % GOL_RULE_COUNT;
      ruleMenuBlinkAt = now;
    } else if (handheldInputs[HH_DOWN].pressed || handheldInputs[HH_ERASE].pressed) {
      // ROTATE = destra: il jack GATE IN non sposta la scelta col clock esterno.
      ruleMenuCandidate = (ruleMenuCandidate + 1) % GOL_RULE_COUNT;
      ruleMenuBlinkAt = now;
    }
    if (handheldInputs[HH_CREATE].pressed) {
      commitRuleMenu(false, now);
      return;
    }
  }
  renderRuleMenu(now);
}

void toggleDualMode(uint32_t now) {
  stopMusicalPulses();
  ruleMenuActive = false;
  if (operatingMode == MODE_DUAL) {
    operatingMode = MODE_EURORACK;
    digitalWrite(ALT_MODE_LED_PIN, altControlsActive ? HIGH : LOW);
    displayDirty = true;
  } else {
    operatingMode = MODE_DUAL;
    dualRunning = false; // ingresso sempre in pausa, pattern e lunghezze conservati
    dualCursorAt = now;
    dualLengthPreview = false;
    digitalWrite(ALT_MODE_LED_PIN, LOW);
  }
  lastStepTime = now;
  resetExternalClock(now);
  requireRuntimeRelease(now);
  if (operatingMode == MODE_DUAL) renderDualSequencer(now);
  else refreshDisplayAtCursor();
}

void resetDualPlayheads(uint32_t now) {
  dualPosition[0] = dualPosition[1] = 0;
  lastStepTime = now;
  resetExternalClock(now);
  stopMusicalPulses();
}

void setDualLengthAtCursor(uint32_t now) {
  const uint8_t lane = dualCursorY / 4;
  dualLength[lane] = (dualCursorY % 4) * 8 + dualCursorX + 1;
  if (dualPosition[lane] >= dualLength[lane]) dualPosition[lane] = 0;
  dualLengthPreview = true;
  dualLengthPreviewAt = now;
}

bool handleRuntimeChords(uint32_t now) {
  const bool altRaw = readButtonActive(BTN_ALT);
  if (!altRaw) runtimeAltConsumed = false;
  if (runtimeChordKey < BUTTON_COUNT) {
    if (buttonDown[BTN_ALT] && buttonDown[runtimeChordKey]) return true;
    runtimeChordKey = BUTTON_COUNT;
  }
  if (!altRaw) return false;
  uint8_t key = BUTTON_COUNT;
  if (readButtonActive(BTN_ROTATE)) key = BTN_ROTATE;
  else if (readButtonActive(BTN_NEW)) key = BTN_NEW;
  else if (operatingMode == MODE_DUAL && readButtonActive(BTN_LOCK)) key = BTN_LOCK;
  else if (operatingMode == MODE_DUAL && readButtonActive(BTN_SPAWN)) key = BTN_SPAWN;
  if (key == BUTTON_COUNT) return false;

  // La lettura raw sopprime subito NEW/ROTATE mentre ALT completa i suoi 50 ms.
  runtimeAltConsumed = true;
  altLongHandled = true;
  if (!buttonDown[BTN_ALT] || !buttonDown[key]) return true;
  if (!buttonPressed[BTN_ALT] && !buttonPressed[key]) return true;
  runtimeChordKey = key;
  // Una combo non deve cambiare il latch ALT, neppure se ALT e' stato premuto
  // abbastanza in anticipo da aver gia' eseguito il suo toggle lungo.
  if (operatingMode == MODE_EURORACK) {
    altControlsActive = altStateAtPress;
    digitalWrite(ALT_MODE_LED_PIN, altControlsActive ? HIGH : LOW);
  }
  if (key == BTN_ROTATE) toggleDualMode(now);
  else if (operatingMode == MODE_EURORACK && key == BTN_NEW) openRuleMenu(now);
  else if (operatingMode == MODE_DUAL) {
    if (key == BTN_NEW) resetDualPlayheads(now);
    else if (key == BTN_SPAWN) setDualLengthAtCursor(now);
    else if (key == BTN_LOCK) {
      dualRunning = !dualRunning;
      digitalWrite(ALT_MODE_LED_PIN, dualRunning ? HIGH : LOW);
      lastStepTime = now;
      resetExternalClock(now);
      if (!dualRunning) stopMusicalPulses();
    }
  }
  return true;
}

void advanceDualSequencers(uint32_t now) {
  if (!dualRunning) return;
  startPulse(PULSE_CLOCK, now);
  for (uint8_t lane = 0; lane < 2; ++lane) {
    if (dualPattern[lane] & ((uint32_t)1 << dualPosition[lane])) {
      startPulse(lane == 0 ? PULSE_LIVE : PULSE_END, now);
    }
    if (++dualPosition[lane] >= dualLength[lane]) dualPosition[lane] = 0;
  }
}

void renderDualSequencer(uint32_t now) {
  byte frame[8] = {};
  const bool cursorBlink = (uint32_t)(now - dualCursorAt) % 720U < 120U;
  const bool playBlink = (uint32_t)(now - dualCursorAt) % 480U < 240U;
  if (dualLengthPreview && (uint32_t)(now - dualLengthPreviewAt) >= 700U) {
    dualLengthPreview = false;
  }
  for (uint8_t lane = 0; lane < 2; ++lane) {
    for (uint8_t step = 0; step < 32; ++step) {
      const uint8_t x = step % 8;
      const uint8_t y = lane * 4 + step / 8;
      bool state = step < dualLength[lane] &&
          ((dualPattern[lane] & ((uint32_t)1 << step)) != 0);
      if (dualLengthPreview) state = step < dualLength[lane];
      else {
        // Non fare due XOR se cursore e playhead occupano lo stesso pixel.
        const bool overlay = (cursorBlink && x == dualCursorX && y == dualCursorY) ||
            (dualRunning && playBlink && step == dualPosition[lane]);
        if (overlay) state = !state;
      }
      if (state) bitSet(frame[y], x);
    }
  }
  writeHandheldFrame(frame);
}

void loopDualSequencer(uint32_t now) {
  servicePulses(now);
  updateClockMode(now);
  updateButtons(now);
  updateHandheldInputs(now);
  const bool chord = handleRuntimeChords(now);
  if (operatingMode != MODE_DUAL || runtimeReleaseRequired) return;
  if (!chord) {
    bool moved = false;
    if (handheldInputs[HH_UP].pressed) {
      dualCursorY = (dualCursorY + 7) % 8; moved = true;
    } else if (handheldInputs[HH_DOWN].pressed) {
      dualCursorY = (dualCursorY + 1) % 8; moved = true;
    } else if (handheldInputs[HH_LEFT].pressed) {
      dualCursorX = (dualCursorX + 7) % 8; moved = true;
    } else if (handheldInputs[HH_ERASE].pressed) {
      dualCursorX = (dualCursorX + 1) % 8; moved = true;
    }
    if (moved) dualCursorAt = now;
    if (handheldInputs[HH_CREATE].pressed) {
      const uint8_t lane = dualCursorY / 4;
      const uint8_t step = (dualCursorY % 4) * 8 + dualCursorX;
      dualPattern[lane] ^= (uint32_t)1 << step;
    }
  }
  if ((uint32_t)(now - lastPotSample) >= POT_SAMPLE_MS) {
    lastPotSample = now;
    updateBPM();
  }
  if (dualRunning) {
    if (clockMode == CLOCK_INTERNAL) {
      if ((uint32_t)(now - lastStepTime) >= stepInterval) {
        lastStepTime = now;
        advanceDualSequencers(now);
      }
    } else serviceExternalClock(now);
  }
  renderDualSequencer(now);
  servicePulses(millis());
}

void loop() {
  if (bootSelecting) {
    serviceBootSelection(millis());
    return;
  }
  if (operatingMode == MODE_EURORACK || operatingMode == MODE_DUAL) {
    const uint32_t now = millis();
    if (serviceRuntimeRelease(now)) return;
    if (ruleMenuActive) loopRuleMenu(now);
    else if (operatingMode == MODE_DUAL) loopDualSequencer(now);
    else loopEurorack();
  } else loopHandheld(millis());
}
