/*
     Chi-1p-40 USB-MCU Firmware Project
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

     Derived from dualMocoLUFA serial / USB-MIDI project
     Derived from USB-MIDI interface LUFA Library project template.
*/
/*
     dualMocoLUFA Project
     Copyright (C) 2013 by morecat_lab

     2013/09/22
              
     http://morecatlab.akiba.coocan.jp/

     based on LUFA-100807
*/
/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2018 Darcy Watkins (darcy [at] xstreamworship [dot] com)
  Copyright 2013 by morecat_lab (http://morecatlab.akiba.coocan.jp/)
  Copyright 2010 Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this 
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in 
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting 
  documentation, and that the name of the author not be used in 
  advertising or publicity pertaining to distribution of the 
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for doubleMonaka.c
 */

#ifndef _USB_MCU_H_
#define _USB_MCU_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/interrupt.h>
		#include <avr/power.h>

		#include "Descriptors.h"

		#include "Lib/LightweightRingBuff.h"

		#include <LUFA/Version.h>
		#include <LUFA/Drivers/Board/LEDs.h>
		#include <LUFA/Drivers/Peripheral/Serial.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/USB/Class/CDC.h>
		
	/* Macros: */
		/** LED mask for the library LED driver, to indicate TX activity. */
		#define LEDMASK_TX               LEDS_LED1

		/** LED mask for the library LED driver, to indicate RX activity. */
		#define LEDMASK_RX               LEDS_LED2
		
		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_ERROR            (LEDS_LED1 | LEDS_LED2)
		
		/** LED mask for the library LED driver, to indicate that the USB interface is busy. */
		#define LEDMASK_BUSY             (LEDS_LED1 | LEDS_LED2)		
		
		typedef uint8_t uchar;
		#define HW_CDC_BULK_OUT_SIZE     8
		#define HW_CDC_BULK_IN_SIZE      8

		#define	TRUE			1
		#define	FALSE			0

	/* Function Prototypes: */
		void SetupHardware(void);
		void processSerial(void);
		void processMIDI(void);

		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_UnhandledControlRequest(void);
		
		void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);
		void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);		

		uchar parseSerialMidiMessage(uchar);
		void parseUSBMidiMessage(uchar *, uchar);
	/* shared variable */
		extern uchar systemMode;

#endif /* _USB_MCU_H_ */

