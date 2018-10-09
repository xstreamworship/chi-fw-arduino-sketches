/////////////////////////////////////////////////////////////////////
// CHI enhanced MIDI keyboard workstation - Main MCU sketch file.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////

// The 'main' MCU is on an Arduino Mega 2560, handling primarily
//     - Keyboard scanning
//     - MIDI (serial and USB) interfaces
//     - MIDI redirection / merging
//     - LED / Switch matrix
//     - User interface logic

#if defined(__AVR_ATmega2560__)
#else
#error Unsupported platform!
#endif

#include "MidiKeySwitch.h"
#include "LedSwitch.h"
#include "MidiPort.h"

#define FORCE_DEBUG 0

#define __READ_BIT(p,b) (p & (1 << b))
#define __WRITE_BIT(p ,b, e) \
  if ( (e) ) { \
    p |= (1 << b); \
  } else { \
    p &= ~(1 << b); \
  }

#define READ_BIT(n) __READ_BIT(n##_PIN, n##_BIT)
#define WRITE_BIT(n,e) __WRITE_BIT(n##_PORT, n##_BIT, e)

// Constants
long ledBlinkInterval = 250000; // us

// Arduino I/O pin addresses
#define ARDUINO_LED_PORT PORTB
#define ARDUINO_LED_PIN PINB
#define ARDUINO_LED_BIT 7
const int ledPin =  LED_BUILTIN; // pin address

#define REAR_ENCODER_SW_PORT PORTG
#define REAR_ENCODER_SW_PIN PING
#define REAR_ENCODER_SW_BIT 2
const int rearEncoderSwPin = 39;

#define REAR_ENCODER_CLK_PORT PORTK
#define REAR_ENCODER_CLK_PIN PINK
#define REAR_ENCODER_CLK_BIT 7
const int rearEncoderClkPin = 69;

#define REAR_ENCODER_DT_PORT PORTD
#define REAR_ENCODER_DT_PIN PIND
#define REAR_ENCODER_DT_BIT 7
const int rearEncoderDtPin = 38;

#define MIDI_CABLE_DETECT_PORT PORTG
#define MIDI_CABLE_DETECT_PIN PING
#define MIDI_CABLE_DETECT_BIT 1
const int midiCableDetectPin = 40;

#define MODE_SWITCH_PORT PORTG
#define MODE_SWITCH_PIN PING
#define MODE_SWITCH_BIT 0
const int modeSwitchPin = 41;


#define DIP_SW1_PORT PORTE
#define DIP_SW1_PIN PINE
#define DIP_SW1_BIT 4
#define DIP_SW2_PORT PORTE
#define DIP_SW2_PIN PINE
#define DIP_SW2_BIT 5
#define DIP_SW3_PORT PORTG
#define DIP_SW3_PIN PING
#define DIP_SW3_BIT 5
#define DIP_SW4_PORT PORTE
#define DIP_SW4_PIN PINE
#define DIP_SW4_BIT 3
#define DIP_SW_AUX_MCU_DEBUG 2
#define DIP_SW_MAIN_MCU_DEBUG 3
const int dipSwPin[4] =  {2, 3, 4, 5}; // pin address

// Variables
bool ledState = LOW; // used to set the LED
uint32_t ledPreviousMicros = 0; // will store last time LED was updated
bool debug_mode;
bool debug_mode_aux;

const PROGMEM char rearEncoderSwStr[] = "EncoderSw";
CSwitch rearEncoderSw(rearEncoderSwStr);
const PROGMEM char modeSwitchStr[] = "Mode";
CSwitch modeSwitch(modeSwitchStr);
const PROGMEM char midiCableDetectStr[] = "MIDICable";
CSwitch midiCableDetect(midiCableDetectStr);

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
CSwitch switchLedMatrix[] =
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

CMidiKeySwitch keyboard[] =
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

// The USB connector setup as MIDI (requires change in firmware in the USB MCU).
CMidiPort<HardwareSerial> midiUSB((HardwareSerial&)Serial);

// The main MIDI-Out and MIDI-In jacks on back of the keyboard.
CMidiPort<HardwareSerial> midiJacks((HardwareSerial&)Serial1);

// The MIDI-Thru/Out2 jack on back of the keyboard and MIDI input B2B from Aux MCU's MIDI output.
CMidiPort<HardwareSerial> midiB2bThru((HardwareSerial&)Serial2);

enum EUseCase
{
  E_UC_SIMPLE_CC = 0,
  E_UC_SHIFT = 1,
  E_UC_SHIFTED_CC = 2,
  E_UC_ROTARY_CC = 3,

