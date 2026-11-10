#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>
#include <climits>
#define __AVR_ATmega328P__ 1
#define ARDUINO_AVR_NANO 1
#include <Arduino.h>
uint32_t fakeTime=0, rng=1;
int pinLevels[22]={}, pinModes[22], rises[22]={}, potValue=114;
#include "../SINCRIA_Scansione_Righe.ino"
void setupForLegacyTest() {
 setup();
 bootSelecting=false;
 operatingMode=MODE_EURORACK;
 initializeEurorack();
}

unsigned checks=0;
#define CHECK(expr) do { ++checks; if(!(expr)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#expr); std::abort(); }} while(0)

void resetTest() {
 fakeTime=0; memset(pinLevels,0,sizeof pinLevels); memset(rises,0,sizeof rises);
 for(auto &m:pinModes)m=-1;
 memset(buttonRaw,0,sizeof buttonRaw); memset(buttonDown,0,sizeof buttonDown);
 memset(buttonPressed,0,sizeof buttonPressed);memset(buttonHasTriggered,0,sizeof buttonHasTriggered);memset(buttonAcceptedAt,0,sizeof buttonAcceptedAt); memset(buttonChangedAt,0,sizeof buttonChangedAt);
 memset(pulseActive,0,sizeof pulseActive); memset(pulseStartedAt,0,sizeof pulseStartedAt);
 currentStep=0; generationLocked=false; altControlsActive=false; altLongHandled=false;
 displayCacheValid=false; displayDirty=true; historyCount=0; oscillationPeriod=0;
 pinLevels[A2]=HIGH; potValue=0; setupForLegacyTest();
 memset(currentGen,0,8); markManualEdit(); memset(rises,0,sizeof rises);
}
void loadBlock() {memset(currentGen,0,8); currentGen[3]=24; currentGen[4]=24; markManualEdit();}
void tick(uint32_t time) {fakeTime=time; servicePulses(fakeTime); advanceSequencer(fakeTime); refreshDisplayAtCursor();}
void driveClock(int state,uint32_t time) {fakeTime=time; pinLevels[CLOCK_IN_PIN]=state; servicePulses(time); serviceExternalClock(time);}
void button(int index,int state,uint32_t time) {fakeTime=time; pinLevels[buttonPins[index]]=state; handleInputs(time);}
void press(int index,uint32_t time) {button(index,HIGH,time); button(index,HIGH,time+50);}
void release(int index,uint32_t time) {button(index,LOW,time);button(index,LOW,time+50);}
unsigned popcountGrid() {unsigned n=0;for(auto r:currentGen)for(int j=0;j<8;++j)n+=(r>>j)&1;return n;}

