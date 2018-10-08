/////////////////////////////////////////////////////////////////////
// Analogue I/O driver for AD7997 8 channel ADC.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Arduino.h"
#include "Ad7997.h"
#include "twi_if.h"

CAd7997::CAd7997(const uint8_t i2cAddr) :
  m_i2cAddr(i2cAddr),
  m_inBytes{ 0 },
  m_errCnt(0)
{
}

CAd7997::~CAd7997(void)
{
}

void CAd7997::begin(void)
{
  // Setup the port configuration.
  uint8_t data[] = { ECREG_ADDR, ECREG_MSB, ECREG_LSB };
  twi_initiate_write(m_i2cAddr, data, sizeof(data));
  twi_wait_until_master_ready();

  // Sync with the hardware
  sync();
}

void CAd7997::start(void)
{
  uint8_t data_wr[] = { E_READ_ALL_CMD };
  twi_initiate_transaction(m_i2cAddr, data_wr, sizeof(data_wr), m_inBytes, sizeof(m_inBytes));
}

void CAd7997::sync(void)
{
  start();
  twi_wait_until_master_ready();
}

void CAd7997::start(uint8_t i)
{
  uint8_t data_wr[] = { (uint8_t)(E_READ_CMD + (i << 4)) };
  twi_initiate_transaction(m_i2cAddr, data_wr, sizeof(data_wr), m_inBytes + (2 * i), sizeof(m_inBytes[0]) * 2);
}

void CAd7997::sync(uint8_t i)
{
  start(i);
  twi_wait_until_master_ready();
}

