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
#include "Wire.h"

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
  Wire.beginTransmission(m_i2cAddr);
  Wire.write(ECREG_PORT_CONFIG);
  Wire.write(lsb_of(m_portConfig));
  Wire.write(msb_of(m_portConfig));
  Wire.endTransmission();

  // Sync with the hardware
  syncOut();
  syncIn();
}

void CCat9555::syncOut(void)
{
  Wire.beginTransmission(m_i2cAddr);
  Wire.write(ECREG_OUTPUT);
  Wire.write(lsb_of(m_outValue));
  Wire.write(msb_of(m_outValue));
  Wire.endTransmission();
}

void CCat9555::syncIn(void)
{
  Wire.beginTransmission(m_i2cAddr);
  Wire.write(ECREG_INPUT);
  Wire.endTransmission();
  Wire.requestFrom(m_i2cAddr, (byte) E_NUM_PORTS);
  if (Wire.available())
  {
    set_lsb(m_inValue, Wire.read());
  }
  else
  {
    m_errCnt++;
  }
  if (Wire.available())
  {
    set_msb(m_inValue, Wire.read());
  }
  else
  {
    m_errCnt++;
  }
}
