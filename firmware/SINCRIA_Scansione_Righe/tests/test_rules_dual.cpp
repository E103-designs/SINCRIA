#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <set>
#define __AVR_ATmega328P__ 1
#define ARDUINO_AVR_NANO 1
#include <Arduino.h>
uint32_t fakeTime = 0, rng = 1;
int pinLevels[22] = {}, pinModes[22] = {}, rises[22] = {}, potValue = 0;
#include "../SINCRIA_Scansione_Righe.ino"
unsigned checks = 0;
#define CHECK(expr) do { ++checks; if (!(expr)) { std::fprintf(stderr,"FAIL line %d at %u: %s\n",__LINE__,fakeTime,#expr); std::abort(); } } while (0)
void run(uint32_t ms) { while (ms--) { ++fakeTime; loop(); } }
void pin(uint8_t p, bool active) { pinLevels[p] = active; loop(); }
void noOutputs() {
  for (uint8_t p : pulsePins) CHECK(pinLevels[p] == LOW);
}
void clearRiseCounts() { memset(rises,0,sizeof rises); }
void cold(uint32_t now = 0) {
  fakeTime = now; potValue = 0; rng = 1;
  memset(pinLevels,0,sizeof pinLevels); memset(rises,0,sizeof rises);
  memset(pinModes,0,sizeof pinModes);
  memset(buttonRaw,0,sizeof buttonRaw); memset(buttonDown,0,sizeof buttonDown);
  memset(buttonPressed,0,sizeof buttonPressed); memset(buttonHasTriggered,0,sizeof buttonHasTriggered);
  memset(buttonAcceptedAt,0,sizeof buttonAcceptedAt); memset(buttonChangedAt,0,sizeof buttonChangedAt);
  memset(pulseActive,0,sizeof pulseActive); memset(pulseStartedAt,0,sizeof pulseStartedAt);
  memset(currentGen,0,8); memset(nextGen,0,8); clearHistory();
  currentStep = 0; generationLocked = altControlsActive = altLongHandled = gameIsOver = false;
  displayDirty = true; displayCacheValid = false;
  operatingMode = MODE_EURORACK; bootSelecting = true;
  golRuleIndex = ruleMenuCandidate = RULE_CLASSIC;
  holdEmptyGrid = ruleMenuActive = menuClearTracking = false;
  runtimeReleaseRequired = runtimeReleaseTracking = runtimeAltConsumed = false;
  altStateAtPress = false;
  runtimeChordKey = BUTTON_COUNT;
  memset(dualPattern,0,sizeof dualPattern); dualLength[0] = dualLength[1] = 32;
  dualPosition[0] = dualPosition[1] = dualCursorX = dualCursorY = 0;
  dualRunning = dualLengthPreview = false;
  pinLevels[A2] = HIGH; setup(); run(140);
  CHECK(!bootSelecting && operatingMode == MODE_EURORACK);
  clearRiseCounts();
}
void releaseAll() {
  for (uint8_t p : buttonPins) pinLevels[p] = LOW;
  loop(); run(60);
  CHECK(!runtimeReleaseRequired);
}
void chord(uint8_t key) {
  pinLevels[ALT_MODES_PIN] = HIGH; pinLevels[key] = HIGH;
  loop(); run(50);
}
void tap(uint8_t key) { pin(key,true); run(30); pin(key,false); run(30); }
void menu() {
  chord(NEW_GAME_PIN); CHECK(ruleMenuActive); CHECK(runtimeReleaseRequired);
  releaseAll(); CHECK(ruleMenuActive);
}
void dual() {
  chord(ROTATE_GENERATION_PIN); CHECK(operatingMode == MODE_DUAL && !dualRunning);
  releaseAll(); noOutputs();
}
unsigned population() {
  unsigned n=0; for (byte b : currentGen) for (uint8_t i=0;i<8;++i) n+=bitRead(b,i); return n;
}
bool physicalPixel(uint8_t x,uint8_t y) { return bitRead(lc.rows[7-x],7-y); }
bool refRule(bool alive,unsigned n,unsigned rule) {
  if (rule==0) return alive ? n==2 || n==3 : n==3;
  if (rule==1) return alive ? n<=1 : n>=2 && n<=4;
  if (rule==2) return alive ? n>=2 && n<=4 : n==3;
  if (rule==3) return alive ? n>=1 && n<=4 : n==3;
  return alive ? n==2 || n==3 || n>=5 : n==3 || n>=7;
}
void refGeneration(const byte src[8],byte dst[8],unsigned rule) {
  memset(dst,0,8);
  for (int r=0;r<8;++r) for (int c=0;c<8;++c) {
    unsigned n=0;
    for (int rr=0;rr<8;++rr) for (int cc=0;cc<8;++cc) {
      if (rr==r && cc==c) continue;
      const int dy=std::abs(rr-r), dx=std::abs(cc-c);
      if (dy<=1 && (dx<=1 || dx==7)) n+=bitRead(src[rr],cc);
    }
    if (refRule(bitRead(src[r],c),n,rule)) bitSet(dst[r],c);
  }
}
void externalRise() { pin(CLOCK_IN_PIN,true); run(5); }
void externalFall() { run(5); pin(CLOCK_IN_PIN,false); run(5); }

