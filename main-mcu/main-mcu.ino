/////////////////////////////////////////////////////////////////////
// CHI enhanced MIDI keyboard workstation - main sketch file.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////

// This sketch is built into two (2) firmware binaries from one (1) set of source files (the sketch).
// 1.  The 'main' MCU is on an Arduino Mega 2560, handling primarily
//     - Keyboard scanning
//     - MIDI (serial and USB) interfaces
//     - MIDI redirection / merging
//     - LED / Switch matrix
//     - User interface logic
// 2.  The 'aux' MCU is an Arduino Pro Mini, handling primarily
//     - I/O expansion via I2C peripheral devices (CAT9555 and AD7997).
//     - Scanning the Hammond organ drawbars
//     - Handling the analog inputs
//     - Other misc I/O
//
// The distinction is determined based on the type of MCU which is determined based on the board type.
// So if you build for Arduino Pro Mini (or any similar product based on ATmega328P) then it will
// build for the aux MCU.  But if you build for Arduino Mega 2560 (or any similar product based on
// the ATmega2560) then it will build for the main MCU.

#if defined(__AVR_ATmega2560__)
#define CHI_MAIN_MCU_FW 1
#define CHI_AUX_MCU_FW 0
#elif defined(__AVR_ATmega328P__)
#define CHI_MAIN_MCU_FW 0
#define CHI_AUX_MCU_FW 1
#else
#error Unsupported platform!
#endif

// Now for mapping of functions to the firmware type.
#define CHI_TWI_IF CHI_AUX_MCU_FW
#define CHI_KEYBOARD CHI_MAIN_MCU_FW
#define CHI_DRAWBARS CHI_AUX_MCU_FW
#define CHI_LEDSWMATRIX CHI_MAIN_MCU_FW
#define CHI_ANALOGCC CHI_AUX_MCU_FW
#define CHI_MIDI_IF CHI_MAIN_MCU_FW
#define CHI_JOYSTROTARY_SW CHI_AUX_MCU_FW
#define CHI_FOOT_SW CHI_AUX_MCU_FW
#define CHI_MISC_SW CHI_MAIN_MCU_FW

#if CHI_TWI_IF
#include "twi_if.h"
#if CHI_DRAWBARS
#include "Cat9555.h"
#endif
#if CHI_ANALOGCC
#include "Ad7997.h"
#endif
#endif

#if CHI_KEYBOARD
#include "MidiKeySwitch.h"
#endif

#if CHI_DRAWBARS
#include "Drawbar.h"
#endif

#if CHI_LEDSWMATRIX || CHI_JOYSTROTARY_SW || CHI_FOOT_SW
#include "Switch.h"
#endif

#if CHI_ANALOGCC
#include "Filter.h"
#endif

#include "MidiPort.h"

#define FORCE_DEBUG CHI_MAIN_MCU_FW

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
#if CHI_TWI_IF
const long wireClockFrequency = 800000; // Hz - I2C overclocked to 800kHz
#endif

// Arduino I/O pin addresses
#define ARDUINO_LED_PORT PORTB
#define ARDUINO_LED_PIN PINB
#if CHI_MAIN_MCU_FW
#define ARDUINO_LED_BIT 7
#elif CHI_AUX_MCU_FW
#define ARDUINO_LED_BIT 5
#endif
const int ledPin =  LED_BUILTIN; // pin address

#if CHI_JOYSTROTARY_SW
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
#endif

#if CHI_MISC_SW
const int midiCableDetectPin =  6; // pin address
#endif

#if CHI_TWI_IF && CHI_DRAWBARS
// Bit masks to setup the digital I/O pins of CAT9555 ports.
const uint16_t REG_Q_PORT_CONFIG = 0xff;
const uint16_t REG_R_PORT_CONFIG = 0xff;
const uint16_t REG_S_PORT_CONFIG = 0xf3;
const uint16_t REG_T_PORT_CONFIG = 0x01;
#endif

