/////////////////////////////////////////////////////////////////////
// Translate organ drawbar scans into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Arduino.h"

#include "Drawbar.h"

extern bool debug_mode;
void drawbarChanged(uint8_t val, uint8_t ccNum, uint8_t uCase);

CDrawbar::CDrawbar(const char *drawbarName) :
  m_drawbarName(drawbarName),
  m_icBus(E_NO_CONTACT),
  m_icBusPrev(E_NO_CONTACT),
  m_ocBus(E_NO_CONTACT),
  m_ocBusPrev(E_NO_CONTACT),
  m_drawbarVal(E_NO_VAL)
{
}

CDrawbar::~CDrawbar(void)
{
}

void CDrawbar::scan(uint32_t sTime, uint8_t busInd, bool icBus, bool ocBus, uint8_t ccNum, uint8_t uCase)
{
  if (busInd < E_NUM_BUSES) {
    if (icBus) {
      // Inner contact in touch with busbar
      m_icBus = busInd;
    }
    if (ocBus) {
      // Outer contact in touch with busbar
      m_ocBus = busInd;
    }
  }

  if (busInd >= E_MAX_BUS) {
    // At the end of the cycle.
    if ((m_icBus != m_icBusPrev) || (m_ocBus != m_ocBusPrev)) {
      // There has been a change.
      uint8_t drawbarVal = E_NO_VAL;
      if (m_icBus > E_MAX_BUS) {
        if (m_ocBus > E_MAX_BUS) {
          // Error glitch - no contacts, but this could be a scanning alias if drawbar is moving.
          // For this case, keep the last scanned value until it clears up.
          drawbarVal = m_drawbarVal;;
        } else {
          // We are at #.0
          drawbarVal = m_ocBus * 4;
        }
      } else {
        if (m_ocBus == m_icBus) {
          // We are at #.25
          drawbarVal = (m_icBus * 4) + 1;
        } else if (m_ocBus > E_MAX_BUS) {
          // We are at #.5
          drawbarVal = (m_icBus * 4) + 2;
        } else if (m_ocBus == (m_icBus + 1)) {
          // We are at #.75
          drawbarVal = (m_icBus * 4) + 3;
        } else {
          // Error - ignore the reading.
        }
      }

      // Validate the change.
      if ((drawbarVal != m_drawbarVal) && (drawbarVal != E_NO_VAL)) {
        // Sync the change.
        m_drawbarVal = drawbarVal;

        // Process the change.
        changed(ccNum, uCase);
      }
      m_icBusPrev = m_icBus;
      m_ocBusPrev = m_ocBus;
    }
    m_icBus = E_NO_CONTACT;
    m_ocBus = E_NO_CONTACT;
  }
}

void CDrawbar::changed(uint8_t ccNum, uint8_t uCase)
{
  if (!debug_mode) {
    // Using serial for MIDI.
    drawbarChanged(m_drawbarVal, ccNum, uCase);
  } else {
    char mbuffer[16];
    Serial.print(F("Drawbar: "));
    strncpy_P(mbuffer, m_drawbarName, sizeof(mbuffer) - 1);
    Serial.print(mbuffer);
    Serial.print(F(" [ "));
    Serial.print(m_drawbarVal / 4);
    switch (m_drawbarVal % 4)
    {
      case 0:
        Serial.print(F(".0"));
        break;
      case 1:
        Serial.print(F(".25"));
        break;
      case 2:
        Serial.print(F(".5"));
        break;
      case 3:
        Serial.print(F(".75"));
        break;
    }
    Serial.println(F(" ]"));
  }
}

