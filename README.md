# chi-fw-arduino-sketches
CHI Firmware Arduino Sketches

Work-in-progress
================

This project contains the firmware for CHI in the form of Arduino sketches (C/C++).

You will need the Arduino IDE/toolchain to build the firmware (or a supported alternate toolchain).

Contents
========

Ad7997.[cpp|h]          - Analogue I/O driver for AD7997 8 channel ADC.
Cat9555.[cpp|h]         - Digital I/O driver for CAT9555 16 line port.
Filter.[cpp|h]          - Filter that translates analogue input sample stream into CC like events.
MidiKeySwitch.[cpp|h]   - Filter that translates keyboard input samples into note on/off like events.
Switch.[cpp|h]          - Filter that translates switch input samples into CC like events.

chi-firmware.ino        - The sketch main file with I/O mapping and the 4x3 LED/SW matrix scanning.

Copyright
=========

Copyright (C) 2018, Darcy Watkins
All Rights Reserved

License
=======

Mozzilla Pubblic License 2.0
See LICENSE file for details.
