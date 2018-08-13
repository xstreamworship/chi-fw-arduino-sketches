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
  m_inValue{ 0, 0, 0, 0, 0, 0, 0, 0 },
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

void CAd7997::sync(void)
{
  for (uint8_t i = 0; i < E_NUM_PORTS; i++)
  {
    sync(i);
  }
}

void CAd7997::sync(uint8_t i)
{
  uint8_t data_wr[] = { E_READ_CMD + (i << 4) };
  uint8_t data_rd[2];
  twi_initiate_transaction(m_i2cAddr, data_wr, sizeof(data_wr), data_rd, sizeof(data_rd));
  twi_wait_until_master_ready();
  if (((data_rd[0] >> 4) & 0x07) != i) {
    m_errCnt++;
    return;
  }
  set_msb(m_inValue[i], (data_rd[0] & 0x0f));
  set_lsb(m_inValue[i], data_rd[1]);
}

