/////////////////////////////////////////////////////////////////////
// CHI enhanced MIDI keyboard workstation - main sketch file.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Wire.h"

#include "Cat9555.h"
#include "MidiKeySwitch.h"
#include "Ad7997.h"
#include "Switch.h"
#include "Filter.h"
#include "MidiPort.h"

// Constants
long ledBlinkInterval = 300; // ms
const long wireClockFrequency = 800000; // Hz - I2C overclocked to 800kHz

// Arduino I/O pin addresses
const int ledPin =  LED_BUILTIN; // pin address
const int joystickButtonPin =  7; // pin address
const int rotarySwPin[2] =  {8, 9}; // pin address
const int midiCableDetectPin =  6; // pin address

// Bit masks to setup the digital I/O pins of CAT9555 ports.
const uint16_t REG_RQ_PORT_CONFIG = 0xff00;
const uint16_t REG_TS_PORT_CONFIG = 0x01ff;

// Variables
bool ledState = LOW; // used to set the LED
unsigned long ledPreviousMillis = 0; // will store last time LED was updated
bool debug_mode;

// Analogue Inputs
CAd7997 AnalogA(34); // I2C address

// Digital I/O
// CAT9555 registers are designated in hardware port (i.e. little endian) order.
CCat9555 RegRQ(32, REG_RQ_PORT_CONFIG); // I2C address and port config
CCat9555 RegTS(33, REG_TS_PORT_CONFIG); // I2C address and port config

// Analog filters
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
  //      name               thresh  origin ormrgn   min   mnmrgn max   mxmrgn
  CFilter(AnalogFilterStr0,   16,     0,      32,     0,     16,  4096,  32),
  CFilter(AnalogFilterStr1,   16,     530,    32,     0,     16,  4096,  64),
  CFilter(AnalogFilterStr2,   16,     0,      32,     0,     16,  4096,  32),
  CFilter(AnalogFilterStr3,   16,     530,    32,     0,     16,  4096,  64),
  CFilter(AnalogFilterStr4,   16,     0,      32,     0,     16,  4096,  32),
  CFilter(AnalogFilterStr5,   16,     2048,   128,    0,     16,  4096,  32),
  CFilter(AnalogFilterStr6,   16,     2048,   128,    0,     16,  4096,  32),
  CFilter(AnalogFilterStr7,   16,     2048,   128,    0,     16,  4096,  32),
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

  E_UC_NUM_USECASES
};

// Regular scanning of input switches.
void scan_switches(void)
{
  // From CAT9555 ports.
  footSwitches[0].scan( //        CC   Use Case
    (RegTS.read() & 0x0080) == 0, 64,  E_UC_SIMPLE_CC);
  footSwitches[1].scan( //        CC   Use Case
    (RegTS.read() & 0x0040) == 0, 66,  E_UC_SIMPLE_CC);
  //modeSwitch.scan(
  //  (RegTS.read() & 0x0100) != 0);

  // From MCU direct ports.
  joystickButton.scan(
    digitalRead(joystickButtonPin) == LOW);
  rotarySw[0].scan(
    digitalRead(rotarySwPin[0]) == LOW);
  rotarySw[1].scan(
    digitalRead(rotarySwPin[1]) == LOW);
  midiCableDetect.scan(
    digitalRead(midiCableDetectPin) == HIGH);
}

