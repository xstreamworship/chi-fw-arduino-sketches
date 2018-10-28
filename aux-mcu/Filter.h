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
    const char *m_filterName;
    const uint8_t m_filter;
    const uint16_t m_threshold;
    const uint16_t m_origin;
    const uint16_t m_originMargin;
    const uint16_t m_min;
    const uint16_t m_minMargin;
    const uint16_t m_max;
    const uint16_t m_maxMargin;
    enum
    {
      E_TIMEVALIDATE = 20000,
      E_TIMEUPDATE = 10000,
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
    uint16_t m_filteredValue;
    uint16_t m_lastValue;
    unsigned long m_timeStart;
    const uint8_t m_ccNum;
    const uint8_t m_useCase;

  public:
    enum filter
    {
      E_FILT_NO_FILTER = 0,
      E_FILT_0P125,
      E_FILT_0P625,
    };
    CFilter(const char *filterName, const enum filter filt, const uint16_t thresh,
      const uint16_t origin, const uint16_t originMargin, const uint16_t minv,
      const uint16_t minMargin, const uint16_t maxv, const uint16_t maxMargin,
      const uint8_t ccNum = 255, const uint8_t useCase = 0);
    ~CFilter(void);
    void begin(uint16_t sample) { m_lastValue = sample; m_filteredValue = sample; }
    void scan(uint16_t sample, uint32_t sTime);
    uint16_t applyFilter( uint16_t sample )
    {
      switch (m_filter)
      {
        case E_FILT_NO_FILTER:
        default: // No filter - pass through
          m_filteredValue = sample;
          break;
        case E_FILT_0P125:
          // Weight 1/8ths to sample, 7/8ths to m_filteredValue.
          m_filteredValue = (sample + (m_filteredValue << 3) - m_filteredValue + (1 << 2)) >> 3;
          break;
        case E_FILT_0P625:
          // Weight 5/8ths to sample, 3/8ths to m_filteredValue.
          m_filteredValue = ((sample << 2) + sample + (m_filteredValue << 1) + m_filteredValue + (1 << 2)) >> 3;
          break;
      }
      return m_filteredValue;
    }
    bool atOrigin(void) { return (m_region == E_FILT_AT_ORIGIN) && (m_state == E_FILT_REST); }
    enum region inRegion(uint16_t sample)
    {
      if (abs(((int)sample) - ((int)m_origin)) < ((int)m_originMargin))
        return E_FILT_AT_ORIGIN;
      if (sample < m_origin)
      {
        // We are below the origin.
        if (sample <= (m_min + m_minMargin))
          return E_FILT_AT_MIN;
        return E_FILT_IN_LOWER_REGION;
      }
      if (sample >= (m_max - m_maxMargin))
        return E_FILT_AT_MAX;
      return E_FILT_IN_UPPER_REGION;
    }
    void changed(void);
};

#endif

