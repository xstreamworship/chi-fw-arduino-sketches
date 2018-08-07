/////////////////////////////////////////////////////////////////////
// Filter that translates analogue input stream into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __FILTER_H
#define __FILTER_H

#include "Arduino.h"

class CFilter
{
  private:
    const PROGMEM char *m_filterName;
    const PROGMEM uint16_t m_threshold;
    const PROGMEM uint16_t m_origin;
    const PROGMEM uint16_t m_originMargin;
    const PROGMEM uint16_t m_min;
    const PROGMEM uint16_t m_minMargin;
    const PROGMEM uint16_t m_max;
    const PROGMEM uint16_t m_maxMargin;
    enum
    {
      E_TIMEDEBOUNCE = 30000,
    };
    enum state
    {
      E_FILT_REST,
      E_FILT_VALIDATING,
    };
    enum state m_state;
    enum region
    {
      E_FILT_AT_ORIGIN,
      E_FILT_AT_MIN,
      E_FILT_IN_LOWER_REGION,
      E_FILT_AT_MAX,
      E_FILT_IN_UPPER_REGION,
    };
    enum region m_region;
    uint16_t m_lastValue;
    unsigned long m_timeStart;
    const uint8_t m_ccNum;
    const uint8_t m_useCase;

  public:
    CFilter(const PROGMEM char *filterName, const PROGMEM uint16_t thresh,
      const PROGMEM uint16_t origin, const PROGMEM uint16_t originMargin, const PROGMEM uint16_t minv,
      const PROGMEM uint16_t minMargin, const PROGMEM uint16_t maxv, const PROGMEM uint16_t maxMargin,
      const uint8_t ccNum = 255, const uint8_t useCase = 0);
    ~CFilter(void);
    void scan(int sample);
    bool atOrigin(void) { return (m_region == E_FILT_AT_ORIGIN) && (m_state == E_FILT_REST); }
    enum region inRegion(int sample)
    {
      if (abs(sample - ((int)m_origin)) < ((int)m_originMargin))
        return E_FILT_AT_ORIGIN;
      if (sample < ((int)m_origin))
      {
        // We are below the origin.
        if (sample <= (((int)m_min) + ((int)m_minMargin)))
          return E_FILT_AT_MIN;
        return E_FILT_IN_LOWER_REGION;
      }
      if (sample >= (((int)m_max) - ((int)m_maxMargin)))
        return E_FILT_AT_MAX;
      return E_FILT_IN_UPPER_REGION;
    }
    void changed(void);
};

#endif

