#pragma once
#include <stdint.h>
#include <stddef.h>
using byte = uint8_t;
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define bitRead(v,b) (((v) >> (b)) & 1UL)
#define bitSet(v,b) ((v) |= (1UL << (b)))
#define bitClear(v,b) ((v) &= ~(1UL << (b)))
#define bitWrite(v,b,s) ((s) ? bitSet(v,b) : bitClear(v,b))
extern uint32_t fakeTime;
extern int pinLevels[22], pinModes[22], rises[22], potValue;
inline uint32_t millis() { return fakeTime; }
inline uint32_t micros() { return fakeTime * 1000U; }
inline int digitalRead(uint8_t p) {
#if SINCRIA_INVERTED_INPUTS
 if(p==2 || p==6 || p==7 || p==8 || p==17 || p==18) return !pinLevels[p];
#endif
 return pinLevels[p];
}
inline void digitalWrite(uint8_t p, int value) { if(value && !pinLevels[p]) ++rises[p]; pinLevels[p]=value; }
inline void pinMode(uint8_t p, int value) { pinModes[p]=value; }
inline int analogRead(uint8_t) { return potValue; }
inline long map(long x,long a,long b,long c,long d) {return (x-a)*(d-c)/(b-a)+c;}
extern uint32_t rng;
inline void randomSeed(unsigned long n) { rng=n ? (uint32_t)n : 1U; }
inline long random(long maximum) { rng=rng*1664525U+1013904223U; return (rng>>1) % maximum; }
