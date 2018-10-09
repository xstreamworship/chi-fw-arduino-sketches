/////////////////////////////////////////////////////////////////////
// CHI enhanced MIDI keyboard workstation - Aux MCU sketch file.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////

// The Aux MCU is an Arduino Pro Mini, handling primarily
//     - I/O expansion via I2C peripheral devices (CAT9555 and AD7997).
//     - Scanning the Hammond organ drawbars
//     - Handling the analog inputs
//     - Other misc I/O
//
#if defined(__AVR_ATmega328P__)
#else
#error Unsupported platform!
#endif

#include "twi_if.h"
#include "Cat9555.h"
#include "Ad7997.h"
#include "Drawbar.h"
#include "Switch.h"
#include "Filter.h"
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
long ledBlinkInterval = 300000; // us
const long wireClockFrequency = 800000; // Hz - I2C overclocked to 800kHz

// Arduino I/O pin addresses
#define ARDUINO_LED_PORT PORTB
#define ARDUINO_LED_PIN PINB
#define ARDUINO_LED_BIT 5
const int ledPin =  LED_BUILTIN; // pin address

#define JOYSTICK_BUTTON_PORT PORTD
#define JOYSTICK_BUTTON_PIN PIND
#define JOYSTICK_BUTTON_BIT 7
const int joystickButtonPin =  7; // pin address
#define ROTARY_SLOW_PORT PORTB
#define ROTARY_SLOW_PIN PINB
#define ROTARY_SLOW_BIT 0
#define ROTARY_FAST_PORT PORTB
#define ROTARY_FAST_PIN PINB
#define ROTARY_FAST_BIT 1
const int rotarySwPin[2] =  {8, 9}; // pin address

// Bit masks to setup the digital I/O pins of CAT9555 ports.
const uint16_t REG_Q_PORT_CONFIG = 0xff;
const uint16_t REG_R_PORT_CONFIG = 0xff;
const uint16_t REG_S_PORT_CONFIG = 0xf3;
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
CFilter analogFilters[] =
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
CSwitch footSwitches[] =
{
  CSwitch(footSwitchesStr0),
  CSwitch(footSwitchesStr1),
};

const PROGMEM char joystickButtonStr[] = "JSButton";
CSwitch joystickButton(joystickButtonStr);
const PROGMEM char rotarySwStr0[] = "RotSlow";
const PROGMEM char rotarySwStr1[] = "RotFast";
CSwitch rotarySw[] =
{
  CSwitch(rotarySwStr0),
  CSwitch(rotarySwStr1),
};

const PROGMEM char Drawbar1Str[] = "1-Bourdin";
const PROGMEM char Drawbar2Str[] = "2-Quint";
const PROGMEM char Drawbar3Str[] = "3-Principal";
const PROGMEM char Drawbar4Str[] = "4-Octave";
const PROGMEM char Drawbar5Str[] = "5-Nazard";
const PROGMEM char Drawbar6Str[] = "6-Block Flote";
const PROGMEM char Drawbar7Str[] = "7-Tierce";
const PROGMEM char Drawbar8Str[] = "8-Larigot";
const PROGMEM char Drawbar9Str[] = "9-Sifflote";
CDrawbar drawbar[] =
{
  CDrawbar(Drawbar1Str),
  CDrawbar(Drawbar2Str),
  CDrawbar(Drawbar3Str),
  CDrawbar(Drawbar4Str),
  CDrawbar(Drawbar5Str),
  CDrawbar(Drawbar6Str),
  CDrawbar(Drawbar7Str),
  CDrawbar(Drawbar8Str),
  CDrawbar(Drawbar9Str),
};

// This is main MIDI-Out and MIDI-In
// Main MCU - the MIDI-Out and MIDI-In jacks on back of the keyboard (may change this to USB MIDI).
// Aux MCU - the serial port to send the MIDI events to the main MCU (no MIDI-In).
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
void scan_misc_switches( uint32_t timeS )
{
  // From CAT9555 ports.
  footSwitches[0].scan( timeS, // CC   Use Case
    (RegS.read() & 0x80) == 0,  64,  E_UC_SIMPLE_CC);
  footSwitches[1].scan( timeS, // CC   Use Case
    (RegS.read() & 0x40) == 0,  66,  E_UC_SIMPLE_CC);

  // From MCU direct ports.
  joystickButton.scan( timeS,
    READ_BIT(JOYSTICK_BUTTON) == 0);
  rotarySw[0].scan( timeS, //      Lf/Rt  Use Case
    READ_BIT(ROTARY_SLOW) == 0, 0,   E_UC_ROTARY_CC);
  rotarySw[1].scan( timeS, //      Lf/Rt  Use Case
    READ_BIT(ROTARY_FAST) == 0, 1,   E_UC_ROTARY_CC);
}

