/////////////////////////////////////////////////////////////////////
// CHI enhanced MIDI keyboard workstation - main sketch file.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "twi_if.h"
#include "Cat9555.h"
#include "MidiKeySwitch.h"
#include "Ad7997.h"
#include "Switch.h"
#include "Filter.h"
#include "MidiPort.h"

#define FORCE_DEBUG 1

// Constants
long ledBlinkInterval = 300000; // us
const long wireClockFrequency = 800000; // Hz - I2C overclocked to 800kHz

// Arduino I/O pin addresses
const int ledPin =  LED_BUILTIN; // pin address
const int joystickButtonPin =  7; // pin address
const int rotarySwPin[2] =  {8, 9}; // pin address
const int midiCableDetectPin =  6; // pin address

// Bit masks to setup the digital I/O pins of CAT9555 ports.
const uint16_t REG_Q_PORT_CONFIG = 0x00;
const uint16_t REG_R_PORT_CONFIG = 0xff;
const uint16_t REG_S_PORT_CONFIG = 0xff;
const uint16_t REG_T_PORT_CONFIG = 0x01;

// Variables
bool ledState = LOW; // used to set the LED
uint32_t ledPreviousMicros = 0; // will store last time LED was updated
bool debug_mode;

// Analogue Inputs
CAd7997 AnalogA(34); // I2C address

// Digital I/O
// CAT9555 registers are designated in hardware port order.
CCat9555 RegQ(32, 0, REG_Q_PORT_CONFIG); // I2C address and port config
CCat9555 RegR(32, 1, REG_R_PORT_CONFIG); // I2C address and port config
CCat9555 RegS(33, 0, REG_S_PORT_CONFIG); // I2C address and port config
CCat9555 RegT(33, 1, REG_T_PORT_CONFIG); // I2C address and port config

// Analog filters
enum EAUseCase
{
  E_AUC_SIMPLE_CC = 0,
  E_AUC_SCALED_19_16_CC = 1,
  E_AUC_JOYSTICK = 2,

  E_AUC_NUM_USECASES
};

const PROGMEM char AnalogFilterStr0[] = "Trem Rate";
const PROGMEM char AnalogFilterStr1[] = "FC2";
const PROGMEM char AnalogFilterStr2[] = "Brilliance";
const PROGMEM char AnalogFilterStr3[] = "FC1";
const PROGMEM char AnalogFilterStr4[] = "Volume";
const PROGMEM char AnalogFilterStr5[] = "JoyZ";
const PROGMEM char AnalogFilterStr6[] = "JoyX";
const PROGMEM char AnalogFilterStr7[] = "JoyY";
const PROGMEM CFilter analogFilters[] =
{
  //      name               thresh  origin ormrgn   min   mnmrgn max   mxmrgn  CC   use case
  CFilter(AnalogFilterStr0,   32,     0,      0,      0,     0,   4095,  0,     90,  E_AUC_SIMPLE_CC),
  CFilter(AnalogFilterStr1,   32,     0,      594,    0,     0,   4095,  61,    4,   E_AUC_SCALED_19_16_CC),
  CFilter(AnalogFilterStr2,   32,     0,      0,      0,     0,   4095,  0,     70,  E_AUC_SIMPLE_CC),
  CFilter(AnalogFilterStr3,   32,     0,      594,    0,     0,   4095,  61,    11,  E_AUC_SCALED_19_16_CC),
  CFilter(AnalogFilterStr4,   32,     0,      0,      0,     0,   4095,  0,     7,   E_AUC_SIMPLE_CC),
  CFilter(AnalogFilterStr5,   16,     2016,   130,    0,     70,  4095,  133,   2,   E_AUC_JOYSTICK),
  CFilter(AnalogFilterStr6,   8,      2010,   100,    0,     94,  4095,  169,   0,   E_AUC_JOYSTICK),
  CFilter(AnalogFilterStr7,   8,      2044,   100,    0,     18,  4095,  25,    1,   E_AUC_JOYSTICK),
};