int main() {
 resetTest();
 // Exact Rev2 mapping, gates remain independent, unused pins untouched.
 CHECK(clockMode==CLOCK_EXTERNAL);
 CHECK(digitalRead(NEW_GAME_PIN)==(SINCRIA_INVERTED_INPUTS ? HIGH : LOW));
 CHECK(digitalRead(ALT_MODES_PIN)==LOW);
 CHECK(!readCommandActive(NEW_GAME_PIN) && !readButtonActive(BTN_ALT));
 pinLevels[NEW_GAME_PIN]=HIGH; CHECK(readCommandActive(NEW_GAME_PIN)); pinLevels[NEW_GAME_PIN]=LOW;
 CHECK(digitalRead(CLOCK_IN_PIN)==(SINCRIA_INVERTED_INPUTS ? HIGH : LOW));
 CHECK(!readCommandActive(CLOCK_IN_PIN));
 CHECK(pinModes[3]==OUTPUT && pinModes[4]==OUTPUT && pinModes[5]==OUTPUT);
 CHECK(pinModes[0]==-1 && pinModes[1]==-1 && pinModes[13]==-1 && pinModes[20]==-1 && pinModes[21]==-1);
 std::set<int> used={DIN_PIN,CLK_PIN,CS_PIN,POT_PIN,SWITCH_INTERNAL_CLOCK_PIN,SWITCH_EXTERNAL_CLOCK_PIN,
 NEW_GAME_PIN,LIVE_CELL_OUT_PIN,GAME_END_OUT_PIN,CLOCK_OUT_PIN,LOCK_GENERATION_PIN,CLOCK_IN_PIN,
 ROTATE_GENERATION_PIN,ALT_MODES_PIN,SPAWN_CELL_PIN,KILL_CELL_PIN,ALT_MODE_LED_PIN};
 CHECK(used.size()==17);
 CHECK(ROTATE_GENERATION_PIN==7 && SPAWN_CELL_PIN==8 && KILL_CELL_PIN==A3 && CLOCK_IN_PIN==A4);
 for(unsigned step=0;step<64;++step) {
  currentStep=step; CHECK(getCurrentRow()==7-step%8 && getCurrentCol()==7-step/8);
  // Montaggio confermato: asse r del registro -> X fisico, asse c -> Y fisico.
  CHECK(7-getCurrentCol()==step/8 && 7-getCurrentRow()==step%8);
 }
 currentStep=0;
 for(uint8_t r=0;r<8;++r)for(uint8_t c=0;c<8;++c){
  memset(currentGen,0,8);CHECK(setCellDisp(r,c,true));CHECK(currentGen[c]==(1U<<(7-r)));CHECK(getCellDisp(r,c));
  CHECK(!setCellDisp(r,c,true));CHECK(setCellDisp(r,c,false));CHECK(!anyLiveCell(currentGen));
 }
 // Complete truth table of the local B3/S23 rule, independent expected result.
 for(unsigned bits=0;bits<512;++bits){
  byte src[8]={},dst[8]={};unsigned n=0,k=0;
  for(int dr=-1;dr<=1;++dr)for(int dc=-1;dc<=1;++dc,++k)if(bits&(1U<<k)){
   src[3+dr]|=1U<<(3+dc); if(dr||dc)++n;
  }
  computeNextGenerationInto(src,dst);CHECK(bool((dst[3]>>3)&1)==(n==3 || (n==2 && (bits&(1U<<4)))));
 }
 // Edges and an independent reference on 1000 arbitrary boards.
 byte edge[8]={};edge[0]=128;CHECK(countLiveNeighborsIn(edge,0,0)==1);
 memset(edge,0,8);edge[7]=1;CHECK(countLiveNeighborsIn(edge,0,0)==0);
 for(int test=0;test<1000;++test){
  byte src[8],dst[8],expected[8]={};for(auto &v:src)v=random(256);
  for(int r=0;r<8;++r)for(int c=0;c<8;++c){int n=0;
   for(int rr=0;rr<8;++rr)for(int cc=0;cc<8;++cc){
    if(r==rr&&c==cc)continue;
    int dy=r-rr;if(dy<0)dy=-dy;int dx=c-cc;if(dx<0)dx=-dx;
    if(dy<=1&&(dx<=1||dx==7))n+=(src[rr]>>cc)&1;
   }
   if(n==3 || (n==2 && ((src[r]>>c)&1))) expected[r]|=1U<<c;
  }
  computeNextGenerationInto(src,dst);CHECK(memcmp(dst,expected,8)==0);
 }
 // Every cell exactly once, first/last and gate alignment, visible next cursor.
 resetTest();generationLocked=true;std::set<int> positions;
 for(unsigned step=0;step<64;++step){
  positions.insert(getCurrentCol()*8+7-getCurrentRow());
  memset(currentGen,0,8);setCellDisp(7-step%8,7-step/8,true);
  tick(20U*(step+1));CHECK(pinLevels[3]==HIGH);CHECK(currentStep==(step+1)%64);
  for(uint8_t r=0;r<8;++r)for(uint8_t c=0;c<8;++c){
   bool want=getCellDisp(r,c)^(r==getCurrentRow()&&c==getCurrentCol());
   CHECK(bool((lc.rows[r]>>c)&1)==want);
  }
 }
 CHECK(positions.size()==64 && rises[3]==64 && rises[5]==64 && rises[4]==0);
 // Still life and blinker integration: detect, scan repeat, then END.
 resetTest();loadBlock();byte saved[8];memcpy(saved,currentGen,8);
 for(int i=0;i<64;++i)tick((i+1)*20);
 CHECK(oscillationPeriod==1 && rises[4]==0 && memcmp(saved,currentGen,8)==0);
 for(int i=64;i<128;++i)tick((i+1)*20);
 CHECK(rises[4]==1 && oscillationPeriod==0 && historyCount==0 && anyLiveCell(currentGen));
 resetTest();currentGen[3]=28;markManualEdit();
 for(int i=0;i<128;++i)tick((i+1)*20);
 CHECK(oscillationPeriod==2 && rises[4]==0);
 for(int i=128;i<192;++i)tick((i+1)*20);
 CHECK(rises[4]==1);
 // Exact minimum period comparator, including synthetic period-3/4 histories.
 for(uint8_t p=1;p<=4;++p){
  clearHistory();memset(currentGen,0,8);currentGen[0]=99;
  for(uint8_t k=0;k<p;++k){byte state[8]={};state[0]=(k==0?99:k);pushHistory(state);}
  CHECK(detectOscillationPeriod()==p);
 }
 clearHistory();historyCount=4;memset(currentGen,255,8);CHECK(detectOscillationPeriod()==0);
 // Extinction predicted once at a scan boundary; LOCK dominates every end state.
 resetTest();currentGen[4]=16;markManualEdit();currentStep=63;tick(20);
 CHECK(rises[4]==1 && anyLiveCell(currentGen) && currentStep==0);
 for(int kind=0;kind<3;++kind){
  resetTest();if(kind!=0)loadBlock();if(kind==2)oscillationPeriod=4;
  generationLocked=true;memcpy(saved,currentGen,8);
  for(int i=0;i<128;++i)tick((i+1)*20);
  CHECK(memcmp(saved,currentGen,8)==0 && rises[4]==0 && historyCount==0);
 }
 // Manual changes invalidate all oscillator history, no-op does not.
 resetTest();loadBlock();historyCount=4;oscillationPeriod=2;rotateGeneration();
 CHECK(historyCount==4);editRandomCell(true);CHECK(historyCount==0 && oscillationPeriod==0);
 unsigned before=popcountGrid();editRandomCell(false);CHECK(popcountGrid()==before-1);
 memset(currentGen,255,8);editRandomCell(true);CHECK(popcountGrid()==64);
 memset(currentGen,0,8);editRandomCell(false);CHECK(popcountGrid()==0);
 // Transform identity and preserved original direction.
 resetTest();currentGen[2]=8;markManualEdit();memcpy(saved,currentGen,8);
 for(int k=0;k<4;++k)rotateGeneration();CHECK(memcmp(saved,currentGen,8)==0);
 shiftCellsRight();CHECK(currentGen[2]==4);shiftCellsUp();CHECK(currentGen[1]==4);
 // Gate expiry, retrigger, independent pins, unsigned 32-bit rollover.
 resetTest();startPulse(PULSE_LIVE,100);servicePulses(109);CHECK(pinLevels[3]==HIGH);
 servicePulses(110);CHECK(pinLevels[3]==LOW);
 startPulse(PULSE_CLOCK,200);startPulse(PULSE_END,205);startPulse(PULSE_CLOCK,208);
 servicePulses(215);CHECK(pinLevels[4]==LOW && pinLevels[5]==HIGH);
 servicePulses(218);CHECK(pinLevels[5]==LOW);
 startPulse(PULSE_LIVE,UINT32_MAX-4);servicePulses(4);CHECK(pinLevels[3]==HIGH);
 servicePulses(5);CHECK(pinLevels[3]==LOW);
 // Input clock qualification: 5ms high/low, glitches, held high and rearm.
 resetTest();generationLocked=true;
 driveClock(HIGH,100);driveClock(HIGH,104);CHECK(currentStep==0);
 driveClock(HIGH,105);CHECK(currentStep==1);driveClock(HIGH,200);CHECK(currentStep==1);
 driveClock(LOW,201);driveClock(HIGH,205);driveClock(HIGH,210);CHECK(currentStep==1);
 driveClock(LOW,220);driveClock(LOW,225);driveClock(HIGH,230);driveClock(HIGH,235);CHECK(currentStep==2);
 // Changing mode onto a HIGH input never creates a new rising edge.
 pinLevels[A1]=HIGH;pinLevels[A2]=LOW;updateClockMode(300);updateClockMode(350);
 CHECK(clockMode==CLOCK_INTERNAL && lastStepTime==350);
 pinLevels[A1]=LOW;pinLevels[A2]=HIGH;updateClockMode(400);updateClockMode(450);
 CHECK(clockMode==CLOCK_EXTERNAL);driveClock(HIGH,500);CHECK(currentStep==2);
 driveClock(LOW,510);driveClock(LOW,515);driveClock(HIGH,520);driveClock(HIGH,525);CHECK(currentStep==3);
 // Invalid selector interval breaks the 50ms candidate qualification.
 pinLevels[A1]=HIGH;pinLevels[A2]=LOW;updateClockMode(600);
 pinLevels[A1]=LOW;updateClockMode(640);pinLevels[A1]=HIGH;updateClockMode(650);updateClockMode(690);
 CHECK(clockMode==CLOCK_EXTERNAL);updateClockMode(700);CHECK(clockMode==CLOCK_INTERNAL);
 // Button debounce/ALT single long toggle, no autorepeat, cursor editing, KILL priority.
 resetTest();press(BTN_ALT,100);handleInputs(349);CHECK(!altControlsActive);
 handleInputs(350);CHECK(altControlsActive && pinLevels[A5]==HIGH);
 handleInputs(700);CHECK(altControlsActive);release(BTN_ALT,710);
 press(BTN_SPAWN,800);CHECK(getCellDisp(7,7));CHECK(historyCount==0);
 tick(900);CHECK(currentStep==1 && pinLevels[3]==HIGH);
 tick(920);CHECK(getCellDisp(6,7) && currentStep==2);
 press(BTN_KILL,1000);CHECK(!getCellDisp(5,7));tick(1070);CHECK(pinLevels[3]==LOW);
 // Release in another mode cannot leave a paint latch behind.
 altControlsActive=false;release(BTN_SPAWN,1100);release(BTN_KILL,1160);altControlsActive=true;
 memset(currentGen,0,8);currentStep=3;tick(1250);CHECK(!getCellDisp(4,7));
 // Normal single press adds exactly one cell even when held.
 resetTest();generationLocked=true;press(BTN_SPAWN,100);CHECK(popcountGrid()==1);
 handleInputs(500);handleInputs(1000);CHECK(popcountGrid()==1);
 release(BTN_SPAWN,1100);press(BTN_SPAWN,1200);CHECK(popcountGrid()==2);
 // ALT NEW preview remains the intentional, unclocked exception.
 altControlsActive=true;setCellDisp(7,7,true);currentStep=23;int prior=rises[5];
 press(BTN_NEW,1400);CHECK(currentStep==0 && pinLevels[3]==HIGH && rises[5]==prior);
 // Internal BPM limits and first step, initialization in EXT and invalid selector.
 resetTest();potValue=0;updateBPM();CHECK(BPM==30 && stepInterval==500);
 potValue=1023;updateBPM();CHECK(BPM==300 && stepInterval==50);
 pinLevels[A1]=HIGH;pinLevels[A2]=LOW;updateClockMode(900);updateClockMode(950);generationLocked=true;lastStepTime=1000;fakeTime=1049;loop();CHECK(currentStep==0);
 fakeTime=1050;loop();CHECK(currentStep==1);
 resetTest();pinLevels[A2]=LOW;setupForLegacyTest();CHECK(clockMode==CLOCK_EXTERNAL);
 // Regression: literal physical pins must match the printed labels.
 resetTest();generationLocked=true;currentGen[2]=8;markManualEdit();
 pinLevels[7]=HIGH;handleInputs(100);handleInputs(102);
 CHECK(currentGen[3]==32 && popcountGrid()==1 && currentStep==0); // ROTATE, never clock
 pinLevels[7]=LOW;handleInputs(110);handleInputs(112);
 pinLevels[8]=HIGH;handleInputs(200);handleInputs(202);CHECK(popcountGrid()==2 && currentStep==0); // SPAWN
 pinLevels[8]=LOW;handleInputs(210);handleInputs(212);
 pinLevels[A3]=HIGH;handleInputs(300);handleInputs(302);CHECK(popcountGrid()==1 && currentStep==0); // KILL
 pinLevels[A3]=LOW;handleInputs(310);handleInputs(312);
 pinLevels[A4]=HIGH;serviceExternalClock(400);serviceExternalClock(405);
 CHECK(currentStep==1 && popcountGrid()==1); // GATE IN, never KILL
 // 10ms jack pulses accepted by all five command channels; no 50ms wait.
 for(uint8_t index=BTN_NEW;index<=BTN_KILL;++index) {
  resetTest();pinLevels[buttonPins[index]]=HIGH;updateButtons(100);CHECK(!buttonPressed[index]);
  updateButtons(101);CHECK(!buttonPressed[index]);updateButtons(102);CHECK(buttonPressed[index]);
  updateButtons(103);CHECK(!buttonPressed[index]);
  pinLevels[buttonPins[index]]=LOW;updateButtons(110);updateButtons(112);CHECK(!buttonDown[index]);
 }
 // A 1ms glitch is rejected, rebound within 30ms is consumed, hold never repeats.
 resetTest();pinLevels[6]=HIGH;handleInputs(100);pinLevels[6]=LOW;handleInputs(101);handleInputs(103);
 CHECK(!generationLocked);
 pinLevels[6]=HIGH;handleInputs(200);handleInputs(202);CHECK(generationLocked);
 pinLevels[6]=LOW;handleInputs(204);handleInputs(206);
 pinLevels[6]=HIGH;handleInputs(208);handleInputs(210);CHECK(generationLocked && !buttonPressed[BTN_LOCK]);
 handleInputs(250);CHECK(generationLocked && !buttonPressed[BTN_LOCK]);
 pinLevels[6]=LOW;handleInputs(260);handleInputs(262);
 pinLevels[6]=HIGH;handleInputs(270);handleInputs(272);CHECK(!generationLocked);
 // One-second input is still exactly one event, not auto-repeat.
 resetTest();pinLevels[8]=HIGH;handleInputs(100);handleInputs(102);CHECK(popcountGrid()==1);
 for(unsigned t=103;t<=1100;++t)handleInputs(t);
 CHECK(popcountGrid()==1);
 // Retrigger timing continues to work across the 32-bit millisecond wrap.
 resetTest();pinLevels[6]=HIGH;handleInputs(UINT32_MAX-3);handleInputs(UINT32_MAX-1);CHECK(generationLocked);
 pinLevels[6]=LOW;handleInputs(0);handleInputs(2);
 pinLevels[6]=HIGH;handleInputs(10);handleInputs(12);CHECK(generationLocked);
 pinLevels[6]=LOW;handleInputs(15);handleInputs(17);
 pinLevels[6]=HIGH;handleInputs(31);handleInputs(33);CHECK(!generationLocked);
 std::printf("PASS: %u assertions; real .ino included in native harness.\n",checks);
}
