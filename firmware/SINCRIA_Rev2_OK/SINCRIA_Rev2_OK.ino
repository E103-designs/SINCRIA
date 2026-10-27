//S’INCRIA 4 Agosto 2025

#include <LedControl.h>

// Pin definitions for MAX7219
const int DIN_PIN = 12;
const int CLK_PIN = 10;
const int CS_PIN = 11;
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1);

// Pin definitions
const int POT_PIN = A0;                     // Potentiometer pin
const int SWITCH_INTERNAL_CLOCK_PIN = A1;   // Switch for internal clock
const int SWITCH_EXTERNAL_CLOCK_PIN = A2;   // Switch for external clock
const int NEW_GAME_PIN = 2;                 // Digital input for new game generation
const int LIVE_CELL_LED_PIN = 3;            // LED that lights up when a live cell is encountered
const int GAME_END_LED_PIN = 4;             // LED that lights up when the game ends
const int CLOCK_OUT_PIN = 5;                // Clock output
const int LOCK_GENERATION_PIN = 6;          // Input to lock/unlock the current generation
const int CLOCK_IN_PIN = 7;                 // Input for external clock
const int ROTATE_GENERATION_PIN = 8;        // Input to rotate the current generation
const int ALT_MODES_PIN = 9;                // Input for alternate modes button
const int SPAWN_CELL_PIN = A3;              // Input to create a new cell at a random dead position
const int KILL_CELL_PIN = A4;               // Input to kill a cell at a random live position
const int ALT_MODE_LED_PIN = A5;            // Output for alternate mode LED

// Game variables
byte currentGen[8];    // Current generation
byte nextGen[8];       // Next generation
byte previousGen[8];   // Previous generation to detect stalling
int currentStep = 0;   // Current step in sequencer (0-63)

// Timing variables
unsigned long lastStepTime = 0;
unsigned long liveCellLedOnTime = 0;
bool liveCellLedState = LOW;
int BPM = 60;
int stepInterval = 250;  // Milliseconds between steps

// Debounce variables for inputs
unsigned long lastDebounceTimeNewGame = 0;
unsigned long lastDebounceTimeLock = 0;
unsigned long lastDebounceTimeRotate = 0;
unsigned long lastDebounceTimeSpawnCell = 0;
unsigned long lastDebounceTimeKillCell = 0;
unsigned long lastDebounceTimeAltModes = 0;
unsigned long debounceDelay = 50; // 50 milliseconds debounce

bool lastNewGameButtonState = LOW;
bool newGameButtonState = LOW;
bool lastLockButtonState = LOW;
bool lockButtonState = LOW;
bool lastRotateButtonState = LOW;
bool rotateButtonState = LOW;
bool lastSpawnCellButtonState = LOW;
bool spawnCellButtonState = LOW;
bool lastKillCellButtonState = LOW;
bool killCellButtonState = LOW;
bool lastAltModesButtonState = LOW;
bool altModesButtonState = LOW;

// Debounce variables for clock mode switch
unsigned long lastDebounceTimeClockSwitch = 0;
unsigned long debounceDelayClockSwitch = 50; // 50 milliseconds debounce
int lastClockMode = -1;

// Variables for clock mode
int clockMode = 0; // 0 for internal clock, 1 for external clock

// Variables for external clock
bool lastClockInState = LOW;
bool debouncedClockInState = LOW;
unsigned long lastClockInDebounceTime = 0;
unsigned long clockInDebounceDelay = 5; // 5 milliseconds debounce

// Game state
bool gameIsOver = false;
bool gameIsStalled = false;
bool generationLocked = false;

// Variables for clock output
unsigned long clockOutOnTime = 0;
bool clockOutState = LOW;

// Variables for GAME_END_LED_PIN
bool gameEndLedOn = false;
unsigned long gameEndLedStartTime = 0;

// Variables for altControls mode
bool altControlsActive = false;
unsigned long altModesButtonPressedTime = 0;
bool altModesButtonHeld = false;
unsigned long altModesLongPressDuration = 200; // 200ms for long press

bool spawnCellHeld = false;
bool killCellHeld = false;

