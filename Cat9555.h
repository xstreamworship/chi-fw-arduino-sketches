/////////////////////////////////////////////////////////////////////
// Digital I/O driver for CAT9555 16 line port.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __CAT9555_H
#define __CAT9555_H

#include "Arduino.h"

class CCat9555
{
  private:
    enum cmd_reg
    {
      ECREG_INPUT = 0,
      ECREG_OUTPUT = 2,
      ECREG_IN_INV = 4,
      ECREG_PORT_CONFIG = 6,
    };
    enum properties
    {
      E_NUM_PORTS = 2,
    };
    const uint8_t m_i2cAddr;
    const uint16_t m_portConfig;
    uint16_t m_inValue;
    uint16_t m_outValue;
    unsigned m_errCnt;

  public:
    CCat9555(const uint8_t i2cAddr, const uint16_t portConfig);
    ~CCat9555(void);
    void begin(uint16_t outValue = 0xffff);
    void syncOut(void);
    void syncIn(void);
    inline void write(uint16_t outValue) { m_outValue = outValue; }
    inline uint16_t read(void) { return (m_inValue & m_portConfig) | (m_outValue & ~ m_portConfig); }

  public:
    static inline uint8_t lsb_of(uint16_t val) { return val & 0xff; }
    static inline uint8_t msb_of(uint16_t val) { return (val >> 8) & 0xff; }
    static inline void set_lsb(uint16_t &targ, uint8_t val) { targ = (targ & 0xff00) | (val & 0x00ff); }
    static inline void set_msb(uint16_t &targ, uint8_t val) { targ = (targ & 0x00ff) | (val << 8) & 0xff00; }
};

#endif