// Variables
bool ledState = LOW; // used to set the LED
uint32_t ledPreviousMicros = 0; // will store last time LED was updated
bool debug_mode;

#if CHI_TWI_IF && CHI_ANALOGCC
// Analogue Inputs
CAd7997 AnalogA(34); // I2C address
#endif

#if CHI_TWI_IF && CHI_DRAWBARS
// Digital I/O
// CAT9555 registers are designated in hardware port order.
CCat9555 RegQ(32, 0, REG_Q_PORT_CONFIG); // I2C address and port config
CCat9555 RegR(32, 1, REG_R_PORT_CONFIG); // I2C address and port config
CCat9555 RegS(33, 0, REG_S_PORT_CONFIG); // I2C address and port config
CCat9555 RegT(33, 1, REG_T_PORT_CONFIG); // I2C address and port config
#endif

#if CHI_ANALOGCC
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
#endif

#if CHI_FOOT_SW
// Logical switch scanner.
const PROGMEM char footSwitchesStr0[] = "Damper";
const PROGMEM char footSwitchesStr1[] = "Soft";
CSwitch footSwitches[] =
{
  CSwitch(footSwitchesStr0),
  CSwitch(footSwitchesStr1),
};
#endif

#if CHI_MISC_SW
//const PROGMEM char modeSwitchStr[] = "Mode";
//const PROGMEM CSwitch modeSwitch(modeSwitchStr);
const PROGMEM char midiCableDetectStr[] = "MIDICable";
CSwitch midiCableDetect(midiCableDetectStr);
#endif

#if CHI_LEDSWMATRIX
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
#endif

#if CHI_JOYSTROTARY_SW
const PROGMEM char joystickButtonStr[] = "JSButton";
CSwitch joystickButton(joystickButtonStr);
const PROGMEM char rotarySwStr0[] = "RotSlow";
const PROGMEM char rotarySwStr1[] = "RotFast";
CSwitch rotarySw[] =
{
  CSwitch(rotarySwStr0),
  CSwitch(rotarySwStr1),
};
#endif

#if CHI_DRAWBARS
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
#endif

#if CHI_KEYBOARD
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
#endif

// This is main MIDI-Out and MIDI-In
// Main MCU - the MIDI-Out and MIDI-In jacks on back of the keyboard (may change this to USB MIDI).
// Aux MCU - the serial port to send the MIDI events to the main MCU (no MIDI-In).
CMidiPort<HardwareSerial> midiJacks((HardwareSerial&)Serial);
#if CHI_MIDI_IF
// The MIDI-Thru/Out2 jack on back of the keyboard and MIDI input from Aux MCU's MIDI output.
CMidiPort<HardwareSerial> midiJacks2((HardwareSerial&)Serial2);
#endif

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
#if CHI_FOOT_SW
  // From CAT9555 ports.
  footSwitches[0].scan( timeS, // CC   Use Case
    (RegS.read() & 0x80) == 0,  64,  E_UC_SIMPLE_CC);
  footSwitches[1].scan( timeS, // CC   Use Case
    (RegS.read() & 0x40) == 0,  66,  E_UC_SIMPLE_CC);
#endif

#if CHI_MISC_SW
//FIXME - map/remap the I/O
  //modeSwitch.scan( timeS,
  //  (RegT.read() & 0x01) != 0);
//  midiCableDetect.scan( timeS,
//    READ_BIT(H, 3) != 0);
#endif

#if CHI_JOYSTROTARY_SW
  // From MCU direct ports.
  joystickButton.scan( timeS,
    READ_BIT(JOYSTICK_BUTTON) == 0);
  rotarySw[0].scan( timeS, //      Lf/Rt  Use Case
    READ_BIT(ROTARY_SLOW) == 0, 0,   E_UC_ROTARY_CC);
  rotarySw[1].scan( timeS, //      Lf/Rt  Use Case
    READ_BIT(ROTARY_FAST) == 0, 1,   E_UC_ROTARY_CC);