  E_UC_NUM_USECASES
};

// Regular scanning of input switches.
void scan_misc_switches( uint32_t timeS )
{
  rearEncoderSw.scan( timeS,
    READ_BIT(REAR_ENCODER_SW) == 0);
  modeSwitch.scan( timeS,
    READ_BIT(MODE_SWITCH) != 0);
  midiCableDetect.scan( timeS,
    READ_BIT(MIDI_CABLE_DETECT) != 0);
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

void switch_led_scan_next_column( uint32_t timeS )
{
  uint8_t pattern[3] = { 0x60, 0x50, 0x30 };
  // LS 7bits are the CC num, MSB is the use case selector between shift and shifted.
  uint8_t ccMap[] = { 128, 106, 102, 129, 107, 103, 93, 108, 104, 92, 109, 105 };

  // Scan the rows (at end of cycle before selecting nect column).
  // Read & mask the present value.
  // TODO - MSB is arduino LED - roll this into same funtion to speed things up.
  const uint8_t mask = 0x80;
  uint8_t val = PINH;
  switchLedMatrix[switch_led_scan_column_index].scan( timeS,     // Row 1
    (val & 0x08) != 0,
    ccMap[switch_led_scan_column_index] & 0x7f,
    (ccMap[switch_led_scan_column_index] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 3].scan( timeS, // Row 2
    (val & 0x10) != 0,
    ccMap[switch_led_scan_column_index + 3] & 0x7f,
    (ccMap[switch_led_scan_column_index + 3] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 6].scan( timeS, // Row 3
    (val & 0x20) != 0,
    ccMap[switch_led_scan_column_index + 6] & 0x7f,
    (ccMap[switch_led_scan_column_index + 6] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);
  switchLedMatrix[switch_led_scan_column_index + 9].scan( timeS, // Row 4
    (val & 0x40) != 0,
    ccMap[switch_led_scan_column_index + 9] & 0x7f,
    (ccMap[switch_led_scan_column_index + 9] & 0x80) != 0?E_UC_SHIFT:E_UC_SHIFTED_CC);

  // Select next column and apply the LED values (atomic operation)
  // Read & mask the present value.
  val = PORTB & mask;
  
  // Select the column to scan.
  switch_led_scan_column_index = (switch_led_scan_column_index + 1) % 3;
  val |= pattern[switch_led_scan_column_index];

  // Apply LED values for this column.
  if (switchLedMatrix[switch_led_scan_column_index].ledState(shiftState))     // Row 1
    val |= 0x01;
  if (switchLedMatrix[switch_led_scan_column_index + 3].ledState(shiftState)) // Row 2
    val |= 0x02;
  if (switchLedMatrix[switch_led_scan_column_index + 6].ledState(shiftState)) // Row 3
    val |= 0x04;
  if (switchLedMatrix[switch_led_scan_column_index + 9].ledState(shiftState)) // Row 4
    val |= 0x08;

  // Output the value.
  PORTB = val;
}

static unsigned kbd_scan_column_index = 0;

void kbd_select_next_column( void )
{
  uint8_t pattern[8] = { 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
//  uint8_t pattern[8] = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };
  PORTL = pattern[kbd_scan_column_index];
}

void kbd_scan_keys_selected_column( void )
{
  // Set the scan time.
  uint32_t scanTime = micros();
  uint8_t valA = PINA;
  uint8_t valC = PINC;

  // Scan the rows.
  keyboard[kbd_scan_column_index].scan(      // Row A
    scanTime, kbd_scan_column_index,
    (valA & 0x01) == 0,
    (valA & 0x02) == 0);

  keyboard[kbd_scan_column_index + 8].scan(  // Row B
    scanTime, kbd_scan_column_index + 8,
    (valA & 0x04) == 0,
    (valA & 0x08) == 0);

  keyboard[kbd_scan_column_index + 16].scan( // Row C
    scanTime, kbd_scan_column_index + 16,
    (valA & 0x10) == 0,
    (valA & 0x20) == 0);

  keyboard[kbd_scan_column_index + 24].scan( // Row D
    scanTime, kbd_scan_column_index + 24,
    (valA & 0x40) == 0,
    (valA & 0x80) == 0);

  keyboard[kbd_scan_column_index + 32].scan( // Row E
    scanTime, kbd_scan_column_index + 32,
    (valC & 0x80) == 0,
    (valC & 0x40) == 0);
}

// Callbacks / hook functions.
void setLed(bool val)
{
  ledPreviousMicros = micros();
  ledState = val;
  WRITE_BIT(ARDUINO_LED, ledState);
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
//  uint8_t channel = CMidiKeySwitch::getMidiCh();
  uint8_t channel = 0;
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

// TODO - FIXME - Rotary encoder appears to advance 2 counts per click while turning
//        although it is possible to turn slow enough to observe the count between
//        adjacent click points.
int rearEncoderPosDebug = 0;
volatile int rearEncoderPosCount = 0;
volatile int rearEncoderClkLast;
ISR(PCINT2_vect)
{
  bool pinA_clk = READ_BIT(REAR_ENCODER_CLK);
  if (pinA_clk != rearEncoderClkLast) {
    // Pin change has occured.
    bool pinB_dt = READ_BIT(REAR_ENCODER_DT);
    if (pinA_clk == pinB_dt) {
      // PinA/Clk changed first - clockwise
      rearEncoderPosCount++;
    } else {
      // PinB/DT changed first - clockwise
      rearEncoderPosCount--;
    }
    rearEncoderClkLast = pinA_clk;
  }
}

// The setup() function runs once at startup.
void setup()
{
  // Digital inputs / outputs / pullup

  // Initialize LED for flashing.
  pinMode(ledPin, OUTPUT);

  // Rear Rotary Encoder
  pinMode(rearEncoderSwPin, INPUT);
  digitalWrite(rearEncoderSwPin, HIGH);
  pinMode(rearEncoderClkPin, INPUT);
  digitalWrite(rearEncoderClkPin, HIGH);
  pinMode(rearEncoderDtPin, INPUT);
  digitalWrite(rearEncoderDtPin, HIGH);

  // Mode Switch and MIDI-In Cable Detect
  pinMode(modeSwitchPin, INPUT);
  digitalWrite(modeSwitchPin, HIGH);
  pinMode(midiCableDetectPin, INPUT);
  digitalWrite(midiCableDetectPin, HIGH);

  // DIP Switches
  for (int i = 0; i < (sizeof(dipSwPin)/sizeof(dipSwPin[0])); i++) {
    pinMode(dipSwPin[i], INPUT);
    digitalWrite(dipSwPin[i], HIGH);
  }

  // LED/Switch matrix col select outputs.
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);

  // LED row drive outputs.
  pinMode(53, OUTPUT);
  pinMode(52, OUTPUT);
  pinMode(51, OUTPUT);
  pinMode(50, OUTPUT);

  // Switch row read inputs.
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);

  // Don't assign the internal pullup resistors.
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);

  // Keyboard scan column select lines.
  pinMode(42, OUTPUT);
  pinMode(43, OUTPUT);
  pinMode(44, OUTPUT);
  pinMode(45, OUTPUT);
  pinMode(46, OUTPUT);
  pinMode(47, OUTPUT);
  pinMode(48, OUTPUT);
  pinMode(49, OUTPUT);

  // Keyboard scan row read lines.
  pinMode(22, INPUT);
  pinMode(23, INPUT);
  pinMode(24, INPUT);
  pinMode(25, INPUT);
  pinMode(26, INPUT);
  pinMode(27, INPUT);
  pinMode(28, INPUT);
  pinMode(29, INPUT);
  pinMode(30, INPUT);
  pinMode(31, INPUT);

  // Assign the internal pullup resistors.
  digitalWrite(22, HIGH);
  digitalWrite(23, HIGH);
  digitalWrite(24, HIGH);
  digitalWrite(25, HIGH);
  digitalWrite(26, HIGH);
  digitalWrite(27, HIGH);
  digitalWrite(28, HIGH);
  digitalWrite(29, HIGH);
  digitalWrite(30, HIGH);
  digitalWrite(31, HIGH);

  // Set up PCINT23 for use as PinA/Clk on rear rotary encoder.
  // Enable A15 / D69 / PK7 as the only pin change that generates the interrupt.
  PCMSK2 = 0x80;
  // Enable PCINT2 ISR.
  PCICR |= 0x04;
  // Get initial state of encoder clk (pin A).
  rearEncoderClkLast = READ_BIT(REAR_ENCODER_CLK);

#if FORCE_DEBUG
  Serial.begin(230400);
  Serial.println(F("Chi - forced debug"));
#endif

  // Read in so we can check the DIP switches at startup.
#if FORCE_DEBUG
  debug_mode = true;
#else
  // DIP SW4 selects debug mode for Main MCU.
  debug_mode = (digitalRead(dipSwPin[DIP_SW_MAIN_MCU_DEBUG]) == 0);
#endif
  // DIP SW3 selects debug mode for Aux MCU.
  debug_mode_aux = (digitalRead(dipSwPin[DIP_SW_AUX_MCU_DEBUG]) == 0);

  if (!debug_mode) {
    // Use USB serial for MIDI
    midiUSB.begin(&setLed, &handleCtrlCh, &handleProgCh);
  } else {
    // Initialize serial UART for debug output.
#if ! FORCE_DEBUG
    Serial.begin(230400);
#endif
    Serial.println(F("Chi - stage 4 version 1"));
    Serial.print(F("DIP SW1..4 [ "));
    for (int i = 0; i < (sizeof(dipSwPin)/sizeof(dipSwPin[0])); i++) {
      if (digitalRead(dipSwPin[i]) == 0) {
        Serial.print(F("on  "));
      } else {
        Serial.print(F("off "));
      }
    }
    Serial.println(F("]"));
    if (debug_mode) {
      Serial.println(F("Main MCU - debug mode"));
    }
    if (debug_mode_aux) {
      Serial.println(F("Aux MCU - debug mode"));
    }
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

  // Setup serial ports used for MIDI (Serial port 0 [USB] already setup earlier).
  // Serial1 - MIDI-In / MIDI-Out connectors.
  midiJacks.begin(&setLed, &handleCtrlCh, &handleProgCh);
//Serial1.begin(31250);
  // Serial2 - MIDI-In from Aux MCU [B2B] / MIDI-Thru/Out2 connector.
  if (debug_mode_aux) {
    // When Aux MCU debug mode set, we lose the MIDI-Thru/Out2
    Serial2.begin(230400);
  } else {
    // FIXME - TODO - need to implement any needed hooks for merging, etc.
    // Start the MIDI port.
    midiB2bThru.begin(&setLed);
  }
  // Serial3 - spare / unused.
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


//unsigned long nextSecondsMicros = 0;

void loop()
{
  unsigned long currentMicros = micros();

#if 0
if (currentMicros >= nextSecondsMicros) {
  // Passed a 1sec tick mark
  Serial.print(F("Seconds: "));
  Serial.println(nextSecondsMicros / 1000000);
  nextSecondsMicros += 1000000;
}
#endif

  static unsigned long lastLEDSwitchScanMicros = 0;
  static unsigned led = 0;
  if (currentMicros - ledPreviousMicros >= ledBlinkInterval) {
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
      midiUSB.activeSense();
    }
    midiJacks.activeSense();
    //midiB2bThru.activeSense();

    // Flush output state to LED
    setLed(ledState);

    if (debug_mode) {
      for (unsigned i = 0; i < 12; i++) {
        switchLedMatrix[switchLedSeq[i]].setLedState(i == led, E_SS_UNSHIFTED);
      }
      led++;
      led %= 12;
//      Serial.print(F("Profile Cycle Time: "));
      // * 8 because it takes 8 cycles to scan the keyboard.
//      Serial.println(cycleTime / CYCLE_WIN_SZ * 8);

      noInterrupts();
      int pos = rearEncoderPosCount;
      interrupts();
      if (pos != rearEncoderPosDebug) {
        Serial.print(F("Rear Encoder: "));
        Serial.println(pos);
        rearEncoderPosDebug = pos;
      }
    }
  }

//  uint32_t startTime = micros();

  // Scan the keyboard rows.
  kbd_scan_keys_selected_column();

  // Advance and select next column.
  kbd_scan_column_index = (kbd_scan_column_index + 1) % 8;
  kbd_select_next_column();

  if ((currentMicros - lastLEDSwitchScanMicros) >= 1000) {
    // 1. Read the switch input values for this column.
    // 2. Select next LED / Switch column
    // 3. Set up the LED output values for next column.
    switch_led_scan_next_column( currentMicros );

    scan_misc_switches( currentMicros );

    // Syncup for next time.
    lastLEDSwitchScanMicros = currentMicros;
  }

  // Check for and handle receive MIDI messages from USB.
  midiUSB.receiveScan();

  // Check for and handle receive MIDI messages from MIDI-In connector.
  midiJacks.receiveScan();

  // Check for and handle receive MIDI messages.
  if (debug_mode_aux) {
    // Aux MCU is in debug mode so receive / repeat any debug output.
    // Note: Max of 100 char, or when newline, or no Rx data available.
    unsigned int polls = 100;
    while ((Serial2.available() > 0) && --polls) {
      uint8_t c = Serial2.read();
      Serial.write(c);
      if (c == '\n')
        break;
    }
  } else {
    midiB2bThru.receiveScan();
  }

//  uint32_t endTime = micros();

//  update_cycle_time(endTime - startTime);
}

