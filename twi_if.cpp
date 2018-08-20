/////////////////////////////////////////////////////////////////////
// TWI/I2C Driver.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////

#include <avr/io.h>
#include <compat/twi.h>
#include <avr/interrupt.h>
#include "pins_arduino.h"
#include "twi_if.h"

static volatile uint8_t master_sla;
static volatile uint8_t *master_data_rd_ptr;
static volatile uint8_t *master_data_wr_ptr;
static volatile uint8_t master_ddr;
static volatile uint8_t master_rd_bytes;
static volatile uint8_t master_wr_bytes;
static volatile uint8_t master_sla_retry_cnt;
static volatile uint8_t master_arb_retry_cnt;
static volatile boolean master_busy;
static volatile boolean master_response_ok;
static volatile boolean master_blocked;
static uint8_t master_sla_number_of_attempts;
static uint8_t master_arb_number_of_attempts;
static volatile uint8_t bus_error;
static volatile uint8_t *bus_data_ptr;
static volatile uint8_t bus_data_bytes;
static volatile uint8_t slave_sla;
static volatile uint8_t *slave_data_rd_ptr;
static volatile uint8_t *slave_data_wr_ptr;
static volatile uint8_t slave_rd_bytes;
static volatile uint8_t slave_wr_bytes;
static volatile uint8_t slave_data_count;
static volatile uint8_t slave_transfer_type;
static volatile uint8_t *slave_default_wr_ptr;
static volatile uint8_t slave_default_wr_bytes;
static volatile uint8_t slave_default_msg;
static void (*slave_transfer_request_callback)(uint8_t xtype, uint8_t **ptrptr, uint8_t *szptr);
static void (*slave_transfer_complete_callback)(uint8_t xtype, uint8_t count);
static volatile uint8_t bit_bucket;
static volatile uint32_t *done_at_time;

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