#endif
}

#if CHI_ANALOGCC
static unsigned analog_ch_index = 0;

void scan_analogs(uint8_t index, uint32_t rTime)
{
  analogFilters[index].scan(AnalogA.read(index), rTime);
}
#endif

#if CHI_LEDSWMATRIX
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

void switch_led_sync_led_states_selected_column( void )
{
  const uint8_t mask = 0xe1;
//FIXME - remap the I/O
#if 0
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
#endif
}

void switch_led_select_next_column( void )
{
  uint8_t pattern[3] = { 0x60, 0xa0, 0xc0 };
  const uint8_t mask = 0x1f;
  
  // Select the column to scan.
  switch_led_scan_column_index = (switch_led_scan_column_index + 1) % 3;
//FIXME - remap the I/O
//  uint16_t val = (RegT.read() & mask) | pattern[switch_led_scan_column_index];
//  RegT.write(val);
}

void switch_led_scan_switches_selected_column( uint32_t timeS )
{
  // LS 7bits are the CC num, MSB is the use case selector between shift and shifted.
  uint8_t ccMap[] = { 128, 106, 102, 129, 107, 103, 93, 108, 104, 92, 109, 105 };
  // Scan the rows.
//FIXME - remap the I/O
#if 0
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
#endif
}
#endif

#if CHI_KEYBOARD
static unsigned kbd_scan_column_index = 0;

void kbd_select_next_column( void )
{
//FIXME - remap the I/O
  // Select the column to scan.
//  WRITE_BIT( D, 7, kbd_scan_column_index != 7);
//  WRITE_BIT( G, 1, kbd_scan_column_index != 6);
//  WRITE_BIT( L, 7, kbd_scan_column_index != 5);
//  WRITE_BIT( L, 5, kbd_scan_column_index != 4);
//  WRITE_BIT( L, 3, kbd_scan_column_index != 3);
//  WRITE_BIT( L, 1, kbd_scan_column_index != 2);
//  WRITE_BIT( B, 3, kbd_scan_column_index != 1);
//  WRITE_BIT( B, 1, kbd_scan_column_index != 0);
}

void kbd_scan_keys_selected_column( void )
{
  // Set the scan time.
  uint32_t scanTime = micros();

//FIXME - remap the I/O
#if 0
  // Scan the rows.
  keyboard[kbd_scan_column_index].scan(      // Row A
    scanTime, kbd_scan_column_index,
    READ_BIT(C ,2) == 0,
    READ_BIT(C ,0) == 0);

  keyboard[kbd_scan_column_index + 8].scan(  // Row B
    scanTime, kbd_scan_column_index + 8,
    READ_BIT(G ,2) == 0,
    READ_BIT(G ,0) == 0);

  keyboard[kbd_scan_column_index + 16].scan( // Row C
    scanTime, kbd_scan_column_index + 16,
    READ_BIT(L ,6) == 0,
    READ_BIT(L ,4) == 0);

  keyboard[kbd_scan_column_index + 24].scan( // Row D
    scanTime, kbd_scan_column_index + 24,
    READ_BIT(L ,2) == 0,
    READ_BIT(L ,0) == 0);

  keyboard[kbd_scan_column_index + 32].scan( // Row E
    scanTime, kbd_scan_column_index + 32,
    READ_BIT(B ,2) == 0,
    READ_BIT(B ,0) == 0);
#endif
}
#endif

#if CHI_DRAWBARS
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
#endif

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

#if CHI_LEDSWMATRIX
//                        Trans PC 1  2  3   4  5  6  7   8  Ch Trem
uint8_t switchLedSeq[12] = { 0, 3, 2, 5, 8, 11, 1, 4, 7, 10, 6, 9 };
uint8_t currentPC = 0;

void syncLedsToProgChange(void)
{
    for (unsigned i = 2; i < 10; i++) {
      switchLedMatrix[switchLedSeq[i]].setLedState(i == (currentPC + 2), E_SS_PROGCH);
    }
}
#endif

