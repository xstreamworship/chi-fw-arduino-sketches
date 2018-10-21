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

#include "MidiKeySwitch.h"

template<class SerialPort>
class CMidiPort
{
  private:
    uint8_t m_currentStatus;
    SerialPort& m_serial;
    bool m_running;
    bool m_runningStatus;
    void (* m_cbSetLed) (bool);
    void (* m_handleCtrlCh) (uint8_t, uint8_t, uint8_t);
    void (* m_handleProgCh) (uint8_t, uint8_t);
    uint8_t m_rxRunningStatus;
    uint8_t m_rxCCNum;
    enum state
    {
      E_MS_
    };
    bool parseMidi(uint8_t rxByte)
    {
      uint8_t rxStatus = rxByte & 0xf0;
      uint8_t rxChannel = rxByte & 0x0f;
      if (rxByte < 0x80) {
        // A data byte
        if (m_rxRunningStatus == 0xb0) {
          if (m_rxCCNum != 255) {
            // Invoke the control change callback.
            if (m_handleCtrlCh)
              m_handleCtrlCh(m_rxCCNum, rxByte, CMidiKeySwitch::getMidiCh());
            m_rxCCNum = 255;
          } else {
            // Store the received CC number
            m_rxCCNum = rxByte;
          }
        } else if (m_rxRunningStatus == 0xc0) {
          // Invoke the program change callback.
          if (m_handleProgCh)
            m_handleProgCh(rxByte, CMidiKeySwitch::getMidiCh());
          m_rxRunningStatus = 255;
        } else {
          // Ignore the data byte.
        }
      } else if (rxByte >= 0xf8) {
        // A real time message - ignore it.
      } else if ((rxStatus == 0xb0) && (rxChannel == CMidiKeySwitch::getMidiCh())) {
        // A CC status byte
        m_rxRunningStatus = 0xb0;
        m_rxCCNum = 255;
      } else if ((rxStatus == 0xc0) && (rxChannel == CMidiKeySwitch::getMidiCh())) {
        // A PC status byte
        m_rxRunningStatus = 0xc0;
      } else {
        // Unrecognized / unsupported messages.
      }
    }
  public:
    inline CMidiPort(SerialPort& serialPort) :
      m_serial(serialPort),
      m_cbSetLed(0),
      m_handleCtrlCh(0),
      m_handleProgCh(0),
      m_currentStatus(0),
      m_rxRunningStatus(255),
      m_rxCCNum(255),
      m_runningStatus(false),
      m_running(false) { }
    inline ~CMidiPort(void) { }
    inline void begin(void (* cbSetLed) (bool),
      void (* cbCtrlCh) (uint8_t, uint8_t, uint8_t) = 0,
      void (* cbProgCh) (uint8_t, uint8_t) = 0,
      uint32_t rate = 31250)
    {
      m_serial.begin(rate);
      m_running = true;
      m_cbSetLed = cbSetLed;
      m_handleCtrlCh = cbCtrlCh;
      m_handleProgCh = cbProgCh;
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
        if (!m_runningStatus || (msgStatus != m_currentStatus))
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
    inline void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) { send( 0x90, note, velocity, channel ); }
    inline void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0)
    {
      if (velocity != 0)
        send( 0x80, note, velocity, channel );
      else
        noteOn( note, 0, channel );
    }
    inline void activeSense(void) { send( 0xfe, 0, 0, 0 ); if (m_cbSetLed) m_cbSetLed(HIGH); }
    inline void ctrlCh(uint8_t ccNum, uint8_t ccVal, uint8_t channel = 0) { send( 0xb0, ccNum, ccVal, channel ); }
    inline void progCh(uint8_t pcNum, uint8_t channel = 0) { send( 0xc0, pcNum, 0, channel ); }
    inline void pitchBend(uint16_t pbVal, uint8_t channel = 0) { send( 0xe0, pbVal & 0x7f, (pbVal >> 7) & 0x7f, channel ); }
    inline bool receiveScan(unsigned int polls = 10)
    {
      while ((m_serial.available() > 0) && --polls) {
        if (!parseMidi(m_serial.read()))
          return false;
      }
      return true;
    }
};

#endif

