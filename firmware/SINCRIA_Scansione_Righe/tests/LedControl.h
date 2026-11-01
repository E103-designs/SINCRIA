#pragma once
#include <Arduino.h>
class LedControl {
public:
 byte rows[8] = {};
 LedControl(int,int,int,int) {}
 void shutdown(int,bool) {}
 void setIntensity(int,int) {}
 void clearDisplay(int) {for(auto &r: rows) r=0;}
 void setRow(int,int row,byte value) {rows[row]=value;}
};
