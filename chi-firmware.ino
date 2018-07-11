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
  //      name        debounce  thresh  origin ormrgn min   mnmrgn max   mxmrgn
  CFilter(joystickStr0, 30000,   16,     2048,   128,    0,     16,  4096,  32),
  CFilter(joystickStr1, 30000,   16,     2048,   128,    0,     16,  4096,  32),
  CFilter(joystickStr2, 30000,   16,     2048,   128,    0,     16,  4096,  32),
};

// Analog filters for foot controllers.
const PROGMEM char footCtrlStr0[] = "FC1";
const PROGMEM char footCtrlStr1[] = "FC2";
const PROGMEM CFilter footCtrls[] =
{
  //      name        debounce  thresh  origin ormrgn min   mnmrgn max   mxmrgn
  CFilter(footCtrlStr0, 30000,   16,     530,     32,    0,     16,  4096,  64),
  CFilter(footCtrlStr1, 30000,   16,     530,     32,    0,     16,  4096,  64),
};

// Analog filters for control dials on panel.
const PROGMEM char dialStr0[] = "Volume";
const PROGMEM char dialStr1[] = "Brilliance";
const PROGMEM char dialStr2[] = "Trem Rate";
const PROGMEM CFilter dialCtrls[] =
{
  //      name     debounce  thresh  origin ormrgn min   mnmrgn max   mxmrgn
  CFilter(dialStr0, 30000,   16,     0,     32,    0,     16,  4096,  32),
  CFilter(dialStr1, 30000,   16,     0,     32,    0,     16,  4096,  32),
  CFilter(dialStr2, 30000,   16,     0,     32,    0,     16,  4096,  32),
};

// Logical switch scanner.
const PROGMEM char footSwitchesStr0[] = "Damper";
const PROGMEM char footSwitchesStr1[] = "Soft";
const PROGMEM CSwitch footSwitches[] =
{
  CSwitch(footSwitchesStr0, 30000),
  CSwitch(footSwitchesStr1, 30000),
};
const PROGMEM char modeSwitchStr[] = "Mode";
const PROGMEM CSwitch modeSwitch(modeSwitchStr, 30000);
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
  CSwitch(switchLedMatrixStr0, 20000),
  CSwitch(switchLedMatrixStr1, 20000),
  CSwitch(switchLedMatrixStr2, 20000),
  CSwitch(switchLedMatrixStr3, 20000),
  CSwitch(switchLedMatrixStr4, 20000),
  CSwitch(switchLedMatrixStr5, 20000),
  CSwitch(switchLedMatrixStr6, 20000),
  CSwitch(switchLedMatrixStr7, 20000),
  CSwitch(switchLedMatrixStr8, 20000),
  CSwitch(switchLedMatrixStr9, 20000),
  CSwitch(switchLedMatrixStr10, 20000),
  CSwitch(switchLedMatrixStr11, 20000),
};
const PROGMEM char joystickButtonStr[] = "JSButton";
const PROGMEM CSwitch joystickButton(joystickButtonStr, 20000);
const PROGMEM char rotarySwStr0[] = "RotSlow";
const PROGMEM char rotarySwStr1[] = "RotFast";
const PROGMEM CSwitch rotarySw[] =
{
  CSwitch(rotarySwStr0, 30000),
  CSwitch(rotarySwStr1, 30000),
};
const PROGMEM char midiCableDetectStr[] = "MIDICable";
const PROGMEM CSwitch midiCableDetect(midiCableDetectStr, 30000);

// Logical MIDI keyboard scanning.
const PROGMEM char NoteAStr[] = "A";
const PROGMEM char NoteBbStr[] = "A#/Bb";
const PROGMEM char NoteBStr[] = "B";
const PROGMEM char NoteCStr[] = "C";
const PROGMEM char NoteDbStr[] = "C#/Db";
const PROGMEM char NoteDStr[] = "D";
const PROGMEM char NoteEbStr[] = "D#/Eb";
const PROGMEM char NoteEStr[] = "E";
const PROGMEM char NoteFStr[] = "F";
const PROGMEM char NoteGbStr[] = "F#/Gb";
const PROGMEM char NoteGStr[] = "G";
const PROGMEM char NoteAbStr[] = "G#/Ab";

const PROGMEM CMidiKeySwitch keyboard[] =
{
  CMidiKeySwitch(45, NoteAStr, 1),
  CMidiKeySwitch(46, NoteBbStr, 1),
  CMidiKeySwitch(47, NoteBStr, 1),
  CMidiKeySwitch(48, NoteCStr, 2),
  CMidiKeySwitch(49, NoteDbStr, 2),
  CMidiKeySwitch(50, NoteDStr, 2),
  CMidiKeySwitch(51, NoteEbStr, 2),
  CMidiKeySwitch(52, NoteEStr, 2),
  CMidiKeySwitch(53, NoteFStr, 2),
  CMidiKeySwitch(54, NoteGbStr, 2),
  CMidiKeySwitch(55, NoteGStr, 2),
  CMidiKeySwitch(56, NoteAbStr, 2),
  CMidiKeySwitch(57, NoteAStr, 2),
  CMidiKeySwitch(58, NoteBbStr, 2),
  CMidiKeySwitch(59, NoteBStr, 2),
  CMidiKeySwitch(60, NoteCStr, 3),
  CMidiKeySwitch(61, NoteDbStr, 3),
  CMidiKeySwitch(62, NoteDStr, 3),
  CMidiKeySwitch(63, NoteEbStr, 3),
  CMidiKeySwitch(64, NoteEStr, 3),
  CMidiKeySwitch(65, NoteFStr, 3),
  CMidiKeySwitch(66, NoteGbStr, 3),
  CMidiKeySwitch(67, NoteGStr, 3),
  CMidiKeySwitch(68, NoteAbStr, 3),
  CMidiKeySwitch(69, NoteAStr, 3),
  CMidiKeySwitch(70, NoteBbStr, 3),
  CMidiKeySwitch(71, NoteBStr, 3),
  CMidiKeySwitch(72, NoteCStr, 4),
  CMidiKeySwitch(73, NoteDbStr, 4),
  CMidiKeySwitch(74, NoteDStr, 4),
  CMidiKeySwitch(75, NoteEbStr, 4),
  CMidiKeySwitch(76, NoteEStr, 4),
  CMidiKeySwitch(77, NoteFStr, 4),
  CMidiKeySwitch(78, NoteGbStr, 4),
  CMidiKeySwitch(79, NoteGStr, 4),
  CMidiKeySwitch(80, NoteAbStr, 4),
  CMidiKeySwitch(81, NoteAStr, 4),
  CMidiKeySwitch(82, NoteBbStr, 4),
  CMidiKeySwitch(83, NoteBStr, 4),
  CMidiKeySwitch(84, NoteCStr, 5),
};

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
    (RegTS.read() & 0x0010) == 0,
    (RegTS.read() & 0x0020) == 0);
  keyboard[kbd_scan_column_index + 8].scan(  // Row B
    (RegRQ.read() & 0x0100) == 0,
    (RegRQ.read() & 0x0200) == 0);
  keyboard[kbd_scan_column_index + 16].scan( // Row C
    (RegRQ.read() & 0x0400) == 0,
    (RegRQ.read() & 0x0800) == 0);
  keyboard[kbd_scan_column_index + 24].scan( // Row D
    (RegRQ.read() & 0x1000) == 0,
    (RegRQ.read() & 0x2000) == 0);
  keyboard[kbd_scan_column_index + 32].scan( // Row E
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
