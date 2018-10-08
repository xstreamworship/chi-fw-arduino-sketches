/////////////////////////////////////////////////////////////////////
// Translate organ drawbar scans into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __DRAWBAR_H
#define __DRAWBAR_H

#include "Arduino.h"

class CDrawbar
{
  private:
    uint8_t m_icBus;
    uint8_t m_icBusPrev;
    uint8_t m_ocBus;
    uint8_t m_ocBusPrev;
    uint8_t m_drawbarVal;
    const char * m_drawbarName;
    enum
    {
      E_NO_CONTACT = 0xff,
      E_NO_VAL = 0xff,
      E_MAX_BUS = 8,
      E_NUM_BUSES = E_MAX_BUS + 1,
    };
    void changed(uint8_t ccNum, uint8_t uCase);

  public:
    CDrawbar(const char *drawbarName);
    ~CDrawbar(void);
    void scan(uint32_t sTime, uint8_t busInd, bool icBus, bool ocBus, uint8_t ccNum = 255, uint8_t uCase = 0);
};

#endif