void setup() {
  // Initialize MAX7219
  lc.shutdown(0, false); // Wake up the MAX7219
  lc.setIntensity(0, 8); // Set brightness (0-15)
  lc.clearDisplay(0);    // Clear display

  // Initialize pins as INPUT (with external pull-down resistors)
  pinMode(NEW_GAME_PIN, INPUT);
  pinMode(LOCK_GENERATION_PIN, INPUT);
  pinMode(ROTATE_GENERATION_PIN, INPUT);
  pinMode(SPAWN_CELL_PIN, INPUT);
  pinMode(KILL_CELL_PIN, INPUT);
  pinMode(ALT_MODES_PIN, INPUT);

  // Initialize other pins
  pinMode(LIVE_CELL_LED_PIN, OUTPUT);
  pinMode(GAME_END_LED_PIN, OUTPUT);
  pinMode(ALT_MODE_LED_PIN, OUTPUT);
  pinMode(CLOCK_OUT_PIN, OUTPUT);
  pinMode(CLOCK_IN_PIN, INPUT);

  // Set initial states for LEDs
  digitalWrite(GAME_END_LED_PIN, LOW);
  digitalWrite(ALT_MODE_LED_PIN, LOW);

  // Set A1 and A2 as digital inputs
  pinMode(SWITCH_INTERNAL_CLOCK_PIN, INPUT);
  pinMode(SWITCH_EXTERNAL_CLOCK_PIN, INPUT);

  // Seed random number generator using A6 (available on Arduino Nano)
  randomSeed(analogRead(A6)); // Use an unconnected analog pin for a more random seed

  // Generate initial random generation
  generateRandomGeneration();

  // Initial BPM and step interval
  updateBPM();
}

void loop() {
  unsigned long currentMillis = millis();

  // Update clock mode with debounce
  updateClockMode(currentMillis);

  // Update BPM and step interval
  updateBPM();

  // Handle clock according to mode
  if (clockMode == 0) {
    // Internal clock mode
    if (currentMillis - lastStepTime >= stepInterval) {
      lastStepTime = currentMillis;

      // Output clock pulse
      digitalWrite(CLOCK_OUT_PIN, HIGH);
      clockOutOnTime = currentMillis;
      clockOutState = HIGH;

      // Advance sequencer
      advanceSequencer();
    }
  } else if (clockMode == 1) {
    // External clock mode
    int clockInState = digitalRead(CLOCK_IN_PIN);

    // Debounce the clock input
    if (clockInState != debouncedClockInState) {
      lastClockInDebounceTime = currentMillis;
      debouncedClockInState = clockInState;
    }

    if ((currentMillis - lastClockInDebounceTime) > clockInDebounceDelay) {
      if (debouncedClockInState == HIGH && lastClockInState == LOW) {
        // Rising edge detected

        // Output clock pulse
        digitalWrite(CLOCK_OUT_PIN, HIGH);
        clockOutOnTime = currentMillis;
        clockOutState = HIGH;

        // Advance sequencer
        advanceSequencer();
      }
      lastClockInState = debouncedClockInState;
    }
  }

  // Turn off clock output after 10ms
  if (clockOutState == HIGH && currentMillis - clockOutOnTime >= 10) {
    digitalWrite(CLOCK_OUT_PIN, LOW);
    clockOutState = LOW;
  }

  // Turn off live cell LED after 10ms
  if (liveCellLedState == HIGH && currentMillis - liveCellLedOnTime >= 10) {
    digitalWrite(LIVE_CELL_LED_PIN, LOW);
    liveCellLedState = LOW;
  }

  // Handle the GAME_END_LED_PIN illumination
  if (gameEndLedOn && (currentMillis - gameEndLedStartTime >= 10)) {
    digitalWrite(GAME_END_LED_PIN, LOW);
    gameEndLedOn = false;
  }

  // Check if game has ended or is stalled
  if (isGameOver()) {
    gameIsOver = true; // Set the game over flag
  } else if (isGameStalled()) {
    gameIsStalled = true; // Set the game stalled flag
  } else {
    gameIsOver = false;
    gameIsStalled = false;
  }

  // Handle inputs regardless of clock mode
  handleInputs();
}

