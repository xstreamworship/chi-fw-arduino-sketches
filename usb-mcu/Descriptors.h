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
 *  Header file for Descriptors.c.
 */
 
#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

	/* Includes: */
		#include <avr/pgmspace.h>

		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/USB/Class/CDC.h>
		#include <LUFA/Drivers/USB/Class/MIDI.h>
		
	/* Product-specific definitions: */
		#define ARDUINO_UNO_PID			0x0001
		#define ARDUINO_MEGA2560_PID		0x0010
		#define ATMEL_LUFA_DEMO_PID		0x204B

	/* Macros for CDC */
		/** Endpoint number of the CDC device-to-host notification IN endpoint. */
		#define CDC_NOTIFICATION_EPNUM         2
		/** Endpoint number of the CDC device-to-host data IN endpoint. */
		#define CDC_TX_EPNUM                   3	
		/** Endpoint number of the CDC host-to-device data OUT endpoint. */
		#define CDC_RX_EPNUM                   4	
		/** Size in bytes of the CDC device-to-host notification IN endpoint. */
		#define CDC_NOTIFICATION_EPSIZE        8
		/** Size in bytes of the CDC data IN and OUT endpoints. */
		#define CDC_TXRX_EPSIZE                64	

	/* Macros for MIDI */
		/** Endpoint number of the MIDI streaming data IN endpoint, for device-to-host data transfers. */
		#define MIDI_STREAM_IN_EPNUM        2
		/** Endpoint number of the MIDI streaming data OUT endpoint, for host-to-device data transfers. */
		#define MIDI_STREAM_OUT_EPNUM       1
		/** Endpoint size in bytes of the Audio isochronous streaming data IN and OUT endpoints. */
		#define MIDI_STREAM_EPSIZE          64
		
	/* Type Defines for CDC */
		/** Type define for the device configuration descriptor structure. This must be defined in the
		 *  application code, as the configuration descriptor contains several sub-descriptors which
		 *  vary between devices, and which describe the device's usage to the host.
		 */
		typedef struct
		{
			USB_Descriptor_Configuration_Header_t    Config;
			USB_Descriptor_Interface_t               CDC_CCI_Interface;
			CDC_FUNCTIONAL_DESCRIPTOR(2)             CDC_Functional_IntHeader;
			CDC_FUNCTIONAL_DESCRIPTOR(1)             CDC_Functional_AbstractControlManagement;
			CDC_FUNCTIONAL_DESCRIPTOR(2)             CDC_Functional_Union;
			USB_Descriptor_Endpoint_t                CDC_NotificationEndpoint;
			USB_Descriptor_Interface_t               CDC_DCI_Interface;
			USB_Descriptor_Endpoint_t                CDC_DataOutEndpoint;
			USB_Descriptor_Endpoint_t                CDC_DataInEndpoint;
		} USB_Descriptor_ConfigurationCDC_t;

	/* Type Defines for MIDI */
		typedef struct
		{
			USB_Descriptor_Header_t   Header; /**< Regular descriptor header containing the descriptor's type and length */
			uint8_t                   Subtype; /**< Sub type value used to distinguish between audio class specific descriptors */

			uint8_t                   TotalEmbeddedJacks; /**< Total number of jacks inside this endpoint */
			uint8_t                   AssociatedJackID[2]; /**< IDs of each jack inside the endpoint */
		} USB_MIDI_DualJack_Endpoint_t;

		typedef struct
		{
			USB_Descriptor_Header_t   Header; /**< Regular descriptor header containing the descriptor's type and length */
			uint8_t                   Subtype; /**< Sub type value used to distinguish between audio class specific descriptors */

			uint8_t                   TotalEmbeddedJacks; /**< Total number of jacks inside this endpoint */
			uint8_t                   AssociatedJackID[3]; /**< IDs of each jack inside the endpoint */
		} USB_MIDI_TripleJack_Endpoint_t;

		typedef struct
		{
			USB_Descriptor_Configuration_Header_t Config;
			USB_Descriptor_Interface_t            Audio_ControlInterface;
			USB_Audio_Interface_AC_t              Audio_ControlInterface_SPC;
			USB_Descriptor_Interface_t            Audio_StreamInterface;
			USB_MIDI_AudioInterface_AS_t          Audio_StreamInterface_SPC;
			USB_MIDI_In_Jack_t                    MIDI_In_Jack_Emb_to_MIDI_Out;
			USB_MIDI_In_Jack_t                    MIDI_In_Jack_Emb_to_MIDI_Thru;
			USB_MIDI_In_Jack_t                    MIDI_In_Jack_Emb_to_Internal;
			USB_MIDI_In_Jack_t                    MIDI_In_Jack_Ext_src_MIDI_In;
			USB_MIDI_In_Jack_t                    MIDI_In_Jack_Ext_src_Internal;
			USB_MIDI_Out_Jack_t                   MIDI_Out_Jack_Emb_from_MIDI_In;
			USB_MIDI_Out_Jack_t                   MIDI_Out_Jack_Emb_from_Internal;
			USB_MIDI_Out_Jack_t                   MIDI_Out_Jack_Ext_dst_MIDI_Out;
			USB_MIDI_Out_Jack_t                   MIDI_Out_Jack_Ext_dst_MIDI_Thru;
			USB_MIDI_Out_Jack_t                   MIDI_Out_Jack_Ext_dst_Internal;
			USB_Audio_StreamEndpoint_Std_t        MIDI_In_Jack_Endpoint;
			USB_MIDI_TripleJack_Endpoint_t        MIDI_In_Jack_Endpoint_SPC;
			USB_Audio_StreamEndpoint_Std_t        MIDI_Out_Jack_Endpoint;
			USB_MIDI_DualJack_Endpoint_t          MIDI_Out_Jack_Endpoint_SPC;
		} USB_Descriptor_ConfigurationMIDI_t;
		
	/* Function Prototypes: */
		uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
		                                    const uint8_t wIndex,
		                                    void** const DescriptorAddress) ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
#endif

