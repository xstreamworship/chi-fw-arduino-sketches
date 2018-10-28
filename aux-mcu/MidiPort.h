/////////////////////////////////////////////////////////////////////
// Simple MIDI port implementation using Serial.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __MIDIPORT_H
#define __MIDIPORT_H

#include "Arduino.h"

template<class SerialPort>
class CMidiPort
{
  private:
    uint8_t m_currentStatus;
    SerialPort& m_serial;
    bool m_running;
    void (* m_cbSetLed) (bool);
    uint8_t m_rxRunningStatus;
    uint8_t m_rxCCNum;
    enum state
    {
      E_MS_
    };
  public:
    inline CMidiPort(SerialPort& serialPort) :
      m_serial(serialPort),
      m_cbSetLed(0),
      m_currentStatus(0),
      m_rxRunningStatus(255),
      m_rxCCNum(255),
      m_running(false) { }
    inline ~CMidiPort(void) { }
    inline void begin(void (* cbSetLed) (bool))
   {
      m_serial.begin(31250);
      m_running = true;
      m_cbSetLed = cbSetLed;
    }
    inline void send(uint8_t msgType, uint8_t data1, uint8_t data2, uint8_t channel)
    {
      if (!m_running)
        return;
      if (msgType < 0xf0) {
        msgType &= 0xf0;
        if (msgType < 0x80)
          return; // Not a valid MIDI message type
        channel &= 0x0f;
        data1 &= 0x7f;
        data2 &= 0x7f;
        uint8_t msgStatus = msgType | channel;
        if (msgStatus != m_currentStatus)
        {
          // Filter running status.
          m_currentStatus = msgStatus;
          m_serial.write(msgStatus);
        }
        m_serial.write(data1);
        if ((msgType != 0xc0) && (msgType != 0xd0))
        {
          m_serial.write(data2);
        }
        if (m_cbSetLed)
          m_cbSetLed(HIGH);
      }
      else
      {
        // Handle the status byte only for system common or real time.  The rest is up to the invoker.
        m_serial.write(msgType);
      }
    }
    inline void activeSense(void) { send( 0xfe, 0, 0, 0 ); if (m_cbSetLed) m_cbSetLed(HIGH); }
    inline void ctrlCh(uint8_t ccNum, uint8_t ccVal, uint8_t channel = 0) { send( 0xb0, ccNum, ccVal, channel ); }
    inline void pitchBend(uint16_t pbVal, uint8_t channel = 0) { send( 0xe0, pbVal & 0x7f, (pbVal >> 7) & 0x7f, channel ); }
};

#endif

