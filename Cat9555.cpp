/////////////////////////////////////////////////////////////////////
// Digital I/O driver for CAT9555 16 line port.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Arduino.h"
#include "Cat9555.h"
#include "twi_if.h"

CCat9555::CCat9555(const uint8_t i2cAddr, const uint16_t portConfig) :
  m_i2cAddr(i2cAddr),
  m_portConfig(portConfig),
  m_inValue(0xffff),
  m_outValue(0xffff),
  m_errCnt(0)
{
}

CCat9555::~CCat9555(void)
{
}

void CCat9555::begin(uint16_t outValue)
{
  // Set the initial output value.
  m_outValue = outValue;

  // Setup the port configuration.
  uint8_t data[] = { ECREG_PORT_CONFIG, lsb_of(m_portConfig), msb_of(m_portConfig) };
  twi_initiate_write(m_i2cAddr, data, sizeof(data));
  twi_wait_until_master_ready();

  // Sync with the hardware
  syncOut();
  syncIn();
}

void CCat9555::syncOut(void)
{
  uint8_t data[] = { ECREG_OUTPUT, lsb_of(m_outValue), msb_of(m_outValue) };
  twi_initiate_write(m_i2cAddr, data, sizeof(data));
  twi_wait_until_master_ready();
}

void CCat9555::syncIn(void)
{
  uint8_t data_wr[] = { ECREG_INPUT };
  uint8_t data_rd[2];
  twi_initiate_transaction(m_i2cAddr, data_wr, sizeof(data_wr), data_rd, sizeof(data_rd));
  twi_wait_until_master_ready();
  set_lsb(m_inValue, data_rd[0]);
  set_msb(m_inValue, data_rd[1]);
}