static unsigned analog_ch_index = 0;

void scan_analogs(uint8_t index, uint32_t rTime)
{
  analogFilters[index].scan(AnalogA.read(index), rTime);
}

static unsigned drawbar_scan_busbar_index = 0;

void drawbar_select_next_busbar( void )
{
  uint8_t pattern_S[] = { 0x08, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x04 };
  const uint8_t mask_S = 0xf3;
  uint8_t val_S = (RegS.read() & mask_S) | pattern_S[drawbar_scan_busbar_index];
  RegS.write(val_S);

  uint8_t pattern_T[] = { 0xfe, 0xfc, 0xfa, 0xf6, 0xee, 0xde, 0xbe, 0x7e, 0xfe };
  const uint8_t mask_T = 0x01;
  uint8_t val_T = (RegT.read() & mask_T) | pattern_T[drawbar_scan_busbar_index];
  RegT.write(val_T);
}

void drawbar_scan_bars_selected_busbar( void )
{
  // Set the scan time.
  uint32_t scanTime = micros();

  uint8_t val_Q = RegQ.read();
  uint8_t val_R = RegR.read();
  uint8_t val_S = RegS.read();

  drawbar[0].scan(scanTime, drawbar_scan_busbar_index,
    (val_S & 0x20) == 0, (val_S & 0x10) == 0, 21);
  drawbar[1].scan(scanTime, drawbar_scan_busbar_index,
    (val_R & 0x02) == 0, (val_R & 0x01) == 0, 22);
  drawbar[2].scan(scanTime, drawbar_scan_busbar_index,
    (val_R & 0x08) == 0, (val_R & 0x04) == 0, 23);
  drawbar[3].scan(scanTime, drawbar_scan_busbar_index,
    (val_R & 0x20) == 0, (val_R & 0x10) == 0, 24);
  drawbar[4].scan(scanTime, drawbar_scan_busbar_index,
    (val_R & 0x80) == 0, (val_R & 0x40) == 0, 25);
  drawbar[5].scan(scanTime, drawbar_scan_busbar_index,
    (val_Q & 0x40) == 0, (val_Q & 0x80) == 0, 26);
  drawbar[6].scan(scanTime, drawbar_scan_busbar_index,
    (val_Q & 0x10) == 0, (val_Q & 0x20) == 0, 27);
  drawbar[7].scan(scanTime, drawbar_scan_busbar_index,
    (val_Q & 0x04) == 0, (val_Q & 0x08) == 0, 28);
  drawbar[8].scan(scanTime, drawbar_scan_busbar_index,
    (val_Q & 0x01) == 0, (val_Q & 0x02) == 0, 29);
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
//  uint8_t channel = CMidiKeySwitch::getMidiCh();
  uint8_t channel = 0;
  uint8_t ccVal = state?127:0;
  if (ccNum > 119)
    // Note: Default value passed into an incompletely defined CSwitch scan() is 255.
    return;
  switch (uCase)
  {
    case E_UC_SIMPLE_CC: // Simple switch mapped to CC use case
      midiJacks.ctrlCh(ccNum, ccVal, channel);
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
//  uint8_t channel = CMidiKeySwitch::getMidiCh();
  uint8_t channel = 0;
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

// Handle drawbar change
void drawbarChanged(uint8_t val, uint8_t ccNum, uint8_t uCase)
{
//  uint8_t channel = CMidiKeySwitch::getMidiCh();
  uint8_t channel = 0;
  uint8_t ccVal = (val >= 32)?127:(val * 4);
  //
  midiJacks.ctrlCh(ccNum, ccVal, channel);
}

// The setup() function runs once at startup.
void setup()
{
  // Initialize LED for flashing.
  pinMode(ledPin, OUTPUT);

  // Digital inputs.
  pinMode(joystickButtonPin, INPUT);
  pinMode(rotarySwPin[0], INPUT);
  pinMode(rotarySwPin[1], INPUT);

  // Assign the internal pullup resistors.
  digitalWrite(joystickButtonPin, HIGH);
  digitalWrite(rotarySwPin[0], HIGH);
  digitalWrite(rotarySwPin[1], HIGH);

#if FORCE_DEBUG
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

  // Read in so we can check the DIP switches at startup.
#if FORCE_DEBUG
  debug_mode = true;
#else
  // DIP SW3 selects debug mode for Aux MCU.
  RegT.syncIn();
  debug_mode = ((RegT.read() & 0x01) == 0);
#endif

  if (!debug_mode) {
    // Use serial for MIDI
    midiJacks.begin(&setLed);
  } else {
    // Initialize serial UART for debug output.
#if ! FORCE_DEBUG
    Serial.begin(230400);
#endif
    Serial.print(F("Chi - "));
    Serial.print(F("aux MCU"));
    Serial.println(F(" - stage 4 version 1"));
  }
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
      midiJacks.activeSense();
    }

    // Flush output state to LED
    setLed(ledState);

    if (debug_mode) {
//      Serial.print(F("Profile Cycle Time: "));
      // * 8 because it takes 8 cycles to scan the keyboard.
//      Serial.println(cycleTime / CYCLE_WIN_SZ * 8);
    }
  }

  enum e_ScanStates
  {
    E_SCAN_START,
    E_SCAN_WRITING_DRAWBAR_BUS_SELECT_S,
    E_SCAN_WRITING_DRAWBAR_BUS_SELECT_T,
    E_SCAN_READING_DRAWBAR_INPUT_Q,
    E_SCAN_READING_DRAWBAR_INPUT_R,
    E_SCAN_READING_DRAWBAR_INPUT_S,
    E_SCAN_MISC_SWITCHES,
    E_SCAN_READING_ANALOGS,
  };
  static enum e_ScanStates state = E_SCAN_START;

  switch (state)
  {
    case E_SCAN_START:
      // Select the drawbar bus.
      drawbar_select_next_busbar();

      RegS.startOut();
      state = E_SCAN_WRITING_DRAWBAR_BUS_SELECT_S;
      break;

    case E_SCAN_WRITING_DRAWBAR_BUS_SELECT_S:
      // Wait until done.
      if (RegS.isBusy()) break;

      RegT.startOut();
      state = E_SCAN_WRITING_DRAWBAR_BUS_SELECT_T;
      break;

    case E_SCAN_WRITING_DRAWBAR_BUS_SELECT_T:
      // Wait until done.
      if (RegT.isBusy()) break;

      RegQ.startIn();
      state = E_SCAN_READING_DRAWBAR_INPUT_Q;
      break;

    case E_SCAN_READING_DRAWBAR_INPUT_Q:
      // Wait until done.
      if (RegQ.isBusy()) break;

      RegR.startIn();
      state = E_SCAN_READING_DRAWBAR_INPUT_R;
      break;

    case E_SCAN_READING_DRAWBAR_INPUT_R:
      // Wait until done.
      if (RegR.isBusy()) break;

      RegS.startIn();
      state = E_SCAN_READING_DRAWBAR_INPUT_S;
      break;

    case E_SCAN_READING_DRAWBAR_INPUT_S:
      // Wait until done.
      if (RegS.isBusy()) break;

      // Process the drawbar inputs read.
      drawbar_scan_bars_selected_busbar();

      state = E_SCAN_MISC_SWITCHES;
      break;

    case E_SCAN_MISC_SWITCHES:
      scan_misc_switches( currentMicros );

      if (rotaryBrakeStart) {
        // Rotary brake mode timer active.
        if ((currentMicros - rotaryBrakeStart) > 250000) {
          // 250ms guard time before braking the rotary since the switch
          // always passes through the centre position.
          switchChanged(false, 2, E_UC_ROTARY_CC);
        }
      }

      analog_ch_index = 0;
      AnalogA.start(analog_ch_index);
      state = E_SCAN_READING_ANALOGS;
      break;

    case  E_SCAN_READING_ANALOGS:
      // Wait until done.
      if (AnalogA.isBusy()) break;

      scan_analogs(analog_ch_index, currentMicros);

      if (analog_ch_index < 7) {
        // start for the next analog.
        AnalogA.sync(++analog_ch_index);
        break;
      }

      // Completed.
      if (analogFilters[5].atOrigin() && analogFilters[6].atOrigin() && analogFilters[7].atOrigin()) {
        // We only allow the joystick switch to shift / unshift while joystick is positioned at origin.
        joystickShifted = joystickButton.switchState();
      }

      // Syncup for next time.
      if (drawbar_scan_busbar_index < 8) {
        drawbar_scan_busbar_index++;
      } else {
        drawbar_scan_busbar_index = 0;
      }
      state = E_SCAN_START;
      break;
  }

//  uint32_t endTime = micros();

//  update_cycle_time(endTime - startTime);
}