// Logical switch scanner.
const PROGMEM char footSwitchesStr0[] = "Damper";
const PROGMEM char footSwitchesStr1[] = "Soft";
const PROGMEM CSwitch footSwitches[] =
{
  CSwitch(footSwitchesStr0),
  CSwitch(footSwitchesStr1),
};
//const PROGMEM char modeSwitchStr[] = "Mode";
//const PROGMEM CSwitch modeSwitch(modeSwitchStr);
const PROGMEM char switchLedMatrixStr0[] = "Trans";
const PROGMEM char switchLedMatrixStr1[] = "Clav";
const PROGMEM char switchLedMatrixStr2[] = "P1";
const PROGMEM char switchLedMatrixStr3[] = "Prog";
const PROGMEM char switchLedMatrixStr4[] = "Vib";
const PROGMEM char switchLedMatrixStr5[] = "P2";
const PROGMEM char switchLedMatrixStr6[] = "Chor";
const PROGMEM char switchLedMatrixStr7[] = "EP1";
const PROGMEM char switchLedMatrixStr8[] = "P3";
const PROGMEM char switchLedMatrixStr9[] = "Trem";
const PROGMEM char switchLedMatrixStr10[] = "Ep2";
const PROGMEM char switchLedMatrixStr11[] = "Harp";
const PROGMEM CSwitch switchLedMatrix[] =
{
  CSwitch(switchLedMatrixStr0),
  CSwitch(switchLedMatrixStr1),
  CSwitch(switchLedMatrixStr2),
  CSwitch(switchLedMatrixStr3),
  CSwitch(switchLedMatrixStr4),
  CSwitch(switchLedMatrixStr5),
  CSwitch(switchLedMatrixStr6),
  CSwitch(switchLedMatrixStr7),
  CSwitch(switchLedMatrixStr8),
  CSwitch(switchLedMatrixStr9),
  CSwitch(switchLedMatrixStr10),
  CSwitch(switchLedMatrixStr11),
};
const PROGMEM char joystickButtonStr[] = "JSButton";
const PROGMEM CSwitch joystickButton(joystickButtonStr);
const PROGMEM char rotarySwStr0[] = "RotSlow";
const PROGMEM char rotarySwStr1[] = "RotFast";
const PROGMEM CSwitch rotarySw[] =
{
  CSwitch(rotarySwStr0),
  CSwitch(rotarySwStr1),
};
const PROGMEM char midiCableDetectStr[] = "MIDICable";
const PROGMEM CSwitch midiCableDetect(midiCableDetectStr);

// Logical MIDI keyboard scanning.
const PROGMEM char NoteA1Str[] = "A1";
const PROGMEM char NoteBb1Str[] = "A#/Bb1";
const PROGMEM char NoteB1Str[] = "B1";
const PROGMEM char NoteC2Str[] = "C2";
const PROGMEM char NoteDb2Str[] = "C#/Db2";
const PROGMEM char NoteD2Str[] = "D2";
const PROGMEM char NoteEb2Str[] = "D#/Eb2";
const PROGMEM char NoteE2Str[] = "E2";
const PROGMEM char NoteF2Str[] = "F2";
const PROGMEM char NoteGb2Str[] = "F#/Gb2";
const PROGMEM char NoteG2Str[] = "G2";
const PROGMEM char NoteAb2Str[] = "G#/Ab2";
const PROGMEM char NoteA2Str[] = "A2";
const PROGMEM char NoteBb2Str[] = "A#/Bb2";
const PROGMEM char NoteB2Str[] = "B2";
const PROGMEM char NoteC3Str[] = "C3";
const PROGMEM char NoteDb3Str[] = "C#/Db3";
const PROGMEM char NoteD3Str[] = "D3";
const PROGMEM char NoteEb3Str[] = "D#/Eb3";
const PROGMEM char NoteE3Str[] = "E3";
const PROGMEM char NoteF3Str[] = "F3";
const PROGMEM char NoteGb3Str[] = "F#/Gb3";
const PROGMEM char NoteG3Str[] = "G3";
const PROGMEM char NoteAb3Str[] = "G#/Ab3";
const PROGMEM char NoteA3Str[] = "A3";
const PROGMEM char NoteBb3Str[] = "A#/Bb3";
const PROGMEM char NoteB3Str[] = "B3";
const PROGMEM char NoteC4Str[] = "C4";
const PROGMEM char NoteDb4Str[] = "C#/Db4";
const PROGMEM char NoteD4Str[] = "D4";
const PROGMEM char NoteEb4Str[] = "D#/Eb4";
const PROGMEM char NoteE4Str[] = "E4";
const PROGMEM char NoteF4Str[] = "F4";
const PROGMEM char NoteGb4Str[] = "F#/Gb4";
const PROGMEM char NoteG4Str[] = "G4";
const PROGMEM char NoteAb4Str[] = "G#/Ab4";
const PROGMEM char NoteA4Str[] = "A4";
const PROGMEM char NoteBb4Str[] = "A#/Bb4";
const PROGMEM char NoteB4Str[] = "B4";
const PROGMEM char NoteC5Str[] = "C5";