void handleInputs() {
  unsigned long currentMillis = millis();

  // Handle ALT_MODES_PIN input with debounce and long press detection
  int readingAltModes = digitalRead(ALT_MODES_PIN);

  if (readingAltModes != lastAltModesButtonState) {
    lastDebounceTimeAltModes = currentMillis;
  }

  if ((currentMillis - lastDebounceTimeAltModes) > debounceDelay) {
    if (readingAltModes != altModesButtonState) {
      altModesButtonState = readingAltModes;

      if (altModesButtonState == HIGH) {
        // Button just pressed
        altModesButtonPressedTime = currentMillis;
        altModesButtonHeld = false;
      } else {
        // Button just released
        if (altModesButtonHeld) {
          // Already handled in long press, do nothing
        } else {
          // Button released before long press duration, do nothing
        }
      }
    } else if (altModesButtonState == HIGH && !altModesButtonHeld) {
      // Button is being held down
      if ((currentMillis - altModesButtonPressedTime) >= altModesLongPressDuration) {
        // Long press detected
        altModesButtonHeld = true;
        altControlsActive = !altControlsActive;
        // Update the ALT_MODE_LED_PIN
        digitalWrite(ALT_MODE_LED_PIN, altControlsActive ? HIGH : LOW);

        // Reset shifts when exiting altControls mode
        if (!altControlsActive) {
          // No shifts to reset since we are shifting cells directly
        }
      }
    }
  }

  lastAltModesButtonState = readingAltModes;

  // Handle NEW_GAME_PIN input with debounce
  int readingNewGame = digitalRead(NEW_GAME_PIN);

  if (readingNewGame != lastNewGameButtonState) {
    lastDebounceTimeNewGame = currentMillis;
  }

  if ((currentMillis - lastDebounceTimeNewGame) > debounceDelay) {
    if (readingNewGame != newGameButtonState) {
      newGameButtonState = readingNewGame;

      // If button is pressed (HIGH with external pull-down)
      if (newGameButtonState == HIGH) {
        if (altControlsActive) {
          // altControls mode behavior
          // Restart the sequencer
          currentStep = 0;
          lastStepTime = currentMillis; // Reset timing

          // Check if first cell is alive to activate LIVE_CELL_LED_PIN
          int row = getCurrentRow();
          int col = getCurrentCol();
          bool cellState = bitRead(currentGen[row], col);

          if (cellState) {
            digitalWrite(LIVE_CELL_LED_PIN, HIGH);
            liveCellLedOnTime = currentMillis;
            liveCellLedState = HIGH;
          }

          // Update display
          updateDisplay(row, col);

          // Optionally, set GAME_END_LED_PIN if desired
          // digitalWrite(GAME_END_LED_PIN, HIGH);
          // gameEndLedOn = true;
          // gameEndLedStartTime = millis();

        } else {
          // Normal behavior
          if (gameIsOver || gameIsStalled) {
            // Start a new game if the previous one ended or stalled
            generateRandomGeneration();
            gameIsOver = false;
            gameIsStalled = false;
            currentStep = 0; // Reset sequencer
            lastStepTime = currentMillis; // Reset timing
          } else {
            // Generate a new random generation without interrupting the sequencer
            generateRandomGeneration();
          }
          // Update display immediately
          updateDisplay(getCurrentRow(), getCurrentCol());

          // Optionally, set GAME_END_LED_PIN if desired
          // digitalWrite(GAME_END_LED_PIN, HIGH);
          // gameEndLedOn = true;
          // gameEndLedStartTime = millis();
        }
      }
    }
  }

  lastNewGameButtonState = readingNewGame;

  // Handle LOCK_GENERATION_PIN input with debounce
  int readingLock = digitalRead(LOCK_GENERATION_PIN);

  if (readingLock != lastLockButtonState) {
    lastDebounceTimeLock = currentMillis;
  }

  if ((currentMillis - lastDebounceTimeLock) > debounceDelay) {
    if (readingLock != lockButtonState) {
      lockButtonState = readingLock;

      // If button is pressed (HIGH with external pull-down)
      if (lockButtonState == HIGH) {
        if (altControlsActive) {
          // altControls mode behavior
          // Shift cells left by one column
          shiftCellsLeft();
          // Update display
          updateDisplay(getCurrentRow(), getCurrentCol());
        } else {
          // Normal behavior
          // Toggle generation lock state
          generationLocked = !generationLocked;
          // Optionally, you could add visual feedback here
        }
      }
    }
  }

  lastLockButtonState = readingLock;

  // Handle ROTATE_GENERATION_PIN input with debounce
  int readingRotate = digitalRead(ROTATE_GENERATION_PIN);

  if (readingRotate != lastRotateButtonState) {
    lastDebounceTimeRotate = currentMillis;
  }

  if ((currentMillis - lastDebounceTimeRotate) > debounceDelay) {
    if (readingRotate != rotateButtonState) {
      rotateButtonState = readingRotate;

      // If button is pressed (HIGH with external pull-down)
      if (rotateButtonState == HIGH) {
        if (altControlsActive) {
          // altControls mode behavior
          // Shift cells up by one row
          shiftCellsUp();
          // Update display
          updateDisplay(getCurrentRow(), getCurrentCol());
        } else {
          // Normal behavior
          // Rotate the current generation by 90 degrees clockwise
          rotateGeneration(currentGen);
          // Update display immediately
          updateDisplay(getCurrentRow(), getCurrentCol());
        }
      }
    }
  }

  lastRotateButtonState = readingRotate;

  // Handle SPAWN_CELL_PIN input with debounce
  int readingSpawnCell = digitalRead(SPAWN_CELL_PIN);

  if (readingSpawnCell != lastSpawnCellButtonState) {
    lastDebounceTimeSpawnCell = currentMillis;
  }

  if ((currentMillis - lastDebounceTimeSpawnCell) > debounceDelay) {
    if (readingSpawnCell != spawnCellButtonState) {
      spawnCellButtonState = readingSpawnCell;

      // If button is pressed (HIGH with external pull-down)
      if (spawnCellButtonState == HIGH) {
        if (altControlsActive && clockMode == 1) {
          // altControls mode behavior in external clock mode
          // Set flag to indicate that SPAWN_CELL_PIN is being held
          spawnCellHeld = true;

          // Create live cell at current sequencer position
          int row = getCurrentRow();
          int col = getCurrentCol();

          if (!bitRead(currentGen[row], col)) {
            bitSet(currentGen[row], col);
            // Update display
            updateDisplay(row, col);
          }

        } else {
          // Normal behavior
          addRandomCell();
          // Update display immediately
          updateDisplay(getCurrentRow(), getCurrentCol());
        }
      } else {
        if (altControlsActive && clockMode == 1) {
          // Button released
          spawnCellHeld = false;
        }
      }
    }
  }

  lastSpawnCellButtonState = readingSpawnCell;

  // Handle KILL_CELL_PIN input with debounce
  int readingKillCell = digitalRead(KILL_CELL_PIN);

  if (readingKillCell != lastKillCellButtonState) {
    lastDebounceTimeKillCell = currentMillis;
  }

  if ((currentMillis - lastDebounceTimeKillCell) > debounceDelay) {
    if (readingKillCell != killCellButtonState) {
      killCellButtonState = readingKillCell;

      // If button is pressed (HIGH with external pull-down)
      if (killCellButtonState == HIGH) {
        if (altControlsActive && clockMode == 1) {
          // altControls mode behavior in external clock mode
          // Set flag to indicate that KILL_CELL_PIN is being held
          killCellHeld = true;

          // Kill live cell at current sequencer position
          int row = getCurrentRow();
          int col = getCurrentCol();

          if (bitRead(currentGen[row], col)) {
            bitClear(currentGen[row], col);
            // Update display
            updateDisplay(row, col);
          }

        } else {
          // Normal behavior
          killRandomCell();
          // Update display immediately
          updateDisplay(getCurrentRow(), getCurrentCol());
        }
      } else {
        if (altControlsActive && clockMode == 1) {
          // Button released
          killCellHeld = false;
        }
      }
    }
  }

  lastKillCellButtonState = readingKillCell;
}

