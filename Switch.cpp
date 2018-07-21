/////////////////////////////////////////////////////////////////////
// Translate switch input samples into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Arduino.h"
#include "Switch.h"

CSwitch::CSwitch(const PROGMEM char *switchName) :
  m_switchName(switchName),
  m_state(E_SW_OFF),
  m_timeStart(0),
  m_ledState(false)
{
}

CSwitch::~CSwitch(void)
{
}

void CSwitch::scan(bool state)
{
  if (state)
  {
    // On or just turned on.
    switch (m_state)
    {
      case E_SW_ON:
        // Do nothing.
        break;
      case E_SW_VALIDATING_OFF:
        // False off - cancel it.
        m_state = E_SW_ON;
        break;
      case E_SW_OFF:
      default:
        // Transition from off to on.
        m_timeStart = micros();
        m_state = E_SW_VALIDATING_ON;
        // fall through
      case E_SW_VALIDATING_ON:
        // Complete the off
        if ((micros() - m_timeStart) >= E_TIMEDEBOUNCE) {
          switchOn();
          m_state = E_SW_ON;
        }
        break;
    }
  }
  else
  {
    // Off or just turned off.
    switch (m_state)
    {
      case E_SW_OFF:
        // Do nothing.
        break;
      case E_SW_VALIDATING_ON:
        // False on - cancel it.
        m_state = E_SW_OFF;
        break;
      case E_SW_ON:
      default:
        // Transition from on to off.
        m_timeStart = micros();
        m_state = E_SW_VALIDATING_OFF;
        // fall through
      case E_SW_VALIDATING_OFF:
        // Complete the off
        if ((micros() - m_timeStart) >= E_TIMEDEBOUNCE) {
          switchOff();
          m_state = E_SW_OFF;
        }
        break;
    }
  }
}

void CSwitch::switchOn(void)
{
  char mbuffer[12];
  Serial.print(F("Switch: "));
  strncpy_P(mbuffer, m_switchName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.println(F(" on"));
}

void CSwitch::switchOff(void)
{
  char mbuffer[12];
  Serial.print(F("Switch: "));
  strncpy_P(mbuffer, m_switchName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.println(F(" off"));
}

