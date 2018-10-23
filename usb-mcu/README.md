Chi-1p-40 USB-MCU Firmware Project
======================

Copyright (C) 2018 Darcy Watkins
2018/10/22
darcy [at] xstreamworship [dot] com
http://xstreamworship.com

Multiplexed USB-MIDI interface (3 out/2 in):
  port 0 - to/from Chi keyboard UI
  port 1 - to/from MIDI-Out/MIDI-In jacks
  port 3 - to MIDI/Thru/Out2 jack

Serial USB-MIDI multiplexed using 0xFD as a MIDI-escape sequence.
  0xFD, 0x00 | (0x0f & port_cable_id) - to change the port MIDI stream.
  0xFD, 0xFD - to insert the 0xFD byte (undefined / unused MIDI status code).
  0xFD, [any other value] - both the 0xFD and the escaped byte are discarded.

MIDI router (details to be added).

Derived from dualMocoLUFA serial / USB-MIDI project.
Derived from USB-MIDI interface LUFA Library project template.

This is dual mode firmware for Arduino Mega2560 (or Uno).
There are two modes on this firmware, multiplexed USB-MIDI with MIDI router and Arduino-Serial.

The multiplexed USB-MIDI with MIDI router mode is intended for normal operation with other Chi-1p-40 firmware.
The Arduino Serial mode is intended for support / debugging of the Main MCU firmware implemented as an Arduino sketch.

INSTRUCTIONS
1. Burn 16u2 on Arduino Mega.
   check original document below.
2. USB-MIDI firmware work as default.
3. To enable Arduino-Serial, place jumper to PIN 4(MOSI PB2) and PIN6 (ground) on ICSP connector for 16U2.
   Reset is required to switch the firmware mode.

-------------------------------------
original readme.txt from Arduino-Serial (LUFA Library)
  
To setup the project and upload the Arduino usbserial application firmware to an ATMEGA16U2 using the Arduino USB DFU bootloader:
	1. unpack the source into LUFA's Projects directory
	2. set ARDUINO_MODEL_PID in the makefile as appropriate
	3. do "make clean; make"
	4. put the 16U2 into USB DFU mode:
	4.a. assert and hold the 16U2's RESET line
	4.b. assert and hold the 16U2's HWB line
	4.c. release the 16U2's RESET line
	4.d. release the 16U2's HWB line
	5. confirm that the board enumerates as either "Arduino Uno DFU" or "Arduino Mega 2560 DFU"
	6. do "make dfu" (OS X or Linux - dfu-programmer must be installed first) or "make flip" (Windows - Flip must be installed first)

Alternately use ISP (such as USB ASP, etc) with avrdude to program.  With correct programmer selected using AVRDUDE_PROGRAMMER variable in
makefile, you can programn it by invoking "make program".  May need to prefix with 'sudo' for access permission to the programmer (Linux).

Check that the board enumerates as either "Arduino Uno" or "Arduino Mega 2560".  Test by uploading a new Arduino sketch from the Arduino IDE.

Other firmware (main-mcu and aux-mcu) are Arduino sketches.

Except where noted otherwise, the Chi-1p-40 firware is available under Mozzila Public License 2.0.
The USB MCU firmware (this sub-project) uses the LUFA Library license (a BSD style license) of the code it is derived from.