void updateClockMode(unsigned long currentMillis) {
  int a1Value = digitalRead(SWITCH_INTERNAL_CLOCK_PIN);
  int a2Value = digitalRead(SWITCH_EXTERNAL_CLOCK_PIN);

  int currentSwitchState = -1;

  // With external pull-down resistors, the switch is HIGH when closed and LOW when open
  if (a1Value == HIGH && a2Value == LOW) {
    currentSwitchState = 0; // Internal clock
  } else if (a1Value == LOW && a2Value == HIGH) {
    currentSwitchState = 1; // External clock
  } else {
    // Invalid state or switch in transition
    return;
  }

  if (currentSwitchState != lastClockMode) {
    lastDebounceTimeClockSwitch = currentMillis;
  }

  if ((currentMillis - lastDebounceTimeClockSwitch) > debounceDelayClockSwitch) {
    if (currentSwitchState != clockMode) {
      clockMode = currentSwitchState;
    }
  }

  lastClockMode = currentSwitchState;
}

void generateRandomGeneration() {
  for (int i = 0; i < 8; i++) {
    currentGen[i] = random(256); // Random byte between 0-255
    previousGen[i] = 0;          // Reset previous generation
  }
}

void updateBPM() {
  int potValue = analogRead(POT_PIN);
  BPM = map(potValue, 0, 1023, 60, 200);
  stepInterval = 15000 / BPM; // Convert BPM to interval in ms
}

