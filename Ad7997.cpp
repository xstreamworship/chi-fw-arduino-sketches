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
#include "Wire.h"

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
  Wire.beginTransmission(m_i2cAddr);
  Wire.write(ECREG_ADDR);
  Wire.write(ECREG_MSB);
  Wire.write(ECREG_LSB);
  Wire.endTransmission();

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
  uint8_t invl;
  // Initiate read command
  Wire.beginTransmission(m_i2cAddr);
  Wire.write(E_READ_CMD + (i << 4));
  Wire.endTransmission();
  // Do the read
  Wire.requestFrom(m_i2cAddr, (byte) 2);
  if (Wire.available())
  {
    invl = Wire.read();
    if (((invl >> 4) & 0x07) != i) {
      m_errCnt++;
      return;
    }
    set_msb(m_inValue[i], (invl & 0x0f));
  }
  else
  {
    m_errCnt++;
  }
  if (Wire.available())
  {
    invl = Wire.read();
    set_lsb(m_inValue[i], invl);
  }
  else
  {
    m_errCnt++;
  }
}

