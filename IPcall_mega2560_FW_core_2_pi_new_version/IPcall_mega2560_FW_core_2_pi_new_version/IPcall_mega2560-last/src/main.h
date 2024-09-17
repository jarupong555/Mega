/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#include <SPI.h>
#include <Arduino.h>
#include <ArduinoJson.h>
// #include <EEPROM.h>
#include <UIPUdp.h>
#include <UIPEthernet.h>

#include "deps/constant.h"
#include "deps/pin_constant.h"
#include "deps/message_constant.h"

#define Serial2Pi Serial3
#ifdef Serial2Pi
static uint32_t autoStartAt = 120000; // millisec(120sec)
#define Serial2PiEvent serialEvent3   // follow from Serial2Pi eg. Serial2 <-> serialEvent2
#endif

void call_debounce();
void pendant_debounce();
void callCancelSWHandler();

void pullcord_debounce();
void pullcordCancelSWHandler();

void EXcall_debounce();
// void EXcallCancelSWHandler();

void EXcall_debounce1();

#ifdef Serial2Pi
void checkStart();
void checkResetTimeCounter();
void checkCommonODL(String data);
#endif

void setInterrupt();
void stageManage();
