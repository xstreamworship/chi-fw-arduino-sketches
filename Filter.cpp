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

CFilterS::CFilterS(void) :
  m_state(E_FILT_REST),
  m_region(E_FILT_AT_ORIGIN),
  m_timeStart(0),
  m_lastValue(0)
{
}

CFilterS::CFilterS(uint16_t startValue) :
  m_state(E_FILT_REST),
  m_region(E_FILT_AT_ORIGIN),
  m_timeStart(0),
  m_lastValue(startValue)
{
}

CFilterS::~CFilterS(void)
{
}

CFilter::CFilter(const PROGMEM char *filterName, const PROGMEM unsigned long timeDebounce, const PROGMEM uint16_t thresh,
      const PROGMEM uint16_t origin, const PROGMEM uint16_t originMargin, const PROGMEM uint16_t minv,
      const PROGMEM uint16_t minMargin, const PROGMEM uint16_t maxv, const PROGMEM uint16_t maxMargin) :
  m_filterName(filterName),
  m_timeDebounce(timeDebounce),
  m_threshold(thresh),
  m_origin(origin),
  m_originMargin(originMargin),
  m_min(minv),
  m_minMargin(minMargin),
  m_max(maxv),
  m_maxMargin(maxMargin),
  m_S(new CFilterS(origin))
{
}

CFilter::CFilter(const PROGMEM char *filterName, const PROGMEM unsigned long timeDebounce, const PROGMEM uint16_t thresh,
      const PROGMEM uint16_t origin, const PROGMEM uint16_t originMargin, const PROGMEM uint16_t minv,
      const PROGMEM uint16_t minMargin, const PROGMEM uint16_t maxv, const PROGMEM uint16_t maxMargin,
      const PROGMEM CFilterS * css) :
  m_filterName(filterName),
  m_timeDebounce(timeDebounce),
  m_threshold(thresh),
  m_origin(origin),
  m_originMargin(originMargin),
  m_min(minv),
  m_minMargin(minMargin),
  m_max(maxv),
  m_maxMargin(maxMargin),
  m_S(css)
{
}

CFilter::~CFilter(void)
{
}

void CFilter::scan(int sample)
{
  enum CFilterS::region where = inRegion(sample);
  if (where == m_S->m_region)
  {
    // No change in region since the previous scan.
    if (m_S->m_state == CFilterS::E_FILT_VALIDATING)
    {
      // Still validating entry to this region.
      if ((micros() - m_S->m_timeStart) >= m_timeDebounce)
      {
        // Complete the validation because time threshold reached.
        m_S->m_lastValue = ((uint16_t)sample);
        changed();
        m_S->m_timeStart = micros();
        m_S->m_state = CFilterS::E_FILT_REST;
      }
    }
    else if ((m_S->m_region == CFilterS::E_FILT_IN_LOWER_REGION) || (m_S->m_region == CFilterS::E_FILT_IN_UPPER_REGION))
    {
      // We are validated as in an active region, so check for movement.
      if ((abs(sample - ((int)m_S->m_lastValue)) >= ((int)m_threshold)) ||
         ((sample != ((int)m_S->m_lastValue)) && ((micros() - m_S->m_timeStart) >= m_timeDebounce)))
      {
        // Either we changed beyond a threshold, or we have changed since the last time interval.
        m_S->m_lastValue = ((uint16_t)sample);
        changed();
        m_S->m_timeStart = micros();
      }
    }
  }
  else if (m_S->m_state == CFilterS::E_FILT_VALIDATING)
  {
    // A change in region occurred before completing validation.
    if (where == inRegion((int)m_S->m_lastValue))
    {
      // Appears to have reverted back to where it came from.
      m_S->m_region = where;
      if ((m_S->m_region == CFilterS::E_FILT_IN_LOWER_REGION) || (m_S->m_region == CFilterS::E_FILT_IN_UPPER_REGION))
      {
        // In an active region for movement so sync up value.
        m_S->m_lastValue = ((uint16_t)sample);
        changed();
        m_S->m_timeStart = micros();
      }
      // Cancel the validation to filter (ignorte) the glitch.
      m_S->m_state = CFilterS::E_FILT_REST;
    }
  }
  else
  {
    // A change in region that needs to be validated.
    m_S->m_region = where;
    m_S->m_timeStart = micros();
    m_S->m_state = CFilterS::E_FILT_VALIDATING;
    // Note: Don't change m_S->m_lastValue in case it jumps back.
  }
}

void CFilter::changed(void)
{
  char mbuffer[12];
  Serial.print(F("Filter: "));
  strncpy_P(mbuffer, m_filterName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  switch (m_S->m_region)
  {
    case CFilterS::E_FILT_AT_ORIGIN:
      Serial.println(F(" origin"));
      break;
    case CFilterS::E_FILT_AT_MIN:
      Serial.println(F(" min"));
      break;
    case CFilterS::E_FILT_IN_LOWER_REGION:
      Serial.print(F(" lower: "));
      Serial.println(m_S->m_lastValue);
      break;
    case CFilterS::E_FILT_AT_MAX:
      Serial.println(F(" max"));
      break;
    case CFilterS::E_FILT_IN_UPPER_REGION:
      Serial.print(F(" upper: "));
      Serial.println(m_S->m_lastValue);
      break;
  } 
}

