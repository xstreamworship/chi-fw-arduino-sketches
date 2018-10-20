# chi-fw-arduino-sketches
CHI Firmware / Arduino Sketches

Work-in-progress
================

Chi-1p-40 is my first MIDI keyboard built from bits and pieces of hardware, including:
 - 40 key section of weighted keys from a Roland HP-5600.
 - LED/Switch matrix and MIDI / foot switch jack interface boards from Rolan HP-5600.
 - Other bits of metal and hardware from the Rolan HP-5600.
 - Custom designed / built boards (all the Roland boards were mod'ed as well).
 - Set of nine (9) Hammond organ draw bars, rewired / mod'ed to be scanned as inputs.
 - Assembly (3D printed) to hold an industrial grade joystick and a toggle switch to
   control rotary effect speed.
 - Numerous wire harnesses (many mod'ed from hardware pulled from the Roland).
 - Two Arduino boards.
 - A USB 3.0 / 2.0 hub.
 - A 5 volt power supply capable of powering the whole thing (including USB peripherals).

This project contains the firmware for the three CHI MCUs (C/C++ - some as Arduino sketches).

main-mcu - Firmware for the Main MCU (ATmega2560 in the Arduino Mega2560 r3) - Arduino sketch
aux-mcu  - Firmware for the Aux MCU (ATmega132 in the Arduino Pro Mini) - Arduino sketch
usb-mcu  - Firmware for the USB MCU (ATmega16u2 in the Arduino Mega2560) - AVR GCC project

You will need the Arduino IDE/toolchain to build the Arduino sketches and AVR GCC toolchain
to build these.  These toolchains are available for Windows, MacOS and Linux.

Contents
========

Main-MCU:
    MidiKeySwitch.[cpp|h]   - Filter that translates keyboard input samples into note on/off like events.
    LedSwitch.[cpp|h]       - Filter that translates switch input samples into CC like events and sets
                              LED status.
    MidiPort.[cpp|h]        - Handle the MIDI I/O and message assembly / disassembly.
    main-mcu.ino            - The sketch main file with I/O mapping and the Main firmware app.

Aux-MCU:
    Ad7997.[cpp|h]          - Analogue I/O driver for AD7997 8 channel ADC.
    Cat9555.[cpp|h]         - Digital I/O driver for CAT9555 16 line port.
    Drawbar.[cpp|h]         - Filter that scans the Hammond organ drawbars.
    Filter.[cpp|h]          - Filter that translates analogue input sample stream into CC like events.
    MidiPort.[cpp|h]        - Handle the MIDI I/O and message assembly (B2B to Main-MCU).
    Switch.[cpp|h]          - Filter that translates switch input samples into CC like events.
    twi_if.[cpp|h]          - Alternate driver for the I2C interface to AD7997 and CAT9555 devices.
    aux-mcu.ino             - The sketch main file with I/O mapping and the Aux firmware app.

USB-MCU:
    The USB MCU firmware is modified from the source obtained from dualMocoLUFA Project (find it on
    github).  Modified from source originally from LUFA project (fourwalledcubicle.com).  It consists
    of source to setup USB descriptors and to handle the USB end-point event / data flows.  I modified
    the Makefile to extract the LUFA USB library from a zip archive (included) rather than to assume
    that you have it checked out elsewhere.

Copyright
=========

Copyright (C) 2018, Darcy Watkins
All Rights Reserved

Note: Some source files are derived works, having additional copyrights from other parties.  See the
      source files.  This project also includes LUFA-100807 USB library from fourwalledcubicle.com,
      under its own copyright and license.

License
=======

Mozzilla Public License 2.0
See LICENSE file for details.

Note: There is a good chance that if you find any of this to be useful, most likely you are cherry
      picking and modifying selected source files and adapting this to your own hardware design.
      This means pay particular attention to license terms related to derived works.  Cheers!

