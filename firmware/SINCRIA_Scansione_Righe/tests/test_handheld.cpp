#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <climits>
#define __AVR_ATmega328P__ 1
#define ARDUINO_AVR_NANO 1
#include <Arduino.h>
uint32_t fakeTime = 0, rng = 1;
int pinLevels[22] = {}, pinModes[22] = {}, rises[22] = {}, potValue = 0;
#include "../SINCRIA_Scansione_Righe.ino"

unsigned checks = 0;
#define CHECK(expr) do { ++checks; if (!(expr)) { std::fprintf(stderr,"FAIL line %d at %u: %s\n",__LINE__,fakeTime,#expr); std::abort(); } } while (0)
void run(uint32_t duration) { while (duration--) { ++fakeTime; loop(); } }
void noOutputs() {
  for (uint8_t i = 0; i < PULSE_COUNT; ++i) {
    CHECK(pinLevels[pulsePins[i]] == LOW);
    CHECK(rises[pulsePins[i]] == 0);
    CHECK(!pulseActive[i]);
  }
}
void resetCold(uint32_t now = 0) {
  fakeTime = now; rng = 1; potValue = 0;
  memset(pinLevels, 0, sizeof pinLevels); memset(rises, 0, sizeof rises);
  for (auto &mode : pinModes) mode = -1;
  memset(buttonRaw, 0, sizeof buttonRaw); memset(buttonDown, 0, sizeof buttonDown);
  memset(buttonPressed, 0, sizeof buttonPressed); memset(buttonHasTriggered, 0, sizeof buttonHasTriggered);
  memset(buttonAcceptedAt, 0, sizeof buttonAcceptedAt); memset(buttonChangedAt, 0, sizeof buttonChangedAt);
  memset(pulseActive, 0, sizeof pulseActive); memset(pulseStartedAt, 0, sizeof pulseStartedAt);
  memset(currentGen, 0, 8); memset(nextGen, 0, 8); clearHistory();
  currentStep = 0; altControlsActive = false; generationLocked = false;
  altLongHandled = false; altPressedAt = 0; gameIsOver = false;
  displayCacheValid = false; displayDirty = true;
  operatingMode = MODE_EURORACK; bootSelecting = true;
  externalRaw = externalStable = externalArmed = false;
  pinLevels[A2] = HIGH;
}
void bootCombo(unsigned mask) {
  pinLevels[ALT_MODES_PIN] = (mask & 1) != 0;
  pinLevels[SPAWN_CELL_PIN] = (mask & 2) != 0;
  pinLevels[KILL_CELL_PIN] = (mask & 4) != 0;
}
void releaseGameControls() {
  for (uint8_t pin : handheldPins) pinLevels[pin] = LOW;
  pinLevels[ALT_MODES_PIN] = LOW;
  loop();
}
void bootGame(OperatingMode mode, uint32_t start = 0) {
  resetCold(start); bootCombo(mode == MODE_SNAKE ? 3 : 5); setup();
  run(139); CHECK(bootSelecting); noOutputs();
  run(1); CHECK(!bootSelecting && operatingMode == mode); CHECK(handheldPhase == HH_ENTRY);
  run(320); CHECK(handheldPhase == HH_WAIT_RELEASE);
  releaseGameControls(); run(60); CHECK(handheldPhase == HH_PLAY);
  noOutputs();
}
void pressGame(uint8_t index) { pinLevels[handheldPins[index]] = HIGH; loop(); run(30); }
void releaseGame(uint8_t index) { pinLevels[handheldPins[index]] = LOW; loop(); run(30); }
void tapGame(uint8_t index) { pressGame(index); releaseGame(index); }
bool physicalPixel(uint8_t x, uint8_t y) { return bitRead(lc.rows[7-x], 7-y); }
void snakeState(uint8_t dir, std::initializer_list<uint8_t> body, uint8_t food = 63) {
  snakeLength = 0;
  for (uint8_t pos : body) snakeBody[snakeLength++] = pos;
  snakeDirection = snakeNextDirection = dir; snakeTurnQueued = false; snakeFood = food;
  handheldPhase = HH_PLAY; lastStepTime = fakeTime;
}
void referenceLife(const byte src[8], byte dst[8]) {
  memset(dst, 0, 8);
  for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
    unsigned count = 0;
    for (int rr = 0; rr < 8; ++rr) for (int cc = 0; cc < 8; ++cc) {
      if (r == rr && c == cc) continue;
      int dx = std::abs(c-cc), dy = std::abs(r-rr);
      if ((dx <= 1 || dx == 7) && (dy <= 1 || dy == 7)) count += bitRead(src[rr], cc);
    }
    if (count == 3 || (count == 2 && bitRead(src[r], c))) bitSet(dst[r], c);
  }
}