/////////////////////////////////////////////////////////////////////
// twi_init
//
// Initialize the TWI driver
/////////////////////////////////////////////////////////////////////
void twi_init(uint32_t fosc, uint32_t twi_clk_speed)
{
  // Internal pull-up resistors.
  digitalWrite(SDA, 1);
  digitalWrite(SCL, 1);


  // initialize twi prescaler and bit rate
  cbi(TWSR, TWPS0);
  cbi(TWSR, TWPS1);
//#if defined(TWPS0)
//	TWSR = 0;
//#endif

	TWBR = ((fosc / twi_clk_speed) - 16) / 2;

	// Default setup is ready to write nothing from nowhere to nobody
	master_sla = 0;
	master_data_rd_ptr = NULL;
	master_data_wr_ptr = NULL;
	master_ddr = TW_WRITE;
	master_rd_bytes = 0;
	master_wr_bytes = 0;
	master_sla_retry_cnt = 0;
	master_arb_retry_cnt = 0;
	master_busy = false;
	master_response_ok = false;
	master_blocked = false;
	master_sla_number_of_attempts = 3;
	master_arb_number_of_attempts = 10;
	bus_error = false;
	bus_data_ptr = NULL;
	bus_data_bytes = 0;
	slave_sla = 0;
	slave_data_rd_ptr = NULL;
	slave_data_wr_ptr = NULL;
	slave_rd_bytes = 0;
	slave_wr_bytes = 0;
	slave_data_count = 0;
	slave_default_wr_ptr = &slave_default_msg;
	slave_default_wr_bytes = 1;
	slave_default_msg = 0;
	slave_transfer_request_callback = NULL;
	slave_transfer_complete_callback = NULL;
	// Default slave_transfer_type is receive.  This is overridden by
	// general call or slave tx but must default to receive for some
	// of the "switch" block fall throughs to properly work.
	slave_transfer_type = TWI_SLAVE_RX;

  // Enable TWI module, acks, and interrupt
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

/////////////////////////////////////////////////////////////////////
// twi_assign_slave_sla
//
// Setup the TWI driver slave address and options
/////////////////////////////////////////////////////////////////////
void twi_assign_slave_sla(uint8_t slave_address, uint8_t general_call)
{
	// Protect from interrupts (this must be an atomic operation)
  noInterrupts();
	// Set up the slave address and general call options (SLA)
	slave_sla = (slave_address << 1); // MS 7 bits is address
	if (general_call)
		slave_sla |= 0x01; // LS bit is general call option
	// Check if TWI is active
	if (!master_busy && !master_blocked)
	{
		// Set the SLA register in the TWI
		TWAR = slave_sla;
		// Enable operation
		if (slave_sla)
			TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
		else
			TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
	}

	// Restore interrupts
  interrupts();
}

/////////////////////////////////////////////////////////////////////
// twi_master_setup
//
// Setup the TWI driver master mode parameters
/////////////////////////////////////////////////////////////////////
void twi_master_setup(uint8_t sla_attempts, uint8_t arb_attempts)
{
	master_sla_number_of_attempts = sla_attempts;
	master_arb_number_of_attempts = arb_attempts;
}

/////////////////////////////////////////////////////////////////////
// twi_slave_setup
//
// Setup the TWI driver slave address, options, default response
// and application callbacks
/////////////////////////////////////////////////////////////////////
void twi_slave_setup(uint8_t slave_address, uint8_t general_call,
	uint8_t *default_message, uint8_t default_message_length,
	void (*transfer_request_callback)
		(uint8_t xtype, uint8_t **ptrptr, uint8_t *szptr),
	void (*transfer_complete_callback)(uint8_t xtype, uint8_t count)
	)
{
	// Protect from interrupts (this must be an atomic operation)
	noInterrupts();
	// Set up the slave address and general call options (SLA)
	twi_assign_slave_sla(slave_address, general_call);
	// Set up the location and length for the default slave
	// response message.  This can be used to ship out a
	// canned error message when we are not ready with data
	// or it can be used for simple cases where there is only
	// one thing that can be read.
	if (default_message_length && default_message)
	{
		// valid pointer and length
		slave_default_wr_ptr = default_message;
		slave_default_wr_bytes = default_message_length;
	}
	else
	{
		// use the default
		slave_default_wr_ptr = &slave_default_msg;
		slave_default_wr_bytes = 1;
	}
	// Set up the callbacks
	slave_transfer_request_callback = transfer_request_callback;
	slave_transfer_complete_callback = transfer_complete_callback;
	// Restore interrupts
	interrupts();
}

/////////////////////////////////////////////////////////////////////
// twi_initiate_transaction
//
// Read bytes from a TWI peripheral
/////////////////////////////////////////////////////////////////////
int8_t twi_initiate_transaction(uint8_t address, uint8_t *data_wr,
	uint8_t bytes_wr, uint8_t *data_rd, uint8_t bytes_rd,
	uint32_t *doneAtPtr)
{
	noInterrupts(); // protect atomic operation

  done_at_time = doneAtPtr;

	// Bus is busy so wait and then try again
	while (master_busy)
	{
		// Allow for interuption but ensure that test and exit
		// from loop is atomic
		interrupts();
		noInterrupts();
	}

	// Reserve bus as busy by us
	master_busy = true;
	master_response_ok = false;
//	bus_error = false;

	while (TWCR & (1 << TWSTO))
	{
		// Allow for interuption but ensure that test and exit from
		// loop is atomic
		interrupts();
		noInterrupts();
	}

	master_sla = address;
	master_data_rd_ptr = data_rd;
	master_rd_bytes = bytes_rd;
	master_data_wr_ptr = data_wr;
	master_wr_bytes = bytes_wr;
	if (master_wr_bytes && !(master_rd_bytes && !master_sla))
		master_ddr = TW_WRITE;
	else if (master_rd_bytes && master_sla)
		master_ddr = TW_READ;
	else
	{
		// Invalid request
		//  - request nothing send or receive
		//  - trying to read during a general call
		master_busy = false;
    if (done_at_time)
      *done_at_time = micros();
		interrupts();
		return TWI_ERR_API_PARM_INVALID;
	}

	master_arb_retry_cnt = 0;
	master_sla_retry_cnt = 0;
	
	// Generate start condition, the remainder of the transfer is
	// interrupt driven and will be performed in the background
	if (!master_blocked)
	{
		// The TWI slave logic is busy so the TWI master logic must
		// wait until the TWI bus becomes available for use.  The start
		// operation is deferred.  On completion of slave transaction,
		// we will enable the start operation.
		TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
	}
	
	interrupts();

  // No error
	return 0;
}

/////////////////////////////////////////////////////////////////////
// twi_initiate_write
//
// Write bytes to a TWI peripheral
/////////////////////////////////////////////////////////////////////
int8_t twi_initiate_write(uint8_t address, uint8_t *data, uint8_t bytes)
{
	// Transaction with write only to slave address
	return twi_initiate_transaction(address, data, bytes, NULL, 0);
}

/////////////////////////////////////////////////////////////////////
// twi_initiate_read
//
// Read bytes from a TWI peripheral
/////////////////////////////////////////////////////////////////////
int8_t twi_initiate_read(uint8_t address, uint8_t *data, uint8_t bytes)
{
	// Transaction with read only from slave address
	return twi_initiate_transaction(address, NULL, 0, data, bytes);
}

/////////////////////////////////////////////////////////////////////
// twi_busy
//
// Check if the TWI interface is busy in case you want to avoid a
// wait loop from a read or write operation.
/////////////////////////////////////////////////////////////////////
boolean twi_busy(void)
{
	return master_busy;
}

/////////////////////////////////////////////////////////////////////
// twi_wait_until_master_ready
//
// Wait loop.
/////////////////////////////////////////////////////////////////////
void twi_wait_until_master_ready(void)
{
	while (master_busy)
	{
	}
}

/////////////////////////////////////////////////////////////////////
// twi_response
//
// Check for a response from the TWI interface.
/////////////////////////////////////////////////////////////////////
boolean twi_response(void)
{
	return master_response_ok;
}

/////////////////////////////////////////////////////////////////////
// SIGNAL SIG_2WIRE_SERIAL
//
// Interrupt handler for TWI bus
/////////////////////////////////////////////////////////////////////
ISR(TWI_vect)
{
	// Mask out prescaler bits to get TWI status
	uint8_t TWI_status = TWSR & TW_STATUS_MASK;
	
	switch(TWI_status)
	{
	case TW_START:			// Start condition
	case TW_REP_START:		// Repeated start condition
		// Transmit SLA + Read or Write
		TWDR = (master_sla << 1) + master_ddr;
		// Affect any pending change in slave address
		if (TWAR != slave_sla)
			TWAR = slave_sla;
		// TWSTA must be cleared by software! This also clears TWINT!!!
		TWCR &= ~(1 << TWSTA);
		break;

	case TW_MT_SLA_ACK:		// Slave acknowledged address,
		// At beginning so clear retry counter
		master_sla_retry_cnt = 0;
		// Transmit data and increment pointer
		TWDR = *master_data_wr_ptr;
		bus_data_ptr = master_data_wr_ptr + 1;
		bus_data_bytes = master_wr_bytes;
		// Clear TWINT to continue
		TWCR |= (1 << TWINT);
		break;

	case TW_MT_SLA_NACK:	// Slave didn't acknowledge address,
	case TW_MR_SLA_NACK:
		// this may mean that the slave is disconnected
		master_sla_retry_cnt++;
		if (master_sla_retry_cnt > (master_sla_number_of_attempts - 1))
		{
			// If 3 times NACK abort
			// Affect any pending change in slave address
			if (TWAR != slave_sla)
				TWAR = slave_sla;
			// Generate stop condition
			TWCR |= (1 << TWINT) | (1 << TWSTO) | (1 << TWEA);
			// Bus no longer busy
			master_busy = false;
      if (done_at_time)
        *done_at_time = micros();
		}
		else
		{
			// Try again up to 3 attempts
			TWCR |= (1 << TWINT) | (1 << TWSTA) | (1 << TWSTO);
		}
		break;

	case TW_MT_DATA_ACK:	// Slave Acknowledged data,
		if(--bus_data_bytes > 0)
		{
			// If there is more data to send, send it and increment
			// pointer
			TWDR = *(bus_data_ptr++);
			// Clear TWINT to continue
			TWCR |= (1 << TWINT);
		}
		else if (master_rd_bytes)
		{
			// Completed sending data but expect to read some back
			// now so initiate a repeat start and set up for read
			// Change direction to read
			master_ddr = TW_READ;
			// Generate the repeated start condition
			TWCR |= (1 << TWSTA) | (1 << TWINT);
			// Start over for slave address retries
			master_sla_retry_cnt = 0;
		}
		else
		{
			// Normal completion of transmit
			// Affect any pending change in slave address
			if (TWAR != slave_sla)
				TWAR = slave_sla;
			// Generate the stop condition
			TWCR |= (1 << TWSTO) | (1 << TWINT) | (1 << TWEA);
			// Bus no longer busy
			master_busy = false;
      if (done_at_time)
        *done_at_time = micros();
			// Response has been received
			master_response_ok = true;
		}
		break;

	case TW_MT_DATA_NACK:	// Slave didn't acknowledge data
		// Check if the slave is just not acknowledging the last byte
		// (or could have halted due to an error).
		if (bus_data_bytes == 1)
		{
			// Slave just not acknowledging the last byte
			if (master_rd_bytes)
			{
				// Completed sending data but expect to read some back
				// now so initiate a repeat start and set up for read
				// Change direction to read
				master_ddr = TW_READ;
				// Generate the repeated start condition
				TWCR |= (1 << TWSTA) | (1 << TWINT);
				// Start over for slave address retries
				master_sla_retry_cnt = 0;
				break;
			}
			// Otherwise end of normal transmit
			// Response has been received
			master_response_ok = true;
		}

		// Affect any pending change in slave address
		if (TWAR != slave_sla)
			TWAR = slave_sla;

		// Send stop condition
		TWCR |= (1 << TWINT) | (1 << TWSTO) | (1 << TWEA);
		// Bus no longer busy
		master_busy = false;
    if (done_at_time)
      *done_at_time = micros();
		break;

	case TW_MR_SLA_ACK:		// Slave acknowledged address
		// If there is more than one byte to read acknowledge it
		// otherwise don't
		bus_data_bytes = master_rd_bytes - 1;
		bus_data_ptr = master_data_rd_ptr;
		if (bus_data_bytes > 0)
			TWCR |= (1 << TWEA) | (1 << TWINT);
		else
			TWCR |= (1 << TWINT);
		break;

	case TW_MR_DATA_ACK:	// Master acknowledged data
		// Read received data byte and increment pointer
		*(bus_data_ptr++) = TWDR;
		if (--bus_data_bytes > 0)
			// Get next databyte and acknowledge
			TWCR |= (1 << TWEA) | (1 << TWINT);	
		else
			// Enable Acknowledge must be cleared by software,
			// this also clears TWINT!!!
			TWCR &= ~(1 << TWEA);
		break;

	case TW_MR_DATA_NACK:	// Master didn't ack data -> end of read process
		// Read received data byte
		*bus_data_ptr = TWDR;

		// Affect any pending change in slave address
		if (TWAR != slave_sla)
			TWAR = slave_sla;

		// Generate stop condition
		TWCR |= (1 << TWSTO) | (1 << TWINT) | (1 << TWEA);
		// Bus no longer busy
		master_busy = false;
    if (done_at_time)
      *done_at_time = micros();
		// Response has been received
		master_response_ok = true;
		break;

	case TW_MT_ARB_LOST: 	// We lost to another master during SLA+R/W
	//case TW_MR_ARB_LOST: 	- redundant as it is same value as TW_MT_ARB_LOST
		// Affect any pending change in slave address
		if (TWAR != slave_sla)
			TWAR = slave_sla;

		master_arb_retry_cnt++;
		if (master_arb_retry_cnt > (master_arb_number_of_attempts - 1))
		{
			// It is likely that bus is overloaded with activity or multiple
			// masters erroneously have the same address (or competing roles).
			// Generate stop condition
			TWCR |= (1 << TWSTO) | (1 << TWINT) | (1 << TWEA);
			// Bus no longer busy
			master_busy = false;
      if (done_at_time)
        *done_at_time = micros();
		}
		else
		{
			// Set up to generate the start condition as soon as the bus becomes
			// available.  Then we will try it all over again.
			TWCR |= (1 << TWSTA) | (1 << TWINT);
			// Start over for slave address retries
			master_sla_retry_cnt = 0;
		}
		break;

	case TW_ST_ARB_LOST_SLA_ACK: // We lost to another master addressing us
	case TW_ST_SLA_ACK:		// We have been addressed as a slave (SLA+R)
		// Block any upcoming master transactions
		// (or blocked by arbitration failure)
		master_blocked = true;
		// Set transfer type
		slave_transfer_type = TWI_SLAVE_TX;
		// Invoke callback
		if (slave_transfer_request_callback)
            (*(slave_transfer_request_callback))(slave_transfer_type,
				&slave_data_wr_ptr, &slave_wr_bytes);
		// Check for slave write data or use default response
		if (slave_data_wr_ptr)
		{
			// Have data ready to send back
			bus_data_ptr = slave_data_wr_ptr;
			bus_data_bytes = slave_wr_bytes;
		}
		else
		{
			// Nothing there so use default (which usually is set up
			// to point to a canned error message or a default data
			// read area).
			bus_data_ptr = slave_default_wr_ptr;
			bus_data_bytes = slave_default_wr_bytes;
		}

		// Clear the source pointer and size now that it has been
		// transfered into teh working variables
		slave_data_wr_ptr = NULL;
		slave_wr_bytes = 0;

		// Start count of actual data transfered
		slave_data_count = 0;

		// Fall through to send first byte of data

	case TW_ST_DATA_ACK:		// Data sent and Ack received from master
		// Now output the data
		TWDR = *(bus_data_ptr++);
		slave_data_count++;
		// Specify whether or not we expect acknowledge
		if (bus_data_bytes-- > 1)
			TWCR |= (1 << TWEA) | (1 << TWINT);
		else
			// Enable Acknowledge must be cleared by software,
			// this also clears TWINT!!!
			TWCR &= ~(1 << TWEA);
		break;

	case TW_SR_ARB_LOST_GCALL_ACK: // We lost to another master ...
	case TW_SR_GCALL_ACK:	// General call address has been invoked
		// Set transfer type
		slave_transfer_type = TWI_GENERAL_CALL;
		// Fall through to otherwise handle the same as for slave addressed.

	case TW_SR_ARB_LOST_SLA_ACK: // We lost to another master addressing us
	case TW_SR_SLA_ACK:		// We have been addressed as a slave (SLA+W)
		// Note: We rely on slave_transfer_type defaulting to TWI_SLAVE_RX
		//       to distinguish between slave receive and general call.
		// Block any upcoming master transactions
		// (or blocked by arbitration failure)
		master_blocked = true;
		// Invoke callback
		if (slave_transfer_request_callback)
            (*(slave_transfer_request_callback))(slave_transfer_type,
				&slave_data_rd_ptr, &slave_rd_bytes);
		// Check for slave read data or default to bit bucket
		if (slave_data_rd_ptr)
		{
			// Have data ready to send back
			bus_data_ptr = slave_data_rd_ptr;
			bus_data_bytes = slave_rd_bytes - 1;
		}
		else
		{
			// Nothing there so use bit bucket
			bus_data_ptr = NULL;
			bus_data_bytes = 0;
		}

		// Clear the source pointer and size now that it has been
		// transfered into teh working variables
		slave_data_rd_ptr = NULL;
		slave_rd_bytes = 0;

		// Start count of actual data transfered
		slave_data_count = 0;

		// Set up for Ack / not ack
		if (bus_data_bytes > 0)
			// Get next databyte and acknowledge
			TWCR |= (1 << TWEA) | (1 << TWINT);
		else
			// Enable Acknowledge must be cleared by software,
			// this also clears TWINT!!!
			TWCR &= ~(1 << TWEA);
		break;

	case TW_SR_DATA_ACK:
	case TW_SR_GCALL_DATA_ACK:
		// Read received data byte and increment pointer
		*(bus_data_ptr++) = TWDR;
		slave_data_count++;
		if (--bus_data_bytes > 0)
			// Get next databyte and acknowledge
			TWCR |= (1 << TWEA) | (1 << TWINT);	
		else
			// Enable Acknowledge must be cleared by software,
			// this also clears TWINT!!!
			TWCR &= ~(1 << TWEA);
		break;

	case TW_SR_DATA_NACK:		// Received last data byte to us
	case TW_SR_GCALL_DATA_NACK:	// Received last data byte of general call
		// Read the last received data byte (that has been not ack'd) and then
		// complete the slave data transfer.
		if (bus_data_ptr)
		{
			*bus_data_ptr = TWDR;
			slave_data_count++;
		}
		else
			bit_bucket = TWDR; // read and discard

		// Fall through to perform completion of slave data transfer.

	case TW_ST_DATA_NACK:		// Data sent and not ack from master
	case TW_ST_LAST_DATA:		// Last data sent but master still ack'd
	case TW_SR_STOP:			// Instead of receive byte, we got a stop or repeat start
		// Invoke callback for completion of slave data transfer
		if (slave_transfer_complete_callback)
            (*(slave_transfer_complete_callback))(slave_transfer_type, slave_data_count);
		// Reset the transfer type to the default of slave receive
		slave_transfer_type = TWI_SLAVE_RX;

		// No longer need to block master operation
		master_blocked = false;

		// Affect any pending change in slave address
		if (TWAR != slave_sla)
			TWAR = slave_sla;

		// Check for interupted or pending master transaction and (re)initiate
		// it if the blocked threshold hasn't been exceeded.
		if (master_busy)
		{
			master_arb_retry_cnt++;
			if (master_arb_retry_cnt <= (master_arb_number_of_attempts - 1))
			{
				// Start / resume the blocked master transaction as soon as the
				// bus becomes available.  We will try it all over again.
				TWCR |= (1 << TWSTA) | (1 << TWEA) | (1 << TWINT);
				// Start over for slave address retries
				master_sla_retry_cnt = 0;
				break;
			}
			// It is likely that bus is overloaded with activity or multiple
			// masters erroneously have the same address (or competing roles).
			// Bus no longer busy
			master_busy = false;
      if (done_at_time)
        *done_at_time = micros();
		}

		// Go back to slave monitoring mode
		TWCR |= (1 << TWEA) | (1 << TWINT);
		break;

	case TW_BUS_ERROR:
	default: // Unhandled state - error
		if (master_busy)
		{
			// Generate stop condition
			TWCR |= (1 << TWINT) | (1 << TWSTO);
			// Bus no longer busy
			master_busy = false;
      if (done_at_time)
        *done_at_time = micros();
			master_blocked = false;
		}
		bus_error = TWI_status;
		break;
	}
}