int main() {
  cold(); CHECK(golRuleIndex == RULE_CLASSIC);
  const unsigned birth[5]={8,28,8,8,392}, survival[5]={12,3,28,30,492};
  for (unsigned rule=0;rule<5;++rule) {
    CHECK(golBirthMask(rule)==birth[rule] && golSurvivalMask(rule)==survival[rule]);
    golRuleIndex=rule;
    for (unsigned bits=0;bits<512;++bits) {
      byte src[8]={},dst[8]={}; unsigned k=0,n=0;
      for (int dr=-1;dr<=1;++dr) for (int dc=-1;dc<=1;++dc,++k) if (bits&(1U<<k)) {
        bitSet(src[3+dr],3+dc); if (dr || dc) ++n;
      }
      computeNextGenerationInto(src,dst);
      CHECK(bool(bitRead(dst[3],3))==refRule(bits&(1U<<4),n,rule));
    }
    for (unsigned i=0;i<500;++i) {
      byte src[8],expected[8],actual[8]; for (auto &v:src) v=random(256);
      refGeneration(src,expected,rule); computeNextGenerationInto(src,actual);
      CHECK(memcmp(expected,actual,8)==0);
    }
  }
  // La modalita' Life handheld conserva B3/S23 anche scegliendo la regola custom.
  byte single[8]={},classic[8]={}; single[3]=8;
  golRuleIndex=RULE_SINCRIA; computeNextGenerationInto(single,classic); CHECK(classic[3]==8);
  computeToroidalGenerationInto(single,classic); CHECK(!anyLiveCell(classic));

  // ALT premuto in anticipo: la combo ripristina il latch di prima della pressione.
  cold(); pin(ALT_MODES_PIN,true); run(250); CHECK(altControlsActive);
  pin(NEW_GAME_PIN,true); run(2); CHECK(ruleMenuActive && !altControlsActive);
  releaseAll(); tap(NEW_GAME_PIN); releaseAll(); CHECK(!altControlsActive);
  cold(); altControlsActive=true; digitalWrite(ALT_MODE_LED_PIN,HIGH);
  pin(ALT_MODES_PIN,true); run(250); CHECK(!altControlsActive);
  pin(ROTATE_GENERATION_PIN,true); run(2);
  CHECK(operatingMode==MODE_DUAL && altControlsActive); releaseAll();
  chord(ROTATE_GENERATION_PIN); releaseAll(); CHECK(operatingMode==MODE_EURORACK && altControlsActive);

  cold(); currentGen[0]=0x81; currentGen[7]=0x12; markManualEdit(); currentStep=37;
  byte saved[8]; memcpy(saved,currentGen,8); const uint32_t rngBefore=rng;
  // La combo deve sopprimere NEW gia' a 2 ms, prima del debounce ALT di 50 ms.
  pinLevels[ALT_MODES_PIN]=pinLevels[NEW_GAME_PIN]=HIGH; loop(); run(2);
  CHECK(!ruleMenuActive && currentStep==37 && memcmp(saved,currentGen,8)==0 && rng==rngBefore);
  run(48); CHECK(ruleMenuActive && !altControlsActive && memcmp(saved,currentGen,8)==0);
  run(1200); CHECK(ruleMenuActive && !altControlsActive && rng==rngBefore); noOutputs();
  releaseAll();
  renderRuleMenu(ruleMenuBlinkAt+250);
  for (unsigned y=0;y<8;++y) for (unsigned x=0;x<8;++x) CHECK(physicalPixel(x,y)==(y==0 && x<5));
  renderRuleMenu(ruleMenuBlinkAt+100); CHECK(!physicalPixel(0,0) && physicalPixel(1,0));
  tap(SPAWN_CELL_PIN); CHECK(ruleMenuCandidate==4);
  tap(LOCK_GENERATION_PIN); CHECK(ruleMenuCandidate==3);
  tap(KILL_CELL_PIN); CHECK(ruleMenuCandidate==4);
  tap(ROTATE_GENERATION_PIN); CHECK(ruleMenuCandidate==0);
  pin(KILL_CELL_PIN,true); run(30); run(1000); CHECK(ruleMenuCandidate==1);
  pin(KILL_CELL_PIN,false); run(30);
  // Anche un clock mantenuto o un treno di clock non naviga il menu.
  const uint8_t selected=ruleMenuCandidate;
  for (unsigned i=0;i<20;++i) { pin(CLOCK_IN_PIN,true); run(100); pin(CLOCK_IN_PIN,false); run(100); }
  CHECK(ruleMenuCandidate==selected && currentStep==37); noOutputs();
  const uint32_t beforeConfirm=rng; tap(NEW_GAME_PIN);
  CHECK(!ruleMenuActive && golRuleIndex==1 && currentStep==0 && anyLiveCell(currentGen));
  CHECK(historyCount==0 && oscillationPeriod==0 && !holdEmptyGrid && rng!=beforeConfirm);
  releaseAll(); CHECK(!altControlsActive);
  // Il tasto di conferma tenuto non genera altre popolazioni all'uscita.
  menu(); pin(NEW_GAME_PIN,true); run(30); CHECK(!ruleMenuActive && runtimeReleaseRequired);
  const uint32_t afterConfirm=rng; run(1500); CHECK(rng==afterConfirm); releaseAll();

  // ALT+KILL breve non naviga/cancella; lungo applica e svuota anche i pixel cursore.
  menu(); tap(ROTATE_GENERATION_PIN); CHECK(ruleMenuCandidate==2);
  chord(KILL_CELL_PIN); run(900); CHECK(ruleMenuActive && anyLiveCell(currentGen));
  releaseAll(); CHECK(ruleMenuActive && ruleMenuCandidate==2);
  chord(KILL_CELL_PIN); run(999); CHECK(ruleMenuActive);
  run(1); CHECK(!ruleMenuActive && holdEmptyGrid && golRuleIndex==2 && !anyLiveCell(currentGen));
  for (byte row:lc.rows) CHECK(row==0);
  run(500); CHECK(!anyLiveCell(currentGen)); releaseAll(); clearRiseCounts();
  // Due giri completi di clock: nessuna rigenerazione, LIVE o END; CLOCK funziona.
  for (unsigned i=0;i<128;++i) { externalRise(); externalFall(); }
  CHECK(!anyLiveCell(currentGen) && holdEmptyGrid && rises[CLOCK_OUT_PIN]==128);
  CHECK(rises[LIVE_CELL_OUT_PIN]==0 && rises[GAME_END_OUT_PIN]==0);
  for (byte row:lc.rows) CHECK(row==0);
  tap(SPAWN_CELL_PIN); CHECK(population()==1 && !holdEmptyGrid);

  // Entrata Dual non ruota il GoL; pattern, storia, regola e step sono preservati.
  cold(); golRuleIndex=3; currentGen[2]=17; currentGen[6]=83; markManualEdit();
  currentStep=42; historyCount=2; oscillationPeriod=2; generationLocked=true;
  memcpy(saved,currentGen,8); dual();
  CHECK(memcmp(saved,currentGen,8)==0 && golRuleIndex==3 && currentStep==42);
  CHECK(historyCount==2 && oscillationPeriod==2 && generationLocked);
  CHECK(dualLength[0]==32 && dualLength[1]==32 && dualPattern[0]==0 && dualPattern[1]==0);
  // Bounce raw prima del rilascio debounced non inverte nuovamente la modalita'.
  chord(LOCK_GENERATION_PIN); CHECK(dualRunning);
  pin(LOCK_GENERATION_PIN,false); run(1); pin(LOCK_GENERATION_PIN,true); run(2);
  CHECK(dualRunning); releaseAll();
  chord(LOCK_GENERATION_PIN); CHECK(!dualRunning); releaseAll();
  // Navigazione a toro: ROTATE destra, NEW toggle, A4 esclusivamente clock.
  tap(ROTATE_GENERATION_PIN); CHECK(dualCursorX==1 && dualCursorY==0);
  tap(SPAWN_CELL_PIN); CHECK(dualCursorX==0);
  tap(SPAWN_CELL_PIN); CHECK(dualCursorX==7);
  tap(LOCK_GENERATION_PIN); CHECK(dualCursorY==7);
  tap(KILL_CELL_PIN); CHECK(dualCursorY==0);
  tap(NEW_GAME_PIN); CHECK(dualPattern[0]==((uint32_t)1<<7));
  tap(NEW_GAME_PIN); CHECK(dualPattern[0]==0);
  dualCursorX=3; dualCursorY=4;
  pin(NEW_GAME_PIN,true); run(29); CHECK(dualPattern[1]==0);
  run(1); CHECK(dualPattern[1]==8); run(1000); CHECK(dualPattern[1]==8);
  pin(NEW_GAME_PIN,false); run(30);
  for (unsigned i=0;i<10;++i) { pin(CLOCK_IN_PIN,true); run(100); pin(CLOCK_IN_PIN,false); run(100); }
  CHECK(dualCursorX==3 && dualCursorY==4 && dualPosition[0]==0 && dualPosition[1]==0); noOutputs();
  const uint8_t xBefore=dualCursorX;
  chord(SPAWN_CELL_PIN); CHECK(dualLength[1]==4 && dualLength[0]==32 && dualCursorX==xBefore);
  releaseAll();
  for (unsigned lane=0;lane<2;++lane) for (unsigned length=1;length<=32;++length) {
    dualCursorX=(length-1)%8; dualCursorY=lane*4+(length-1)/8;
    setDualLengthAtCursor(fakeTime); CHECK(dualLength[lane]==length);
  }
  // Mapping di tutti i 64 step e conservazione degli step oltre la lunghezza.
  dualLength[0]=dualLength[1]=32; dualLengthPreview=false; dualRunning=false;
  for (unsigned lane=0;lane<2;++lane) for (unsigned step=0;step<32;++step) {
    dualPattern[0]=dualPattern[1]=0; dualPattern[lane]=(uint32_t)1<<step;
    renderDualSequencer(dualCursorAt+300);
    for (unsigned y=0;y<8;++y) for (unsigned x=0;x<8;++x) {
      CHECK(physicalPixel(x,y)==(y==lane*4+step/8 && x==step%8));
    }
  }
  dualPattern[0]=0x80000001U; dualLength[0]=4;
  renderDualSequencer(dualCursorAt+300); CHECK(!physicalPixel(7,3) && physicalPixel(0,0));
  dualLength[0]=32; renderDualSequencer(dualCursorAt+300); CHECK(physicalPixel(7,3));
  CHECK(dualPattern[0]==0x80000001U);

  // Clock comune, lunghezze indipendenti, due uscite gate con ritmi polimetrici.
  dualPattern[0]=5; dualPattern[1]=18; dualLength[0]=3; dualLength[1]=5;
  dualPosition[0]=dualPosition[1]=0; dualCursorX=2; dualCursorY=2;
  chord(LOCK_GENERATION_PIN); CHECK(dualRunning); releaseAll(); clearRiseCounts();
  unsigned expected1=0,expected2=0;
  for (unsigned tick=0;tick<150;++tick) {
    const bool want1=(5U>>(tick%3))&1, want2=(18U>>(tick%5))&1;
    externalRise(); expected1+=want1; expected2+=want2;
    CHECK(pinLevels[CLOCK_OUT_PIN]==HIGH);
    CHECK(bool(pinLevels[LIVE_CELL_OUT_PIN])==want1 && bool(pinLevels[GAME_END_OUT_PIN])==want2);
    CHECK(dualPosition[0]==(tick+1)%3 && dualPosition[1]==(tick+1)%5);
    CHECK(dualCursorX==2 && dualCursorY==2 && memcmp(saved,currentGen,8)==0);
    externalFall(); noOutputs();
  }
  CHECK(rises[CLOCK_OUT_PIN]==150 && rises[LIVE_CELL_OUT_PIN]==int(expected1) && rises[GAME_END_OUT_PIN]==int(expected2));
  // Reset comune: niente random, edit o gate extra dalla combo NEW.
  const uint32_t pattern0=dualPattern[0],pattern1=dualPattern[1];
  dualPosition[0]=2; dualPosition[1]=4; chord(NEW_GAME_PIN);
  CHECK(dualPosition[0]==0 && dualPosition[1]==0 && dualPattern[0]==pattern0 && dualPattern[1]==pattern1);
  releaseAll();
  // Pausa e ripresa su un jack gia' alto: nessun tick fantasma.
  chord(LOCK_GENERATION_PIN); CHECK(!dualRunning); releaseAll();
  pin(CLOCK_IN_PIN,true); run(100); chord(LOCK_GENERATION_PIN); CHECK(dualRunning);
  releaseAll(); run(100); CHECK(dualPosition[0]==0 && dualPosition[1]==0);
  pin(CLOCK_IN_PIN,false); run(5); externalRise(); CHECK(dualPosition[0]==1 && dualPosition[1]==1); externalFall();
  // Gli edit restano possibili in marcia; GATE IN non diventa mai destra.
  const uint8_t px=dualCursorX,py=dualCursorY;
  tap(NEW_GAME_PIN); CHECK(dualPattern[0]==(pattern0 ^ ((uint32_t)1<<18)));
  CHECK(dualCursorX==px && dualCursorY==py);
  chord(ROTATE_GENERATION_PIN); CHECK(operatingMode==MODE_EURORACK); releaseAll();
  CHECK(memcmp(saved,currentGen,8)==0 && currentStep==42 && golRuleIndex==3);
  CHECK(historyCount==2 && oscillationPeriod==2 && generationLocked);
  noOutputs();

  // INT: stesso intervallo musicale e impulsi da 10 ms, anche con combo tenuta.
  cold(); pinLevels[A1]=HIGH; pinLevels[A2]=LOW; run(50); dual();
  dualPattern[0]=dualPattern[1]=1; dualLength[0]=dualLength[1]=1;
  CHECK(stepInterval==500); chord(LOCK_GENERATION_PIN); clearRiseCounts();
  run(499); CHECK(rises[CLOCK_OUT_PIN]==0); run(1); CHECK(rises[CLOCK_OUT_PIN]==1);
  CHECK(pinLevels[LIVE_CELL_OUT_PIN]==HIGH && pinLevels[GAME_END_OUT_PIN]==HIGH);
  run(9); CHECK(pinLevels[GAME_END_OUT_PIN]==HIGH); run(1); noOutputs();
  run(990); CHECK(rises[CLOCK_OUT_PIN]==3); releaseAll();
  chord(LOCK_GENERATION_PIN); CHECK(!dualRunning); releaseAll(); clearRiseCounts(); run(2000);
  CHECK(rises[CLOCK_OUT_PIN]==0); noOutputs();

  // Clear-menu e clock Dual a cavallo del rollover uint32_t.
  cold(UINT32_MAX-1000); menu(); chord(KILL_CELL_PIN); run(1000);
  CHECK(holdEmptyGrid && !ruleMenuActive && !anyLiveCell(currentGen)); releaseAll();
  dual(); dualPattern[0]=dualPattern[1]=1; dualLength[0]=dualLength[1]=2;
  dualRunning=true; clockMode=CLOCK_INTERNAL; switchCandidate=CLOCK_INTERNAL;
  pinLevels[A1]=HIGH; pinLevels[A2]=LOW; potValue=1023; updateBPM();
  fakeTime=UINT32_MAX-10; lastStepTime=fakeTime; clearRiseCounts();
  run(49); CHECK(dualPosition[0]==0); run(1); CHECK(dualPosition[0]==1 && dualPosition[1]==1);
  CHECK(rises[LIVE_CELL_OUT_PIN]==1 && rises[GAME_END_OUT_PIN]==1); run(10); noOutputs();
  CHECK(holdEmptyGrid && !anyLiveCell(currentGen));

  std::printf("PASS rules+dual: %u assertions; actual firmware, simulated electrical I/O.\n",checks);
}