int main() {
  // Ogni combinazione: la lettura non esegue azioni musicali prima della scelta.
  for (unsigned mask = 0; mask < 8; ++mask) {
    resetCold(); bootCombo(mask); setup(); run(139);
    CHECK(bootSelecting); noOutputs(); run(1);
    CHECK(!bootSelecting);
    CHECK(operatingMode == (mask == 3 ? MODE_SNAKE : mask == 5 ? MODE_GAME_OF_LIFE : MODE_EURORACK));
    noOutputs();
  }
  // Avvio normale reale: INT parte dopo un intervallo completo dalla selezione.
  resetCold(); pinLevels[A1] = HIGH; pinLevels[A2] = LOW; setup(); run(140);
  CHECK(operatingMode == MODE_EURORACK && lastStepTime == 140 && stepInterval == 500);
  run(499); CHECK(currentStep == 0); run(1); CHECK(currentStep == 1 && rises[CLOCK_OUT_PIN] == 1);
  run(10); CHECK(pinLevels[CLOCK_OUT_PIN] == LOW);
  resetCold(); pinLevels[CLOCK_IN_PIN] = HIGH; setup(); run(140); run(100);
  CHECK(currentStep == 0 && operatingMode == MODE_EURORACK);
  pinLevels[CLOCK_IN_PIN] = LOW; loop(); run(5);
  pinLevels[CLOCK_IN_PIN] = HIGH; loop(); run(4); CHECK(currentStep == 0);
  run(1); CHECK(currentStep == 1);
  // La combinazione applicata dopo la scelta non entra in un gioco.
  bootCombo(3); loop(); run(60); CHECK(operatingMode == MODE_EURORACK);
  // I rimbalzi allungano la qualificazione; un ingresso instabile non blocca il boot.
  resetCold(); setup(); run(120); bootCombo(3); loop(); run(39); CHECK(bootSelecting);
  run(1); CHECK(operatingMode == MODE_SNAKE && !bootSelecting);
  resetCold(); setup(); run(100);
  while (fakeTime < 240) { bootCombo(fakeTime % 10 == 0 ? 3 : 5); loop(); run(5); }
  CHECK(!bootSelecting && operatingMode == MODE_EURORACK); noOutputs();
  // Tenere la combinazione dopo il boot non muove Snake e non alterna ALT.
  resetCold(); bootCombo(3); setup(); run(1500);
  CHECK(operatingMode == MODE_SNAKE && handheldPhase == HH_WAIT_RELEASE);
  CHECK(snakeLength == 3 && snakeBody[0] == 27 && !altControlsActive);
  CHECK(pinLevels[ALT_MODE_LED_PIN] == LOW); noOutputs();
  releaseGameControls(); run(60); CHECK(handheldPhase == HH_PLAY);
  CHECK(snakeBody[0] == 27 && !snakeTurnQueued);
  run(699); CHECK(snakeBody[0] == 27); run(1); CHECK(snakeBody[0] == 28);
  CHECK(handheldPins[HH_RIGHT] == A4 && handheldPins[HH_CREATE] == 2 && handheldPins[HH_ERASE] == 7);

  // Orientamento reale e framebuffer, tutti i 64 pixel e nessuna trasposizione.
  for (uint8_t y = 0; y < 8; ++y) for (uint8_t x = 0; x < 8; ++x) {
    byte frame[8] = {}; bitSet(frame[y], x); writeHandheldFrame(frame);
    for (uint8_t yy = 0; yy < 8; ++yy) for (uint8_t xx = 0; xx < 8; ++xx) {
      CHECK(physicalPixel(xx, yy) == (x == xx && y == yy));
    }
  }

  bootGame(MODE_SNAKE);
  CHECK(snakeLength == 3 && !snakeOccupies(snakeFood, snakeLength));
  CHECK(stepInterval == 700); potValue = 1023; run(20); CHECK(stepInterval == 100);
  potValue = 0; run(20); CHECK(stepInterval == 700);
  // Un impulso di 29 ms viene scartato; uno stabile 30 ms genera un solo evento.
  pinLevels[LOCK_GENERATION_PIN] = HIGH; loop(); run(29); CHECK(!snakeTurnQueued);
  run(1); CHECK(snakeTurnQueued && snakeNextDirection == SNAKE_UP);
  run(100); CHECK(snakeNextDirection == SNAKE_UP); releaseGame(HH_UP);
  pressGame(HH_LEFT); CHECK(snakeNextDirection == SNAKE_UP); releaseGame(HH_LEFT);
  // Una sola svolta per tick impedisce RIGHT -> UP -> LEFT prima del movimento.
  moveSnake(fakeTime); CHECK(snakeDirection == SNAKE_UP && snakeBody[0] == 19);
  pressGame(HH_DOWN); CHECK(!snakeTurnQueued && snakeNextDirection == SNAKE_UP); releaseGame(HH_DOWN);
  pressGame(HH_RIGHT); CHECK(snakeNextDirection == SNAKE_RIGHT); releaseGame(HH_RIGHT);
  moveSnake(fakeTime); CHECK(snakeBody[0] == 20);
  // Wrapping sulle quattro direzioni, nella geometria fisica.
  snakeState(SNAKE_LEFT, {24}); moveSnake(fakeTime); CHECK(snakeBody[0] == 31);
  snakeState(SNAKE_RIGHT, {31}); moveSnake(fakeTime); CHECK(snakeBody[0] == 24);
  snakeState(SNAKE_UP, {3}); moveSnake(fakeTime); CHECK(snakeBody[0] == 59);
  snakeState(SNAKE_DOWN, {59}); moveSnake(fakeTime); CHECK(snakeBody[0] == 3);
  snakeState(SNAKE_RIGHT, {27,26,25}, 28); moveSnake(fakeTime);
  CHECK(snakeLength == 4 && snakeBody[0] == 28 && snakeBody[3] == 25);
  CHECK(snakeFood < 64 && !snakeOccupies(snakeFood, snakeLength));
  for (unsigned i = 0; i < 1000; ++i) { placeSnakeFood(); CHECK(!snakeOccupies(snakeFood, snakeLength)); }
  // La coda che si libera nello stesso tick e' attraversabile.
  snakeState(SNAKE_DOWN, {9,10,18,17}); moveSnake(fakeTime);
  CHECK(handheldPhase == HH_PLAY && snakeBody[0] == 17 && snakeLength == 4);
  snakeState(SNAKE_DOWN, {9,8,16,17,18,26}); moveSnake(fakeTime);
  CHECK(handheldPhase == HH_LOST && snakeBody[0] == 9);
  // SPAWN premuto durante il lampeggio e tenuto non riavvia in ritardo.
  pressGame(HH_LEFT);
  uint8_t lostBody[64]; memcpy(lostBody, snakeBody, 64); run(601);
  CHECK(handheldPhase == HH_LOST && memcmp(lostBody, snakeBody, 64) == 0); noOutputs();
  releaseGame(HH_LEFT);
  pressGame(HH_LEFT); CHECK(handheldPhase == HH_PLAY && snakeLength == 3);
  CHECK(snakeDirection == SNAKE_RIGHT && !snakeTurnQueued); releaseGame(HH_LEFT);
  // Vittoria al 64esimo segmento, array senza sforamenti e nessun random(0).
  snakeLength = 63;
  for (uint8_t i = 0; i < 63; ++i) {
    const uint8_t pos = i+1, y = pos/8, x = pos%8;
    snakeBody[i] = y*8 + (y%2 ? 7-x : x);
  }
  snakeFood = 0; snakeDirection = snakeNextDirection = SNAKE_LEFT; snakeTurnQueued = false;
  moveSnake(fakeTime); CHECK(handheldPhase == HH_WON && snakeLength == 64 && snakeFood == 64);
  std::set<uint8_t> unique(snakeBody, snakeBody+64); CHECK(unique.size() == 64);
  renderHandheld(false); for (byte value : lc.rows) CHECK(value == 255);
  run(600); tapGame(HH_LEFT); CHECK(handheldPhase == HH_PLAY && snakeLength == 3); noOutputs();

  bootGame(MODE_GAME_OF_LIFE);
  CHECK(!anyLiveCell(currentGen) && lifeCursorX == 3 && lifeCursorY == 3 && clockMode == CLOCK_EXTERNAL);
  CHECK(stepInterval == 2000); potValue = 1023; run(20); CHECK(stepInterval == 100);
  potValue = 0; run(20); CHECK(stepInterval == 2000);
  // Il cursore inverte solo l'immagine: nessuna scrittura nella griglia.
  byte before[8]; memcpy(before, currentGen, 8);
  lifeCursorInverted = true; renderHandheld(false); CHECK(physicalPixel(3,3));
  CHECK(memcmp(before, currentGen, 8) == 0);
  lifeCursorInverted = false; renderHandheld(false); CHECK(!physicalPixel(3,3));
  tapGame(HH_CREATE); CHECK(getCellDisp(4,4) && bitRead(currentGen[4],3));
  lifeCursorInverted = false; renderHandheld(false); CHECK(physicalPixel(3,3));
  lifeCursorInverted = true; renderHandheld(false); CHECK(!physicalPixel(3,3));
  tapGame(HH_ERASE); CHECK(!anyLiveCell(currentGen));
  lifeCursorX = 0; tapGame(HH_LEFT); CHECK(lifeCursorX == 7);
  tapGame(HH_RIGHT); CHECK(lifeCursorX == 0);
  lifeCursorY = 0; tapGame(HH_UP); CHECK(lifeCursorY == 7);
  tapGame(HH_DOWN); CHECK(lifeCursorY == 0);
  const uint8_t previousY = lifeCursorY; pressGame(HH_DOWN); run(1000);
  CHECK(lifeCursorY == (previousY+1)%8); releaseGame(HH_DOWN);
  CHECK(currentStep == 0 && !altControlsActive); noOutputs();
  // Tutte le 512 configurazioni locali e confronto indipendente sui bordi.
  for (unsigned bits = 0; bits < 512; ++bits) {
    byte src[8] = {}, dst[8] = {}, expected[8] = {};
    unsigned k = 0;
    for (int dr = -1; dr <= 1; ++dr) for (int dc = -1; dc <= 1; ++dc, ++k) {
      if (bits & (1U<<k)) bitSet(src[(dr+8)%8], (dc+8)%8);
    }
    referenceLife(src, expected); computeToroidalGenerationInto(src, dst);
    CHECK(memcmp(dst, expected, 8) == 0);
  }
  for (unsigned i = 0; i < 1000; ++i) {
    byte src[8], dst[8], expected[8]; for (auto &v : src) v = random(256);
    referenceLife(src, expected); computeToroidalGenerationInto(src, dst);
    CHECK(memcmp(dst, expected, 8) == 0);
  }
  byte corners[8] = {}, result[8] = {}; corners[0] = corners[7] = 129;
  computeToroidalGenerationInto(corners,result); CHECK(memcmp(corners,result,8) == 0);
  // Oscillatore attraverso i bordi orizzontale e verticale.
  byte blinker[8] = {}; blinker[0] = 131;
  computeToroidalGenerationInto(blinker,result);
  CHECK(result[7] == 1 && result[0] == 1 && result[1] == 1);
  byte back[8]; computeToroidalGenerationInto(result,back); CHECK(memcmp(back,blinker,8) == 0);
  // INT dopo 50 ms, prima generazione dopo un intervallo completo; EXT pausa.
  memcpy(currentGen, blinker, 8); pinLevels[A1] = HIGH; pinLevels[A2] = LOW; loop();
  run(49); CHECK(clockMode == CLOCK_EXTERNAL); run(1); CHECK(clockMode == CLOCK_INTERNAL);
  run(1999); CHECK(memcmp(currentGen, blinker, 8) == 0);
  run(1); CHECK(memcmp(currentGen, result, 8) == 0);
  for (unsigned i = 0; i < 20; ++i) {
    run(2000); CHECK(memcmp(currentGen, i%2 ? result : blinker, 8) == 0);
  }
  CHECK(historyCount == 0 && oscillationPeriod == 0 && currentStep == 0); noOutputs();
  // Modifica durante l'evoluzione: immediata e non legata a un clock in ingresso.
  lifeCursorX = 5; lifeCursorY = 5; tapGame(HH_CREATE); CHECK(getCellDisp(2,2));
  tapGame(HH_ERASE); CHECK(!getCellDisp(2,2));
  pinLevels[A1] = LOW; pinLevels[A2] = HIGH; loop(); run(50);
  memcpy(before,currentGen,8); run(4001); CHECK(memcmp(before,currentGen,8) == 0);
  // Ripresa: statico stabile, deserto persistente, nessun popolamento automatico.
  memcpy(currentGen,corners,8); pinLevels[A1] = HIGH; pinLevels[A2] = LOW; loop(); run(50);
  run(10000); CHECK(memcmp(currentGen,corners,8) == 0);
  memset(currentGen,0,8); run(10000); CHECK(!anyLiveCell(currentGen));
  memset(currentGen,255,8); run(2000); CHECK(!anyLiveCell(currentGen));
  run(4000); CHECK(!anyLiveCell(currentGen)); noOutputs();

  // Entrambi i giochi: ingressi/jack possono essere controlli, mai uscite musicali.
  for (OperatingMode mode : {MODE_SNAKE, MODE_GAME_OF_LIFE}) {
    bootGame(mode);
    for (unsigned iteration = 0; iteration < 3000; ++iteration) {
      uint32_t bits = static_cast<uint32_t>(random(1L<<20));
      for (unsigned i = 0; i < HH_BUTTON_COUNT; ++i) pinLevels[handheldPins[i]] = (bits>>i)&1;
      pinLevels[ALT_MODES_PIN] = (bits>>6)&1;
      pinLevels[A1] = (bits>>7)&1; pinLevels[A2] = (bits>>8)&1;
      potValue = (bits>>10)&1023;
      loop(); run(35); noOutputs();
      CHECK(operatingMode == mode && currentStep == 0 && !altControlsActive);
      CHECK(snakeLength > 0 && snakeLength <= 64);
      CHECK(lifeCursorX < 8 && lifeCursorY < 8);
    }
  }
  // Timer a 32 bit: boot, movimento ed evoluzione attraverso overflow.
  bootGame(MODE_SNAKE, UINT32_MAX-100);
  snakeFood = 63; run(700); CHECK(snakeBody[0] == 28); noOutputs();
  fakeTime = UINT32_MAX-15; lastStepTime = fakeTime; potValue = 1023; updateHandheldSpeed();
  run(100); CHECK(snakeBody[0] == 29); noOutputs();
  bootGame(MODE_GAME_OF_LIFE, UINT32_MAX-100);
  memcpy(currentGen,blinker,8); fakeTime = UINT32_MAX-30;
  clockMode = CLOCK_INTERNAL; switchCandidate = CLOCK_INTERNAL;
  pinLevels[A1] = HIGH; pinLevels[A2] = LOW; lastStepTime = fakeTime;
  potValue = 1023; updateHandheldSpeed(); run(100);
  CHECK(memcmp(currentGen,result,8) == 0); noOutputs();
  std::printf("PASS handheld: %u assertions; real .ino, simulated electrical I/O.\n",checks);
}
