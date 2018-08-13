/////////////////////////////////////////////////////////////////////
// TWI/I2C Driver.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __TWI_IF_H
#define __TWI_IF_H

#include "Arduino.h"

#define TWI_SLAVE_RX					0
#define TWI_SLAVE_TX					1
#define TWI_GENERAL_CALL			2

#define TWI_ERR_API_PARM_INVALID		(-1)

// Prototypes
void twi_init(uint32_t fosc = 16000000UL, uint32_t twi_clk_speed = 400000UL);
void twi_assign_slave_sla(uint8_t slave_address, uint8_t general_call);
void twi_master_setup(uint8_t sla_attempts, uint8_t arb_attempts);
void twi_slave_setup(uint8_t slave_address, uint8_t general_call,
	uint8_t *default_message, uint8_t default_message_length,
	void (*transfer_request_callback)
		(uint8_t xtype, uint8_t **ptrptr, uint8_t *szptr),
	void (*transfer_complete_callback)(uint8_t xtype, uint8_t count)
	);
int8_t twi_initiate_transaction(uint8_t address, uint8_t *data_wr, uint8_t bytes_wr,
	uint8_t *data_rd, uint8_t bytes_rd);
int8_t twi_initiate_write(uint8_t address, uint8_t *data, uint8_t bytes);
int8_t twi_initiate_read(uint8_t address, uint8_t *data, uint8_t bytes);
boolean twi_busy(void);
void twi_wait_until_master_ready(void);
boolean twi_response(void);

#endif