void scan_analogs(uint8_t index)
{
  analogFilters[index].scan(AnalogA.read(index));
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

void switch_led_sync_led_states_this_column(void)
{
  const uint16_t mask = 0xe1ff;
  uint16_t val = (RegTS.read() & mask);

  if (switchLedMatrix[switch_led_scan_column_index].ledState(shiftState))     // Row 1
    val |= 0x1000;
  if (switchLedMatrix[switch_led_scan_column_index + 3].ledState(shiftState)) // Row 2
    val |= 0x0800;
  if (switchLedMatrix[switch_led_scan_column_index + 6].ledState(shiftState)) // Row 3
    val |= 0x0400;
  if (switchLedMatrix[switch_led_scan_column_index + 9].ledState(shiftState)) // Row 4
    val |= 0x0200;

  RegTS.write(val); 
}

void switch_led_next_column( void )
{
  uint16_t pattern[3] = { 0x6000, 0xa000, 0xc000 };
  const uint16_t mask = 0x1fff;
  
  // Select the column to scan.
  switch_led_scan_column_index = (switch_led_scan_column_index + 1) % 3;
  uint16_t val = (RegTS.read() & mask) | pattern[switch_led_scan_column_index];
  RegTS.write(val);
  switch_led_sync_led_states_this_column();
}

void switch_led_scan_switches_this_column(void)
{
  // LS 7bits are the CC num, MSB is the use case selector between shift and shifted.
  uint8_t ccMap[] = { 128, 106, 102, 129, 107, 103, 93, 108, 104, 92, 109, 105 };
  // Scan the rows.
  switchLedMatrix[switch_led_scan_column_index].scan(     // Row 1
    (RegTS.read() & 0x0001) != 0,
    ccMap[switch_led_scan_column_index] & 0x7f,
    (ccMap[switch_led_scan_column_index] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 3].scan( // Row 2
    (RegTS.read() & 0x0002) != 0,
    ccMap[switch_led_scan_column_index + 3] & 0x7f,
    (ccMap[switch_led_scan_column_index + 3] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 6].scan( // Row 3
    (RegTS.read() & 0x0004) != 0,
    ccMap[switch_led_scan_column_index + 6] & 0x7f,
    (ccMap[switch_led_scan_column_index + 6] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 9].scan( // Row 4
    (RegTS.read() & 0x0008) != 0,
    ccMap[switch_led_scan_column_index + 9] & 0x7f,
    (ccMap[switch_led_scan_column_index + 9] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
}

static unsigned kbd_scan_column_index = 0;

void kbd_scan_next_column_sequence( void )
{
  // Tried: 'static const PROGMEM' but it didn't work as expected.  Probably need an accessor function later.
  byte pattern[8] = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };

  // Select the column to scan.
  uint16_t val = RegRQ.read();
  CCat9555::set_lsb(val, pattern[kbd_scan_column_index]);
  RegRQ.write(val);
}

void kbd_scan_keys_this_column(void)
{
  // Scan the rows.
  keyboard[kbd_scan_column_index].scan(      // Row A
    kbd_scan_column_index,
    (RegTS.read() & 0x0010) == 0,
    (RegTS.read() & 0x0020) == 0);
  keyboard[kbd_scan_column_index + 8].scan(  // Row B
    kbd_scan_column_index + 8,
    (RegRQ.read() & 0x0100) == 0,
    (RegRQ.read() & 0x0200) == 0);
  keyboard[kbd_scan_column_index + 16].scan( // Row C
    kbd_scan_column_index + 16,
    (RegRQ.read() & 0x0400) == 0,
    (RegRQ.read() & 0x0800) == 0);
  keyboard[kbd_scan_column_index + 24].scan( // Row D
    kbd_scan_column_index + 24,
    (RegRQ.read() & 0x1000) == 0,
    (RegRQ.read() & 0x2000) == 0);
  keyboard[kbd_scan_column_index + 32].scan( // Row E
    kbd_scan_column_index + 32,
    (RegRQ.read() & 0x4000) == 0,
    (RegRQ.read() & 0x8000) == 0);

  // Advance to next column.
  kbd_scan_column_index = (kbd_scan_column_index + 1) % 8;
}

// Callbacks / hook functions.
void setLed(bool val)
{
  ledPreviousMillis = millis();
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
    default:
      break;
  }
}


// The setup() function runs once at startup.
unsigned long lastCycleTimestamp = 0;
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

  // Initialize the I2C bus.
  Wire.begin();
  Wire.setClock(wireClockFrequency);

  // Probe and initialize the CAT9555 expansion ports.
  RegRQ.begin();
  RegTS.begin();

  // Probe and initialize the AD7997 analog ports.
  AnalogA.begin();

  // Read in so we can check the mode switch at startup.
  RegTS.syncIn();
  debug_mode = ((RegTS.read() & 0x0100) != 0);

  if (!debug_mode) {
    // Use serial for MIDI
    midiJacks.begin(&setLed);
    //Serial.begin(31250);
  } else {
    // Initialize serial UART for debug output.
    Serial.begin(230400);
    Serial.println(F("Chi - stage 1 version 1"));
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

  lastCycleTimestamp = micros();
}

// The loop() function runs over and over again.

#define CYCLE_WIN_SZ 8
unsigned long cycleTimes[CYCLE_WIN_SZ] = { 0 };
unsigned long cycleTime = 0;
int cycleInd = 0;

void loop()
{
  unsigned long currentMillis = millis();
  static unsigned led = 0;
  unsigned long thisCycleTimestamp = micros();
  if (currentMillis - ledPreviousMillis >= ledBlinkInterval) {
    // save the last time you blinked the LED
    ledPreviousMillis = currentMillis;

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
      //char buf[] = { 0xFE };
      //Serial.write(buf, sizeof(buf));
    }

    // Flush output state to LED
    digitalWrite(ledPin, ledState);

    for (unsigned i = 2; i < 12; i++) {
      switchLedMatrix[switchLedSeq[i]].setLedState(i == led, E_SS_UNSHIFTED);
    }
    led++;
    led %= 12;
  }

  kbd_scan_next_column_sequence();
  switch_led_next_column();
  switch_led_sync_led_states_this_column();

  // Synchronize with the hardware
  // I2C write
  RegRQ.syncOut();
  RegTS.syncOut();
  // I2C read
  RegRQ.syncIn();
  RegTS.syncIn();

  kbd_scan_keys_this_column();
  switch_led_scan_switches_this_column();
  scan_switches();

  // Read in from the AD7997 analog ports.
  AnalogA.sync(kbd_scan_column_index);
  scan_analogs(kbd_scan_column_index);

  cycleTime -= cycleTimes[cycleInd];
  cycleTimes[cycleInd] = micros() - lastCycleTimestamp;
  cycleTime += cycleTimes[cycleInd];
  
  if (cycleInd < (CYCLE_WIN_SZ - 1))
    cycleInd++;
  else
    cycleInd = 0;
  lastCycleTimestamp = thisCycleTimestamp;
}