void advanceSequencer() {
  // Move to the next step
  currentStep = (currentStep + 1) % 64;

  // Get current row and column
  int row = getCurrentRow();
  int col = getCurrentCol();

  // altControls mode cell modifications
  if (altControlsActive && clockMode == 1) {
    if (spawnCellHeld) {
      // Create live cell at current position
      if (!bitRead(currentGen[row], col)) {
        bitSet(currentGen[row], col);
      }
    }
    if (killCellHeld) {
      // Kill live cell at current position
      if (bitRead(currentGen[row], col)) {
        bitClear(currentGen[row], col);
      }
    }
  }

  // Get cell state after any possible modifications
  bool cellState = bitRead(currentGen[row], col);

  // Light up LED if cell is alive
  if (cellState) {
    digitalWrite(LIVE_CELL_LED_PIN, HIGH);
    liveCellLedOnTime = millis();
    liveCellLedState = HIGH;
  }

  // Update display
  updateDisplay(row, col);

  // Proceed to next generation after 64 steps
  if (currentStep == 63) {
    // Check if generation is not locked
    if (!gameIsOver && !gameIsStalled && !generationLocked) {
      // Preemptively check if the next generation will be game over
      if (willNextGenerationBeGameOver()) {
        // Generate a new game before the game over
        generateRandomGeneration();
        gameIsOver = false;
        gameIsStalled = false;
        currentStep = 0; // Reset sequencer
        lastStepTime = millis(); // Reset timing

        // Illuminate GAME_END_LED_PIN for 10ms only if not already lit
        if (!gameEndLedOn) {
          digitalWrite(GAME_END_LED_PIN, HIGH);
          gameEndLedOn = true;
          gameEndLedStartTime = millis();
        }
      } else {
        // Compute the next generation normally
        computeNextGeneration();
      }
    } else if (gameIsOver || gameIsStalled) {
      // Generate new game when sequencer reaches the end
      generateRandomGeneration();
      gameIsOver = false;
      gameIsStalled = false;

      // Illuminate GAME_END_LED_PIN for 10ms only if not already lit
      if (!gameEndLedOn) {
        digitalWrite(GAME_END_LED_PIN, HIGH);
        gameEndLedOn = true;
        gameEndLedStartTime = millis();
      }
    }
  }
}

int getCurrentRow() {
  // Invert the row calculation to go from bottom to top
  return 7 - (currentStep / 8);
}

int getCurrentCol() {
  // Column remains the same
  return currentStep % 8;
}

void updateDisplay(int currentRow, int currentCol) {
  for (int row = 0; row < 8; row++) {
    byte rowData = 0;
    for (int col = 0; col < 8; col++) {
      bool cellState = bitRead(currentGen[row], col);

      // Highlight current cell by inverting its state temporarily
      if (row == currentRow && col == currentCol) {
        cellState = !cellState;
      }

      if (cellState) {
        bitSet(rowData, col);
      } else {
        bitClear(rowData, col);
      }
    }
    lc.setRow(0, row, rowData);
  }
}

void rotateGeneration(byte gen[8]) {
  byte rotatedGen[8];

  // Rotate the generation by 90 degrees clockwise
  for (int row = 0; row < 8; row++) {
    rotatedGen[row] = 0;
    for (int col = 0; col < 8; col++) {
      bool cellState = bitRead(gen[7 - col], row); // 90° rotation
      if (cellState) {
        bitSet(rotatedGen[row], col);
      }
    }
  }

  // Copy the rotated generation back to the original
  for (int i = 0; i < 8; i++) {
    gen[i] = rotatedGen[i];
  }
}

