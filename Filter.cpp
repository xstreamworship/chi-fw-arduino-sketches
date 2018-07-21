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

CFilter::CFilter(const PROGMEM char *filterName, const PROGMEM uint16_t thresh,
      const PROGMEM uint16_t origin, const PROGMEM uint16_t originMargin, const PROGMEM uint16_t minv,
      const PROGMEM uint16_t minMargin, const PROGMEM uint16_t maxv, const PROGMEM uint16_t maxMargin) :
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
  m_lastValue(0)
{
}

CFilter::~CFilter(void)
{
}

void CFilter::scan(int sample)
{
  enum region where = inRegion(sample);
  if (where == m_region)
  {
    // No change in region since the previous scan.
    if (m_state == E_FILT_VALIDATING)
    {
      // Still validating entry to this region.
      if ((micros() - m_timeStart) >= E_TIMEDEBOUNCE)
      {
        // Complete the validation because time threshold reached.
        m_lastValue = ((uint16_t)sample);
        changed();
        m_timeStart = micros();
        m_state = E_FILT_REST;
      }
    }
    else if ((m_region == E_FILT_IN_LOWER_REGION) || (m_region == E_FILT_IN_UPPER_REGION))
    {
      // We are validated as in an active region, so check for movement.
      if ((abs(sample - ((int)m_lastValue)) >= ((int)m_threshold)) ||
         ((sample != ((int)m_lastValue)) && ((micros() - m_timeStart) >= E_TIMEDEBOUNCE)))
      {
        // Either we changed beyond a threshold, or we have changed since the last time interval.
        m_lastValue = ((uint16_t)sample);
        changed();
        m_timeStart = micros();
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
        m_timeStart = micros();
      }
      // Cancel the validation to filter (ignorte) the glitch.
      m_state = E_FILT_REST;
    }
  }
  else
  {
    // A change in region that needs to be validated.
    m_region = where;
    m_timeStart = micros();
    m_state = E_FILT_VALIDATING;
    // Note: Don't change m_S->m_lastValue in case it jumps back.
  }
}

void CFilter::changed(void)
{
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
      Serial.println(m_lastValue);
      break;
    case E_FILT_AT_MAX:
      Serial.println(F(" max"));
      break;
    case E_FILT_IN_UPPER_REGION:
      Serial.print(F(" upper: "));
      Serial.println(m_lastValue);
      break;
  } 
}