const PROGMEM CMidiKeySwitch keyboard[] =
{
  CMidiKeySwitch(NoteA1Str),
  CMidiKeySwitch(NoteBb1Str),
  CMidiKeySwitch(NoteB1Str),
  CMidiKeySwitch(NoteC2Str),
  CMidiKeySwitch(NoteDb2Str),
  CMidiKeySwitch(NoteD2Str),
  CMidiKeySwitch(NoteEb2Str),
  CMidiKeySwitch(NoteE2Str),
  CMidiKeySwitch(NoteF2Str),
  CMidiKeySwitch(NoteGb2Str),
  CMidiKeySwitch(NoteG2Str),
  CMidiKeySwitch(NoteAb2Str),
  CMidiKeySwitch(NoteA2Str),
  CMidiKeySwitch(NoteBb2Str),
  CMidiKeySwitch(NoteB2Str),
  CMidiKeySwitch(NoteC3Str),
  CMidiKeySwitch(NoteDb3Str),
  CMidiKeySwitch(NoteD3Str),
  CMidiKeySwitch(NoteEb3Str),
  CMidiKeySwitch(NoteE3Str),
  CMidiKeySwitch(NoteF3Str),
  CMidiKeySwitch(NoteGb3Str),
  CMidiKeySwitch(NoteG3Str),
  CMidiKeySwitch(NoteAb3Str),
  CMidiKeySwitch(NoteA3Str),
  CMidiKeySwitch(NoteBb3Str),
  CMidiKeySwitch(NoteB3Str),
  CMidiKeySwitch(NoteC4Str),
  CMidiKeySwitch(NoteDb4Str),
  CMidiKeySwitch(NoteD4Str),
  CMidiKeySwitch(NoteEb4Str),
  CMidiKeySwitch(NoteE4Str),
  CMidiKeySwitch(NoteF4Str),
  CMidiKeySwitch(NoteGb4Str),
  CMidiKeySwitch(NoteG4Str),
  CMidiKeySwitch(NoteAb4Str),
  CMidiKeySwitch(NoteA4Str),
  CMidiKeySwitch(NoteBb4Str),
  CMidiKeySwitch(NoteB4Str),
  CMidiKeySwitch(NoteC5Str),
};

// The MIDI jacks (In and Out) on back of the keyboard.
CMidiPort<HardwareSerial> midiJacks((HardwareSerial&)Serial);

enum EUseCase
{
  E_UC_SIMPLE_CC = 0,
  E_UC_SHIFT = 1,
  E_UC_SHIFTED_CC = 2,
  E_UC_ROTARY_CC = 3,

  E_UC_NUM_USECASES
};

// Regular scanning of input switches.
void scan_switches( void )
{
  uint32_t timeS = RegS.syncdInAt();
//  uint32_t timeT = RegT.syncdInAt();
  // From CAT9555 ports.
  footSwitches[0].scan( timeS, // CC   Use Case
    (RegS.read() & 0x80) == 0,  64,  E_UC_SIMPLE_CC);
  footSwitches[1].scan( timeS, // CC   Use Case
    (RegS.read() & 0x40) == 0,  66,  E_UC_SIMPLE_CC);
  //modeSwitch.scan( timeT,
  //  (RegT.read() & 0x01) != 0);

  // From MCU direct ports.
  joystickButton.scan( micros(),
    digitalRead(joystickButtonPin) == LOW);
  rotarySw[0].scan( micros(), //      Lf/Rt  Use Case
    digitalRead(rotarySwPin[0]) == LOW, 0,   E_UC_ROTARY_CC);
  rotarySw[1].scan( micros(), //      Lf/Rt  Use Case
    digitalRead(rotarySwPin[1]) == LOW, 1,   E_UC_ROTARY_CC);
  midiCableDetect.scan( micros(),
    digitalRead(midiCableDetectPin) == HIGH);
}

void scan_analogs(uint8_t index, uint32_t rTime)
{
  analogFilters[index].scan(AnalogA.read(index), rTime);
}

// Scanning of switches, updating of LEDs in the 4x3 matrix.
static unsigned switch_led_scan_column_index = 0;

enum EShiftStates
{
  E_SS_UNSHIFTED = 0,
  E_SS_PROGCH = 1,
  E_SS_TRANS = 2,
  E_SS_SYS = 3,

  E_SS_NUM_STATES
};
enum EShiftStates shiftState = E_SS_UNSHIFTED;

void switch_led_sync_led_states_this_column( void )
{
  const uint8_t mask = 0xe1;
  uint8_t val = (RegT.read() & mask);

  if (switchLedMatrix[switch_led_scan_column_index].ledState(shiftState))     // Row 1
    val |= 0x10;
  if (switchLedMatrix[switch_led_scan_column_index + 3].ledState(shiftState)) // Row 2
    val |= 0x08;
  if (switchLedMatrix[switch_led_scan_column_index + 6].ledState(shiftState)) // Row 3
    val |= 0x04;
  if (switchLedMatrix[switch_led_scan_column_index + 9].ledState(shiftState)) // Row 4
    val |= 0x02;

  RegT.write(val); 
}

void switch_led_next_column( void )
{
  uint8_t pattern[3] = { 0x60, 0xa0, 0xc0 };
  const uint8_t mask = 0x1f;
  
  // Select the column to scan.
  switch_led_scan_column_index = (switch_led_scan_column_index + 1) % 3;
  uint16_t val = (RegT.read() & mask) | pattern[switch_led_scan_column_index];
  RegT.write(val);
  switch_led_sync_led_states_this_column();
}

