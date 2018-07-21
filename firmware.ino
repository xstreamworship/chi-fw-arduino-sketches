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
#include <MIDI.h>

#include "Cat9555.h"
#include "MidiKeySwitch.h"
#include "Ad7997.h"
#include "Switch.h"
#include "Filter.h"

// Constants
long ledBlinkInterval = 250; // ms
const long wireClockFrequency = 400000; // Hz - I2C fast mode

// Arduino I/O pin addresses
const int ledPin =  LED_BUILTIN; // pin address
const int joystickButtonPin =  7; // pin address
const int rotarySwPin[2] =  {8, 9}; // pin address
const int midiCableDetectPin =  6; // pin address

// Bit masks to setup the digital I/O pins of CAT9555 ports.
const uint16_t REG_RQ_PORT_CONFIG = 0xff00;
const uint16_t REG_TS_PORT_CONFIG = 0x01ff;

// Variables
int ledState = LOW; // used to set the LED
unsigned long ledPreviousMillis = 0; // will store last time LED was updated

// Analogue Inputs
CAd7997 AnalogA(34); // I2C address

// Digital I/O
// CAT9555 registers are designated in hardware port (i.e. little endian) order.
CCat9555 RegRQ(32, REG_RQ_PORT_CONFIG); // I2C address and port config
CCat9555 RegTS(33, REG_TS_PORT_CONFIG); // I2C address and port config

// Analog filters for joystick.
const PROGMEM char joystickStr0[] = "JoyX";
const PROGMEM char joystickStr1[] = "JoyY";
const PROGMEM char joystickStr2[] = "JoyZ";
const PROGMEM CFilter joysticks[] =
{
  //      name          thresh  origin ormrgn min   mnmrgn max   mxmrgn
  CFilter(joystickStr0,   16,     2048,   128,    0,     16,  4096,  32),
  CFilter(joystickStr1,   16,     2048,   128,    0,     16,  4096,  32),
  CFilter(joystickStr2,   16,     2048,   128,    0,     16,  4096,  32),
};

// Analog filters for foot controllers.
const PROGMEM char footCtrlStr0[] = "FC1";
const PROGMEM char footCtrlStr1[] = "FC2";
const PROGMEM CFilter footCtrls[] =
{
  //      name          thresh  origin ormrgn min   mnmrgn max   mxmrgn
  CFilter(footCtrlStr0,   16,     530,     32,    0,     16,  4096,  64),
  CFilter(footCtrlStr1,   16,     530,     32,    0,     16,  4096,  64),
};

// Analog filters for control dials on panel.
const PROGMEM char dialStr0[] = "Volume";
const PROGMEM char dialStr1[] = "Brilliance";
const PROGMEM char dialStr2[] = "Trem Rate";
const PROGMEM CFilter dialCtrls[] =
{
  //      name       thresh  origin ormrgn min   mnmrgn max   mxmrgn
  CFilter(dialStr0,   16,     0,     32,    0,     16,  4096,  32),
  CFilter(dialStr1,   16,     0,     32,    0,     16,  4096,  32),
  CFilter(dialStr2,   16,     0,     32,    0,     16,  4096,  32),
};

// Logical switch scanner.
const PROGMEM char footSwitchesStr0[] = "Damper";
const PROGMEM char footSwitchesStr1[] = "Soft";
const PROGMEM CSwitch footSwitches[] =
{
  CSwitch(footSwitchesStr0),
  CSwitch(footSwitchesStr1),
};
const PROGMEM char modeSwitchStr[] = "Mode";
const PROGMEM CSwitch modeSwitch(modeSwitchStr);
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

//MIDI_CREATE_DEFAULT_INSTANCE();


// Regular scanning of input switches.
void scan_switches(void)
{
  // From CAT9555 ports.
  footSwitches[0].scan(
    (RegTS.read() & 0x0080) == 0);
  footSwitches[1].scan(
    (RegTS.read() & 0x0040) == 0);
  modeSwitch.scan(
    (RegTS.read() & 0x0100) != 0);

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

void scan_analogs(void)
{
  joysticks[0].scan(AnalogA.read(6)); // Joystick X
  joysticks[1].scan(AnalogA.read(7)); // Joystick Y
  joysticks[2].scan(AnalogA.read(5)); // Joystick Z
  footCtrls[0].scan(AnalogA.read(3)); // FC1
  footCtrls[1].scan(AnalogA.read(1)); // FC2
  dialCtrls[0].scan(AnalogA.read(4)); // Volume
  dialCtrls[1].scan(AnalogA.read(2)); // Brilliance
  dialCtrls[2].scan(AnalogA.read(0)); // Tremolo Rate
}

// Scanning of switches, updating of LEDs in the 4x3 matrix.
static unsigned switch_led_scan_column_index = 0;

void switch_led_sync_led_states_this_column(void)
{
  const uint16_t mask = 0xe1ff;
  uint16_t val = (RegTS.read() & mask);

  if (switchLedMatrix[switch_led_scan_column_index].ledState())     // Row 1
    val |= 0x1000;
  if (switchLedMatrix[switch_led_scan_column_index + 3].ledState()) // Row 2
    val |= 0x0800;
  if (switchLedMatrix[switch_led_scan_column_index + 6].ledState()) // Row 3
    val |= 0x0400;
  if (switchLedMatrix[switch_led_scan_column_index + 9].ledState()) // Row 4
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
  // Scan the rows.
  switchLedMatrix[switch_led_scan_column_index].scan(     // Row 1
    (RegTS.read() & 0x0001) != 0);
  switchLedMatrix[switch_led_scan_column_index + 3].scan( // Row 2
    (RegTS.read() & 0x0002) != 0);
  switchLedMatrix[switch_led_scan_column_index + 6].scan( // Row 3
    (RegTS.read() & 0x0004) != 0);
  switchLedMatrix[switch_led_scan_column_index + 9].scan( // Row 4
    (RegTS.read() & 0x0008) != 0);
}

static unsigned kbd_scan_column_index = 0;

void kbd_scan_next_column_sequence( void )
{
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

  // Initialize the I2C bus.
  Wire.begin();
  Wire.setClock(wireClockFrequency);

  // Initialize serial UART for debug output.
  Serial.begin(9600);
  Serial.println(F("Chi - stage 1 version 1"));

  // Probe and initialize the CAT9555 expansion ports.
  RegRQ.begin();
  RegTS.begin();

  // Probe and initialize the AD7997 analog ports.
  AnalogA.begin();
}

// The loop() function runs over and over again.
uint8_t switchLedSeq[12] = { 0, 3, 2, 5, 8, 11, 1, 4, 7, 10, 6, 9 };
void loop()
{
  unsigned long currentMillis = millis();
  static unsigned led = 0;

  if (currentMillis - ledPreviousMillis >= ledBlinkInterval) {
    // save the last time you blinked the LED
    ledPreviousMillis = currentMillis;

    // Toggle LED state
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // Flush output state to LED
    digitalWrite(ledPin, ledState);

#if 0
    Serial.print(F("Analogs: ["));
    for (uint8_t i = 0; i < CAd7997::E_NUM_PORTS; i++) {
      Serial.print(F(" #"));
      Serial.print(i);
      Serial.print(F(":"));
      Serial.print(AnalogA.read(i));
    }
    Serial.println(F(" ]"));
#endif

    for (unsigned i = 0; i < 12; i++) {
      switchLedMatrix[switchLedSeq[i]].setLedState(i == led);
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
  AnalogA.sync();
  scan_analogs();
}