#if CHI_JOYSTROTARY_SW
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
#endif

#if CHI_LEDSWMATRIX || CH_JOYSTROTARY_SW || CHI_FOOT_SW || CHI_MISC_SW
void switchChanged(bool state, uint8_t ccNum, uint8_t uCase)
{
//  uint8_t channel = CMidiKeySwitch::getMidiCh();
  uint8_t channel = 0;
  uint8_t ccVal = state?127:0;
#if CHI_LEDSWMATRIX
  uint8_t newPC = currentPC;
#endif
  if (ccNum > 119)
    // Note: Default value passed into an incompletely defined CSwitch scan() is 255.
    return;
  switch (uCase)
  {
    case E_UC_SIMPLE_CC: // Simple switch mapped to CC use case
      midiJacks.ctrlCh(ccNum, ccVal, channel);
      break;
#if CHI_LEDSWMATRIX
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
#endif
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
#endif

#if CHI_ANALOGCC && CHI_JOYSTROTARY_SW
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
#endif

#if CHI_DRAWBARS
// Handle drawbar change
void drawbarChanged(uint8_t val, uint8_t ccNum, uint8_t uCase)
{
//  uint8_t channel = CMidiKeySwitch::getMidiCh();
  uint8_t channel = 0;
  uint8_t ccVal = (val >= 32)?127:(val * 4);
  //
  midiJacks.ctrlCh(ccNum, ccVal, channel);
}
#endif

#if CHI_LEDSWMATRIX
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
#endif

// The setup() function runs once at startup.
void setup()
{
  // Initialize LED for flashing.
  pinMode(ledPin, OUTPUT);

#if CHI_JOYSTROTARY_SW
  // Digital inputs.
  pinMode(joystickButtonPin, INPUT);
  pinMode(rotarySwPin[0], INPUT);
  pinMode(rotarySwPin[1], INPUT);

  // Assign the internal pullup resistors.
  digitalWrite(joystickButtonPin, HIGH);
  digitalWrite(rotarySwPin[0], HIGH);
  digitalWrite(rotarySwPin[1], HIGH);
#endif

#if CHI_MISC_SW
  // Digital input / pullup
  pinMode(midiCableDetectPin, INPUT);
  digitalWrite(midiCableDetectPin, HIGH);
#endif

#if CHI_KEYBOARD
  // Keyboard scan column select lines.
  pinMode(38, OUTPUT);
  pinMode(40, OUTPUT);
  pinMode(42, OUTPUT);
  pinMode(44, OUTPUT);
  pinMode(46, OUTPUT);
  pinMode(48, OUTPUT);
  pinMode(50, OUTPUT);
  pinMode(52, OUTPUT);

  // Keyboard scan row read lines.
  pinMode(35, INPUT);
  pinMode(37, INPUT);
  pinMode(39, INPUT);
  pinMode(41, INPUT);
  pinMode(43, INPUT);
  pinMode(45, INPUT);
  pinMode(47, INPUT);
  pinMode(49, INPUT);
  pinMode(51, INPUT);
  pinMode(53, INPUT);

  // Assign the internal pullup resistors.
  digitalWrite(35, HIGH);
  digitalWrite(37, HIGH);
  digitalWrite(39, HIGH);
  digitalWrite(41, HIGH);
  digitalWrite(43, HIGH);
  digitalWrite(45, HIGH);
  digitalWrite(47, HIGH);
  digitalWrite(49, HIGH);
  digitalWrite(51, HIGH);
  digitalWrite(53, HIGH);
#endif

#if FORCE_DEBUG
  Serial.begin(230400);
  Serial.println(F("Chi - debug"));
#endif

#if CHI_TWI_IF
  // Initialize the I2C bus.
  twi_init(16000000UL, wireClockFrequency);

  // Probe and initialize the CAT9555 expansion ports.
  RegQ.begin();
  RegR.begin();
  RegS.begin();
  RegT.begin();

  // Probe and initialize the AD7997 analog ports.
  AnalogA.begin();
#endif

  // Read in so we can check the DIP switches at startup.
#if FORCE_DEBUG
  debug_mode = true;
#else
  // DIP SW3 selects debug mode for Aux MCU.
#if CHI_AUX_MCU_FW && CHI_TWI_IF
  RegT.syncIn();
  debug_mode = ((RegT.read() & 0x01) == 0);
#endif
#endif

  if (!debug_mode) {
    // Use serial for MIDI
#if CHI_MIDI_IF
    midiJacks.begin(&setLed, &handleCtrlCh, &handleProgCh);
#else
    midiJacks.begin(&setLed);
#endif
  } else {
    // Initialize serial UART for debug output.
#if ! FORCE_DEBUG
    Serial.begin(230400);
#endif
    Serial.print(F("Chi - "));
#if CHI_MAIN_MCU_FW
    Serial.print(F("main MCU"));
#elif CHI_AUX_MCU_FW
    Serial.print(F("aux MCU"));
#endif
    Serial.println(F(" - stage 3 version 1"));
  }

#if CHI_LEDSWMATRIX
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
#endif
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
#if CHI_LEDSWMATRIX
      for (unsigned i = 0; i < 12; i++) {
        switchLedMatrix[switchLedSeq[i]].setLedState(i == led, E_SS_UNSHIFTED);
      }
      led++;
      led %= 12;
#endif
//      Serial.print(F("Profile Cycle Time: "));
      // * 8 because it takes 8 cycles to scan the keyboard.
//      Serial.println(cycleTime / CYCLE_WIN_SZ * 8);
    }
  }

#if CHI_KEYBOARD
  // Scan the keyboard
//  uint32_t startTime = micros();
  kbd_select_next_column();
  kbd_scan_keys_selected_column();
#endif

#if CHI_LEDSWMATRIX
  if ((currentMicros - lastLEDSwitchScanMicros) >= 2000) {
    // Select next LED / Switch column
    switch_led_select_next_column();

    // Set up the LED output values for this column.
    switch_led_sync_led_states_selected_column();

    // Read the switch input values for this column.
    switch_led_scan_switches_selected_column( currentMicros );

#if CHI_MISC_SW
    scan_misc_switches( currentMicros );
#endif

    // Syncup for next time.
    lastLEDSwitchScanMicros = currentMicros;
  }
#endif

#if CHI_TWI_IF
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
#if CHI_FOOT_SW || CH_JOYSTROTARY_SW
      scan_misc_switches( currentMicros );

#if CH_JOYSTROTARY_SW
      if (rotaryBrakeStart) {
        // Rotary brake mode timer active.
        if ((currentMicros - rotaryBrakeStart) > 250000) {
          // 250ms guard time before braking the rotary since the switch
          // always passes through the centre position.
          switchChanged(false, 2, E_UC_ROTARY_CC);
        }
      }
#endif
#endif

#if CHI_ANALOGCC
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
#if CHI_JOYSWROTARY_SW
        // We only allow the joystick switch to shift / unshift while joystick is positioned at origin.
        joystickShifted = joystickButton.switchState();
#endif
      }
#endif

#if CHI_DRAWBARS
      // Syncup for next time.
      if (drawbar_scan_busbar_index < 8) {
        drawbar_scan_busbar_index++;
      } else {
        drawbar_scan_busbar_index = 0;
      }
#endif
      state = E_SCAN_START;
      break;
  }
#endif

#if CHI_MIDI_IF
  // Check for and handle receive MIDI messages.
  // Note: Not applicable to Aux MCU (no MIDI input).
  midiJacks.receiveScan();
#endif

//  uint32_t endTime = micros();

#if CHI_KEYBOARD
  // Advance to next column.
  kbd_scan_column_index = (kbd_scan_column_index + 1) % 8;
#endif

//  update_cycle_time(endTime - startTime);
}