void shiftCellsLeft() {
  for (int row = 0; row < 8; row++) {
    bool leftmostBit = (currentGen[row] & 0x80) >> 7;
    currentGen[row] = (currentGen[row] << 1) | leftmostBit; // Wrap around
  }
}

void shiftCellsUp() {
  byte topRow = currentGen[0];
  for (int row = 0; row < 7; row++) {
    currentGen[row] = currentGen[row + 1];
  }
  currentGen[7] = topRow; // Wrap around
}

void addRandomCell() {
  // Find all dead cell positions
  int deadCells[64];
  int deadCellCount = 0;

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (!bitRead(currentGen[row], col)) {
        deadCells[deadCellCount] = row * 8 + col;
        deadCellCount++;
      }
    }
  }

  if (deadCellCount > 0) {
    // Choose a random dead cell
    int randomIndex = random(deadCellCount);
    int position = deadCells[randomIndex];
    int row = position / 8;
    int col = position % 8;

    // Set the cell to alive
    bitSet(currentGen[row], col);
  }
}

void killRandomCell() {
  // Find all live cell positions
  int liveCells[64];
  int liveCellCount = 0;

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (bitRead(currentGen[row], col)) {
        liveCells[liveCellCount] = row * 8 + col;
        liveCellCount++;
      }
    }
  }

  if (liveCellCount > 0) {
    // Choose a random live cell
    int randomIndex = random(liveCellCount);
    int position = liveCells[randomIndex];
    int row = position / 8;
    int col = position % 8;

    // Set the cell to dead
    bitClear(currentGen[row], col);
  }
}

bool willNextGenerationBeGameOver() {
  // Simulate the next generation into a temporary array
  byte tempNextGen[8];
  for (int row = 0; row < 8; row++) {
    tempNextGen[row] = 0;
    for (int col = 0; col < 8; col++) {
      int liveNeighbors = countLiveNeighbors(row, col);
      bool cellState = bitRead(currentGen[row], col);
      bool newCellState = cellState;

      if (cellState) {
        if (liveNeighbors < 2 || liveNeighbors > 3) {
          newCellState = false;
        }
      } else {
        if (liveNeighbors == 3) {
          newCellState = true;
        }
      }

      if (newCellState) {
        bitSet(tempNextGen[row], col);
      } else {
        bitClear(tempNextGen[row], col);
      }
    }
  }

  // Check if all cells in tempNextGen are dead
  for (int row = 0; row < 8; row++) {
    if (tempNextGen[row] != 0) {
      return false; // Next generation is not game over
    }
  }
  return true; // Next generation would result in game over
}

void computeNextGeneration() {
  // Copy currentGen to previousGen
  for (int i = 0; i < 8; i++) {
    previousGen[i] = currentGen[i];
  }

  for (int row = 0; row < 8; row++) {
    byte newRow = 0;
    for (int col = 0; col < 8; col++) {
      int liveNeighbors = countLiveNeighbors(row, col);
      bool cellState = bitRead(currentGen[row], col);
      bool newCellState = cellState;

      if (cellState) {
        if (liveNeighbors < 2 || liveNeighbors > 3) {
          newCellState = false;
        }
      } else {
        if (liveNeighbors == 3) {
          newCellState = true;
        }
      }

      if (newCellState) {
        bitSet(newRow, col);
      } else {
        bitClear(newRow, col);
      }
    }
    nextGen[row] = newRow;
  }

  // Update current generation
  for (int i = 0; i < 8; i++) {
    currentGen[i] = nextGen[i];
  }
}

int countLiveNeighbors(int row, int col) {
  int count = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      if (i == 0 && j == 0)
        continue;
      int neighborRow = row + i;
      int neighborCol = col + j;
      if (neighborRow < 0 || neighborRow >= 8)
        continue;
      if (neighborCol < 0)
        neighborCol = 7; // Wrap around horizontally
      else if (neighborCol >= 8)
        neighborCol = 0; // Wrap around horizontally
      if (bitRead(currentGen[neighborRow], neighborCol)) {
        count++;
      }
    }
  }
  return count;
}

bool isGameOver() {
  for (int i = 0; i < 8; i++) {
    if (currentGen[i] != 0) {
      return false;
    }
  }
  return true;
}

bool isGameStalled() {
  for (int i = 0; i < 8; i++) {
    if (currentGen[i] != previousGen[i]) {
      return false;
    }
  }
  return true;
}

