/////////////////////////////////////////////////////////////////////
// Filter that translates analogue input stream into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Arduino.h"

#include "Filter.h"

extern bool debug_mode;
void analogChanged(bool state, uint16_t val, uint8_t ccNum, uint8_t uCase);

CFilter::CFilter(const char *filterName, const uint16_t thresh,
      const uint16_t origin, const uint16_t originMargin, const uint16_t minv,
      const uint16_t minMargin, const uint16_t maxv, const uint16_t maxMargin,
      const uint8_t ccNum, const uint8_t useCase) :
  m_filterName(filterName),
  m_threshold(thresh),
  m_origin(origin),
  m_originMargin(originMargin),
  m_min(minv),
  m_minMargin(minMargin),
  m_max(maxv),
  m_maxMargin(maxMargin),
  m_state(E_FILT_REST),
  m_region(E_FILT_AT_ORIGIN),
  m_timeStart(0),
  m_lastValue(0),
  m_ccNum(ccNum),
  m_useCase(useCase)
{
}

CFilter::~CFilter(void)
{
}

void CFilter::scan(int sample, uint32_t sTime)
{
  enum region where = inRegion(sample);
  if (where == m_region)
  {
    // No change in region since the previous scan.
    if (m_state == E_FILT_VALIDATING)
    {
      // Still validating entry to this region.
      if ((sTime - m_timeStart) >= E_TIMEDEBOUNCE)
      {
        // Complete the validation because time threshold reached.
        m_lastValue = ((uint16_t)sample);
        changed();
        m_timeStart = sTime;
        m_state = E_FILT_REST;
      }
    }
    else if ((m_region == E_FILT_IN_LOWER_REGION) || (m_region == E_FILT_IN_UPPER_REGION))
    {
      // We are validated as in an active region, so check for movement.
      if ((abs(sample - ((int)m_lastValue)) >= ((int)m_threshold)) )// ||
         //((sample != ((int)m_lastValue)) && ((sTime - m_timeStart) >= E_TIMEDEBOUNCE)))
      {
        // Either we changed beyond a threshold, or we have changed since the last time interval.
        m_lastValue = ((uint16_t)sample);
        changed();
        m_timeStart = sTime;
      }
    }
  }
  else if (m_state == E_FILT_VALIDATING)
  {
    // A change in region occurred before completing validation.
    if (where == inRegion((int)m_lastValue))
    {
      // Appears to have reverted back to where it came from.
      m_region = where;
      if ((m_region == E_FILT_IN_LOWER_REGION) || (m_region == E_FILT_IN_UPPER_REGION))
      {
        // In an active region for movement so sync up value.
        m_lastValue = ((uint16_t)sample);
        changed();
        m_timeStart = sTime;
      }
      // Cancel the validation to filter (ignore) the glitch.
      m_state = E_FILT_REST;
    }
  }
  else
  {
    // A change in region that needs to be validated.
    m_region = where;
    m_timeStart = sTime;
    m_state = E_FILT_VALIDATING;
    // Note: Don't change m_S->m_lastValue in case it jumps back.
  }
}

void CFilter::changed(void)
{
  if (!debug_mode) {
    // Using serial for MIDI.
    bool state = false;
    uint16_t val = 0;
    switch (m_region)
    {
      case E_FILT_AT_ORIGIN:
        break;
      case E_FILT_AT_MIN:
        val = m_origin - m_originMargin - m_min - m_minMargin;
        break;
      case E_FILT_IN_LOWER_REGION:
        val = m_origin - m_originMargin - m_lastValue;
        break;
      case E_FILT_AT_MAX:
        val = m_max - m_maxMargin - m_origin - m_originMargin;
        state = true;
        break;
      case E_FILT_IN_UPPER_REGION:
        state = true;
        val = m_lastValue - m_origin - m_originMargin;
        break;
    }
    analogChanged(state, val, m_ccNum, m_useCase);
  } else {
    char mbuffer[12];
    Serial.print(F("Filter: "));
    strncpy_P(mbuffer, m_filterName, sizeof(mbuffer) - 1);
    Serial.print(mbuffer);
    switch (m_region)
    {
      case E_FILT_AT_ORIGIN:
        Serial.println(F(" origin"));
        break;
      case E_FILT_AT_MIN:
        Serial.println(F(" min"));
        break;
      case E_FILT_IN_LOWER_REGION:
        Serial.print(F(" lower: "));
        Serial.println(m_origin - m_originMargin - m_lastValue);
        break;
      case E_FILT_AT_MAX:
        //Serial.println(m_max - m_maxMargin - m_origin - m_originMargin);
        Serial.println(F(" max"));
        break;
      case E_FILT_IN_UPPER_REGION:
        Serial.print(F(" upper: "));
        Serial.println(m_lastValue - m_origin - m_originMargin);
        break;
    }
  }
}

