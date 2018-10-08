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

CCat9555::CCat9555(const uint8_t i2cAddr, const uint8_t portNum, const uint8_t portConfig) :
  m_i2cAddr(i2cAddr),
  m_portNum(portNum),
  m_portConfig(portConfig)
{
  m_in.port = 0xff;
  m_out.port = 0xff;
}

CCat9555::~CCat9555(void)
{
}

void CCat9555::begin(uint8_t outValue)
{
  // Setup the port configuration.
  m_out.cmd = ECREG_PORT_CONFIG + m_portNum;
  m_out.port = m_portConfig;
  twi_initiate_write(m_i2cAddr, (uint8_t *)(&m_out), sizeof(m_out));
  twi_wait_until_master_ready();

  // Set the initial output value.
  write(outValue);

  // Sync with the hardware
  syncOut();
  syncIn();
}

void CCat9555::startOut(void)
{
  m_out.cmd = ECREG_OUTPUT + m_portNum;
  twi_initiate_write(m_i2cAddr, (uint8_t *)(&m_out), sizeof(m_out));
}

void CCat9555::startIn(void)
{
  m_in.cmd = ECREG_INPUT + m_portNum;
  twi_initiate_transaction(m_i2cAddr, &m_in.cmd, sizeof(m_in.cmd), &m_in.port, sizeof(m_in.port));
}

void CCat9555::syncOut(void)
{
  startOut();
  twi_wait_until_master_ready();
}

void CCat9555::syncIn(void)
{
  startIn();
  twi_wait_until_master_ready();
}

