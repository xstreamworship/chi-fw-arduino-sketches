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
  m_readAt(0),
  m_i2cAddr(i2cAddr),
  m_portConfig(portConfig),
  m_errCnt(0)
{
  m_in.port.val = 0xffff;
  m_out.port.val = 0xffff;
}

CCat9555::~CCat9555(void)
{
}

void CCat9555::begin(uint16_t outValue)
{
  // Setup the port configuration.
  m_out.cmd = ECREG_PORT_CONFIG;
  m_out.port.val = m_portConfig;
  twi_initiate_write(m_i2cAddr, (uint8_t *)(&m_out), sizeof(m_out));
  twi_wait_until_master_ready();

  // Set the initial output value.
  write(outValue);

  // Sync with the hardware
  syncOut();
  syncIn();
}

void CCat9555::syncOut(void)
{
  m_out.cmd = ECREG_OUTPUT;
  twi_initiate_write(m_i2cAddr, (uint8_t *)(&m_out), sizeof(m_out));
  twi_wait_until_master_ready();
}

uint32_t CCat9555::syncIn(void)
{
  m_in.cmd = ECREG_INPUT;
  twi_initiate_transaction(m_i2cAddr, &m_in.cmd, sizeof(m_in.cmd), m_in.port.raw_val, sizeof(m_in.port.raw_val), &m_readAt);
  twi_wait_until_master_ready();
  return m_readAt;
}

#if 0
STWI_TRANSACTION(yaddaOut, sizeof(SCat9555Buf)) (33, sizeof(SCat9555Buf::cmd), sizeof(SCat9555Buf::port));

void blah(void)
{
  TWI_INITIATE_TRANSACTION(yaddaOut);
}
#endif