void switch_led_scan_switches_this_column( void )
{
  uint32_t timeS = RegS.syncdInAt();
  // LS 7bits are the CC num, MSB is the use case selector between shift and shifted.
  uint8_t ccMap[] = { 128, 106, 102, 129, 107, 103, 93, 108, 104, 92, 109, 105 };
  // Scan the rows.
  switchLedMatrix[switch_led_scan_column_index].scan( timeS,     // Row 1
    (RegS.read() & 0x01) != 0,
    ccMap[switch_led_scan_column_index] & 0x7f,
    (ccMap[switch_led_scan_column_index] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 3].scan( timeS, // Row 2
    (RegS.read() & 0x02) != 0,
    ccMap[switch_led_scan_column_index + 3] & 0x7f,
    (ccMap[switch_led_scan_column_index + 3] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 6].scan( timeS, // Row 3
    (RegS.read() & 0x04) != 0,
    ccMap[switch_led_scan_column_index + 6] & 0x7f,
    (ccMap[switch_led_scan_column_index + 6] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 9].scan( timeS, // Row 4
    (RegS.read() & 0x08) != 0,
    ccMap[switch_led_scan_column_index + 9] & 0x7f,
    (ccMap[switch_led_scan_column_index + 9] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
}

static unsigned kbd_scan_column_index = 0;

void kbd_scan_next_column_sequence( void )
{
  // Tried: 'static const PROGMEM' but it didn't work as expected.  Probably need an accessor function later.
  byte pattern[8] = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };

  // Select the column to scan.
  RegQ.write(pattern[kbd_scan_column_index]);
}

void kbd_scan_keys_this_column( void )
{
  uint32_t timeR = RegR.syncdInAt();
  uint32_t timeS = RegS.syncdInAt();
  // Scan the rows.
  keyboard[kbd_scan_column_index].scan(      // Row A
    timeS, kbd_scan_column_index,
    (RegS.read() & 0x10) == 0,
    (RegS.read() & 0x20) == 0);
  keyboard[kbd_scan_column_index + 8].scan(  // Row B
    timeR, kbd_scan_column_index + 8,
    (RegR.read() & 0x01) == 0,
    (RegR.read() & 0x02) == 0);
  keyboard[kbd_scan_column_index + 16].scan( // Row C
    timeR, kbd_scan_column_index + 16,
    (RegR.read() & 0x04) == 0,
    (RegR.read() & 0x08) == 0);
  keyboard[kbd_scan_column_index + 24].scan( // Row D
    timeR, kbd_scan_column_index + 24,
    (RegR.read() & 0x10) == 0,
    (RegR.read() & 0x20) == 0);
  keyboard[kbd_scan_column_index + 32].scan( // Row E
    timeR, kbd_scan_column_index + 32,
    (RegR.read() & 0x40) == 0,
    (RegR.read() & 0x80) == 0);

  // Advance to next column.
  kbd_scan_column_index = (kbd_scan_column_index + 1) % 8;
}

// Callbacks / hook functions.
void setLed(bool val)
{
  ledPreviousMicros = micros();
  ledState = val;
  digitalWrite(ledPin, ledState);
}

void noteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
  midiJacks.noteOn(note, velocity, channel);
}

void noteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
  midiJacks.noteOff(note, velocity, channel);
}

//                        Trans PC 1  2  3   4  5  6  7   8  Ch Trem
uint8_t switchLedSeq[12] = { 0, 3, 2, 5, 8, 11, 1, 4, 7, 10, 6, 9 };
uint8_t currentPC = 0;

void syncLedsToProgChange(void)
{
    for (unsigned i = 2; i < 10; i++) {
      switchLedMatrix[switchLedSeq[i]].setLedState(i == (currentPC + 2), E_SS_PROGCH);
    }
}

enum ERotaryStates
{
  E_RS_BRAKED = 0,
  E_RS_SLOW = 1,
  E_RS_FAST = 2,
  E_RS_BRAKE_PENDING = 3,

  E_RS_NUM_STATES
};
enum ERotaryStates rotaryState = E_RS_BRAKED;
unsigned long rotaryBrakeStart = 0;

void switchChanged(bool state, uint8_t ccNum, uint8_t uCase)
{
  uint8_t channel = CMidiKeySwitch::getMidiCh();
  uint8_t ccVal = state?127:0;
  uint8_t newPC = currentPC;
  if (ccNum > 119)
    // Note: Default value passed into an incompletely defined CSwitch scan() is 255.
    return;
  switch (uCase)
  {
    case E_UC_SIMPLE_CC: // Simple switch mapped to CC use case
      midiJacks.ctrlCh(ccNum, ccVal, channel);
      break;
    case E_UC_SHIFT:
      if (state) break; // Only process the switch release.
      switch (shiftState)
      {
        case E_SS_UNSHIFTED:
          if (ccNum) {
            // Prog Change switch pressed
            shiftState = E_SS_PROGCH;
            syncLedsToProgChange();
          } else {
            // Transpose switch pressed
            shiftState = E_SS_TRANS;
          }
          break;
        case E_SS_PROGCH:
          // Prog Change or Transpose switch pressed - cancel / exit
          shiftState = E_SS_UNSHIFTED;
          break;
        case E_SS_TRANS:
          if (ccNum) {
            // Prog Change switch pressed - go to system mode
            shiftState = E_SS_SYS;
          } else {
            // Transpose switch pressed - cancel / exit
            shiftState = E_SS_UNSHIFTED;
          }
          break;
        case E_SS_SYS:
          if (ccNum) {
            // Prog Change switch pressed - step through system mode functions
            // TODO - stuff
          } else {
            // Transpose switch pressed - cancel / exit
            shiftState = E_SS_UNSHIFTED;
          }
          break;
        default:
          shiftState = E_SS_UNSHIFTED;
          break;
      }
      break;
    case E_UC_SHIFTED_CC:
      switch (shiftState)
      {
        case E_SS_UNSHIFTED:
          // Unshifted, so this switch sends out a CC
          midiJacks.ctrlCh(ccNum, ccVal, channel);
          break;
        case E_SS_PROGCH:
#if 0
// Not sure I like having the quick PC 1-8 exit the mode.
          if (!state) {
            // Affect state change at the switch release
            if (ccNum >= 102) {
              // Exit the shifted state
              shiftState = E_SS_UNSHIFTED;
            }
            // Otherwise stay in present state because user could inc / dec multiple times.
            break;
          }
#else
          if (!state) break; // Only process the switch press.
#endif
          // Shifted as Prog Change, so this switch sends out a prog change.
          if (ccNum >= 102) {
            // One of the eight short cut PC for PC1-PC8 (0-7).
            newPC = ccNum - 102;
          } else if (ccNum == 93) {
            // Chorus - Decrement PC by one
            if (newPC > 0)
              newPC--;
          } else if (ccNum == 92) {
            // Tremelo - Increment PC by one
            if (newPC < 127)
              newPC++;
          }
          if (newPC != currentPC) {
            // Only send out a PC message if the value has changed.
            currentPC = newPC;
            midiJacks.progCh(currentPC, channel);
          }
          syncLedsToProgChange();
          break;
        // Rest of these are TBD.
        default:
          break;
      }
      break;
    case E_UC_ROTARY_CC:
      switch(rotaryState)
      {
        case E_RS_BRAKED:
          if (state) {
            if (ccNum == 0) {
              // switched to slow
              midiJacks.ctrlCh(80, 0, channel);
              midiJacks.ctrlCh(81, 0, channel);
              rotaryState = E_RS_SLOW;
            } else if (ccNum == 1) {
              // switched to fast
              midiJacks.ctrlCh(80, 127, channel);
              midiJacks.ctrlCh(81, 0, channel);
              midiJacks.ctrlCh(82, 127, channel);
              rotaryState = E_RS_FAST;
            }
          }
          break;
        case E_RS_SLOW:
          if ((ccNum == 0) && !state) {
            // switched to centre
            midiJacks.ctrlCh(80, 64, channel);
            rotaryState = E_RS_BRAKE_PENDING;
            // Start timer
            rotaryBrakeStart = micros();
          } else if ((ccNum == 1) && state) {
            // rapidly switched to fast
            midiJacks.ctrlCh(80, 127, channel);
            midiJacks.ctrlCh(82, 127, channel);
            rotaryState = E_RS_FAST;
          }
          break;
        case E_RS_FAST:
          if ((ccNum == 0) && state) {
            // rapidly switched to slow
            midiJacks.ctrlCh(80, 0, channel);
            midiJacks.ctrlCh(82, 0, channel);
            rotaryState = E_RS_SLOW;
          } else if ((ccNum == 1) && !state) {
            // switched to centre
            midiJacks.ctrlCh(80, 64, channel);
            midiJacks.ctrlCh(82, 0, channel);
            rotaryState = E_RS_BRAKE_PENDING;
            // Start timer
            rotaryBrakeStart = micros();
          }
          break;
        case E_RS_BRAKE_PENDING:
          if (ccNum = 2) {
            // Timeout
            midiJacks.ctrlCh(81, 127, channel);
            rotaryState = E_RS_BRAKED;
          } else if (state) {
            if (ccNum == 0) {
              // switched to slow
              midiJacks.ctrlCh(80, 0, channel);
              rotaryState = E_RS_SLOW;
              // Stop timer
              rotaryBrakeStart = 0;
            } else if (ccNum == 1) {
              // switched to fast
              midiJacks.ctrlCh(80, 127, channel);
              midiJacks.ctrlCh(82, 127, channel);
              rotaryState = E_RS_FAST;
              // Stop timer
              rotaryBrakeStart = 0;
            }
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}


bool joystickShifted = false;
void analogChanged(bool state, uint16_t val, uint8_t ccNum, uint8_t uCase)
{
  uint8_t channel = CMidiKeySwitch::getMidiCh();
  unsigned int ccVal = 127;
  switch (uCase)
  {
    case E_AUC_SIMPLE_CC:
      if (((val & 0xfe0) >> 5) < 127)
        ccVal = (val & 0xfe0) >> 5;
      midiJacks.ctrlCh(ccNum, ccVal, channel);
      break;
    case E_AUC_SCALED_19_16_CC:
      if (((((val + (val >> 3) + (val >> 4))) & 0xfe0) >> 5) < 127)
        ccVal = (((val + (val >> 3) + (val >> 4))) & 0xfe0) >> 5;
      midiJacks.ctrlCh(ccNum, ccVal, channel);
      break;
    case E_AUC_JOYSTICK:
      switch (ccNum)
      {
        case 0:
          // X axis (forward / backwards motion)
          if (((((val + (val >> 3))) & 0xfe0) >> 4) < 127)
            ccVal = (((val + (val >> 3))) & 0xfe0) >> 4;
          if (joystickShifted) {
            uint8_t shftVal = 64;
            if (ccVal) {
              if (state) {
                if (ccVal < 128)
                  shftVal = 64 + (ccVal >> 1);
                else
                  shftVal = 127;
              } else {
                if (ccVal <= 128)
                  shftVal = 64 - (ccVal >> 1);
                else
                  shftVal = 0;
              }
            }
            midiJacks.ctrlCh(75, shftVal, channel);
          } else {
            if (ccVal == 0) {
              // At centre position so zero both CC's.
              midiJacks.ctrlCh(1, 0, channel);
              midiJacks.ctrlCh(2, 0, channel);
            } else if (state) {
              // Forward motion - modulation
              midiJacks.ctrlCh(1, ccVal, channel);
            } else {
              // Reverse motion - breath
              midiJacks.ctrlCh(2, ccVal, channel);
            }
          }
          break;
        case 1:
          // Y axis (left / right motion)
          if (((val + (val >> 4))) < 2048)
            ccVal = ((val + (val >> 4)));
          else
            ccVal = 2048;
          if (joystickShifted) {
            // have to shift ccVal down from 0-2047 range
            uint8_t shftVal = 64;
            if (ccVal) {
              if (state) {
                if (ccVal < 2048)
                  shftVal = 64 + (ccVal >> 5);
                else
                  shftVal = 127;
              } else {
                if (ccVal <= 2048)
                  shftVal = 64 - (ccVal >> 5);
                else
                  shftVal = 0;
              }
            }
            midiJacks.ctrlCh(76, shftVal, channel);
          } else {
            // Pitch bend
            uint16_t pbVal = 8192;
            if (ccVal) {
              if (state) {
                // Right (pitch up) motion
                if (ccVal < 2048)
                  pbVal = 8192 + (ccVal << 2);
                else
                  pbVal = 16383;
              } else {
                // Left (pitch down) motion
                if (ccVal <= 2048)
                  pbVal = 8192 - (ccVal << 2);
                else
                  pbVal = 0;
              }
            }
            midiJacks.pitchBend(pbVal, channel);
          }
          break;
        case 2:
          // Z axis (twist CW / CCW motion)
          if (((((val + (val >> 3))) & 0xfe0) >> 4) < 127)
            ccVal = (((val + (val >> 3))) & 0xfe0) >> 4;
          if (joystickShifted) {
            uint8_t shftVal = 64;
            if (ccVal) {
              if (state) {
                if (ccVal < 128)
                  shftVal = 64 + (ccVal >> 1);
                else
                  shftVal = 127;
              } else {
                if (ccVal <= 128)
                  shftVal = 64 - (ccVal >> 1);
                else
                  shftVal = 0;
              }
            }
            midiJacks.ctrlCh(77, shftVal, channel);
          } else {
            if (ccVal == 0) {
              // At centre position so zero both CC's.
              midiJacks.ctrlCh(12, 0, channel);
              midiJacks.ctrlCh(13, 0, channel);
            } else if (state) {
              // CW motion
              midiJacks.ctrlCh(12, ccVal, channel);
            } else {
              // CCW motion
              midiJacks.ctrlCh(13, ccVal, channel);
            }
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

// Received control change messages only sync up LEDs - useful for Mainstage templates.
void handleCtrlCh(uint8_t ccNum, uint8_t ccVal, uint8_t channel)
{
  if (ccNum == 93) {
    // Chorus
    switchLedMatrix[switchLedSeq[10]].setLedState(ccVal >= 64, E_SS_UNSHIFTED);
  } else if (ccNum == 92) {
    // Tremolo
    switchLedMatrix[switchLedSeq[11]].setLedState(ccVal >= 64, E_SS_UNSHIFTED);
  } else if ((ccNum >= 102) && (ccNum <= 109)) {
    // Parts 1 to 8
    switchLedMatrix[switchLedSeq[ccNum - 102 + 2]].setLedState(ccVal >= 64, E_SS_UNSHIFTED);
  } else if (ccNum == 121) {
    // Reset all controllers
    for (unsigned i = 2; i < 12; i++) {
      switchLedMatrix[switchLedSeq[i]].setLedState(false, E_SS_UNSHIFTED);
    }
  }
}

// Received program change messages only sync up state - useful for Mainstage templates.
void handleProgCh(uint8_t pcVal, uint8_t channel)
{
  currentPC = pcVal;
  syncLedsToProgChange();
}

#if 0
enum I2C_Thread_States
{
  ITS_IDLE = 0,
  ITS_LEDSW_T_OUT,
  ITS_LEDSW_S_IN,
  ITS_KEYBD_Q_OUT,
  ITS_KEYBD_R_IN,
  ITS_KEYBD_S_IN,
  ITS_ANALOG_IN,

  ITS_NUM_STATES
};

enum I2C_Reg_States
{
  IRS_IDLE = 0,
  IRS_READY,
  IRS_ISR_BUSY,
  IRS_ISR_DONE,

  IRS_NUM_STATES
};

enum I2C_Misc_Parms
{
  LEDSW_NUM_COL = 3,
  KEYBD_NUM_COL = 8,
  ANALOG_NUM_CH = 8,

  REG_QR_I2C_ADDR = 32,
  REG_ST_I2C_ADDR = 33,
  ANALOG_I2C_ADDR = 34,
};

struct RegDigIn
{
  uint32_t m_timeRead;
  uint8_t m_valueRead;
};

struct RegDigRSIn
{
  struct RegDigIn R;
  struct RegDigIn S;
  enum I2C_Reg_States m_state;
};

struct RegDigRSIn keyboardSwitchInputs[KEYBD_NUM_COL] = { 0 };
uint8_t keyboardSwitchCol = 0;
struct RegDigIn ledSwitchInputs[LEDSW_NUM_COL] = { 0 };
uint8_t ledSwitchCol = 0;
enum I2C_Thread_States i2cIsrState = ITS_IDLE;



// Handle I2C transactions
void complete(void)
{
  switch (i2cIsrState)
  {
    case ITS_LEDSW_T_OUT:
      // Done sending reg T out.
      //  - selecting LED / switch column.
      //  - setting the LED outputs for the cross section of the rows.
      // Next, read reg S, switch input values for the cross section of the rows.
//      twi_initiate_transaction(REG_ST_I2C_ADDR, uint8_t *data_wr, uint8_t bytes_wr,
//  uint8_t *data_rd, uint8_t bytes_rd, uint32_t *doneAtPtr = NULL);
      break;
    case ITS_LEDSW_S_IN:
      break;
    case ITS_KEYBD_Q_OUT:
      break;
    case ITS_KEYBD_R_IN:
      break;
    case ITS_KEYBD_S_IN:
      break;
    case ITS_ANALOG_IN:
      break;
    default:
      break;
  }
}
#endif

// The setup() function runs once at startup.
void setup()
{
  // Initialize LED for flashing.
  pinMode(ledPin, OUTPUT);

  // Digital inputs.
  pinMode(joystickButtonPin, INPUT);
  pinMode(rotarySwPin[0], INPUT);
  pinMode(rotarySwPin[1], INPUT);
  pinMode(midiCableDetectPin, INPUT);

  // Assign the internal pullup resistors.
  digitalWrite(joystickButtonPin, HIGH);
  digitalWrite(rotarySwPin[0], HIGH);
  digitalWrite(rotarySwPin[1], HIGH);
  digitalWrite(midiCableDetectPin, HIGH);

#ifdef FORCE_DEBUG
  Serial.begin(230400);
  Serial.println(F("Chi - debug"));
#endif

  // Initialize the I2C bus.
  twi_init(16000000UL, wireClockFrequency);

  // Probe and initialize the CAT9555 expansion ports.
  RegQ.begin();
  RegR.begin();
  RegS.begin();
  RegT.begin();

  // Probe and initialize the AD7997 analog ports.
  AnalogA.begin();

  // Read in so we can check the mode switch at startup.
#ifndef FORCE_DEBUG
  RegT.syncIn();
  debug_mode = ((RegT.read() & 0x01) != 0);
#else
  debug_mode = true;
#endif

  if (!debug_mode) {
    // Use serial for MIDI
    midiJacks.begin(&setLed, &handleCtrlCh, &handleProgCh);
  } else {
    // Initialize serial UART for debug output.
#ifndef FORCE_DEBUG
    Serial.begin(230400);
#endif
    Serial.println(F("Chi - stage 2 version 1"));
  }

  // Set up the shift state overlays to indicate the modes
  // E_SS_UNSHIFTED - both off
  switchLedMatrix[switchLedSeq[0]].setLedState(false, E_SS_UNSHIFTED); // Transpose LED
  switchLedMatrix[switchLedSeq[1]].setLedState(false, E_SS_UNSHIFTED); // Prog Change LED
  // E_SS_PROGCH - prog change on, transpose off
  switchLedMatrix[switchLedSeq[0]].setLedState(false, E_SS_PROGCH); // Transpose LED
  switchLedMatrix[switchLedSeq[1]].setLedState(true, E_SS_PROGCH); // Prog Change LED
  // E_SS_TRANS - transpose on, prog change off
  switchLedMatrix[switchLedSeq[0]].setLedState(true, E_SS_TRANS); // Transpose LED
  switchLedMatrix[switchLedSeq[1]].setLedState(false, E_SS_TRANS); // Prog Change LED
  // E_SS_SYS - both on
  switchLedMatrix[switchLedSeq[0]].setLedState(true, E_SS_SYS); // Transpose LED
  switchLedMatrix[switchLedSeq[1]].setLedState(true, E_SS_SYS); // Prog Change LED
}

// The loop() function runs over and over again.

#define CYCLE_WIN_SZ 32
unsigned long cycleTimes[CYCLE_WIN_SZ] = { 0 };
unsigned long cycleTime = 0;
int cycleInd = 0;

void update_cycle_time(uint32_t delta)
{
  cycleTime -= cycleTimes[cycleInd];
  cycleTimes[cycleInd] = delta;
  cycleTime += cycleTimes[cycleInd];
  
  if (cycleInd < (CYCLE_WIN_SZ - 1))
    cycleInd++;
  else
    cycleInd = 0;
}

void loop()
{
  unsigned long currentMicros = micros();
  static unsigned led = 0;
  if (currentMicros - ledPreviousMicros >= ledBlinkInterval) {
    // save the last time you blinked the LED
    ledPreviousMicros = currentMicros;

    if (debug_mode) {
      // In debug mode, the LED flashes, toggling every ledBlickInterval.
      // Toggle LED state
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
    } else {
      // Active sense
      midiJacks.activeSense();
    }

    // Flush output state to LED
    digitalWrite(ledPin, ledState);

    if (debug_mode) {
      for (unsigned i = 0; i < 12; i++) {
        switchLedMatrix[switchLedSeq[i]].setLedState(i == led, E_SS_UNSHIFTED);
      }
      led++;
      led %= 12;
      //Serial.print(F("Profile Cycle Time: "));
      //Serial.println(cycleTime / CYCLE_WIN_SZ);
    }
  }

  //uint32_t startTime = micros();
  kbd_scan_next_column_sequence();
  switch_led_next_column();
  switch_led_sync_led_states_this_column();
  //uint32_t endTime = micros();

  // Synchronize with the hardware
  // I2C write
  //uint32_t startTime = micros();
  RegQ.syncOut();
  RegR.syncOut();
  RegS.syncOut();
  RegT.syncOut();
  // I2C read
  RegQ.syncIn();
  RegR.syncIn();
  RegS.syncIn();
  RegT.syncIn();
  //uint32_t endTime = micros();

  //uint32_t startTime = micros();
  kbd_scan_keys_this_column();
  switch_led_scan_switches_this_column();
  scan_switches();
  //uint32_t endTime = micros();

  // Read in from the AD7997 analog ports.
#if 0
  AnalogA.sync(kbd_scan_column_index);
  //uint32_t startTime = micros();
  scan_analogs(kbd_scan_column_index, AnalogA.syncdAt());
  //uint32_t endTime = micros();
#else
  if (kbd_scan_column_index == 0) {
    //uint32_t startTime = micros();
    AnalogA.sync();
    uint32_t endTime = AnalogA.syncdAt();
    //update_cycle_time(endTime - startTime);
    //uint32_t startTime = micros();
    for (unsigned i = 0; i < 8; i++) {
      scan_analogs(i, endTime);
    }
    //uint32_t end2Time = micros();
    //update_cycle_time(end2Time - startTime);
  }
#endif

  //uint32_t startTime = micros();
  if (rotaryBrakeStart) {
    // Rotary brake mode timer active.
    if ((micros() - rotaryBrakeStart) > 250000) {
      // 250ms guard time before braking the rotary since the switch
      // always passes through the centre position.
      switchChanged(false, 2, E_UC_ROTARY_CC);
    }
  }

  if (analogFilters[5].atOrigin() && analogFilters[6].atOrigin() && analogFilters[7].atOrigin()) {
    // We only allow the joystick switch to shift / unshift while joystick is positioned at origin.
    joystickShifted = joystickButton.switchState();
  }

  // Check for and handle receive MIDI messages.
  midiJacks.receiveScan();
  //uint32_t endTime = micros();

  //update_cycle_time(endTime - startTime);
}

