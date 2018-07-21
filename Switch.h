/////////////////////////////////////////////////////////////////////
// Translate switch input samples into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __SWITCH_H
#define __SWITCH_H

#include "Arduino.h"

class CSwitch
{
  private:
    enum state
    {
      E_SW_OFF,
      E_SW_VALIDATING_ON,
      E_SW_ON,
      E_SW_VALIDATING_OFF,
    };
    unsigned long m_timeStart;
    enum state m_state;
    bool m_ledState;
    char * const PROGMEM m_switchName;
    enum
    {
      E_TIMEDEBOUNCE = 30000,
    };

  public:
    CSwitch(const PROGMEM char *switchName);
    ~CSwitch(void);
    void scan(bool state);
    void switchOn(void);
    void switchOff(void);
    bool ledState(void) { return m_ledState; }
    void setLedState(bool state) { m_ledState = state; }
    void ledOn(void) { setLedState(true); }
    void ledOff(void) { setLedState(false); }
};

#endif
