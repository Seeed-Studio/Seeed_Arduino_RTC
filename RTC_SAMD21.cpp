/*
  RTC library for Arduino Zero.
  Copyright (c) 2015 Arduino LLC. All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifdef __SAMD21__
#include "RTC_SAMD21.h"

rtcCallBack RTC_callBack = NULL;

bool RTC_SAMD21::begin()
{
    uint16_t tmp_reg = 0;

    PM->APBAMASK.reg |= PM_APBAMASK_RTC; // turn on digital interface clock
    config32kOSC();

    // If the RTC is in clock mode and the reset was
    // not due to POR or BOD, preserve the clock time
    // POR causes a reset anyway, BOD behaviour is?
    bool validTime = false;
    RTC_MODE2_CLOCK_Type oldTime;

    if ((PM->RCAUSE.reg & (PM_RCAUSE_SYST | PM_RCAUSE_WDT | PM_RCAUSE_EXT)))
    {
        if (RTC->MODE2.CTRL.reg & RTC_MODE2_CTRL_MODE_CLOCK)
        {
            validTime = true;
            oldTime.reg = RTC->MODE2.CLOCK.reg;
        }
    }

    // Setup clock GCLK2 with OSC32K divided by 32
    configureClock();

    RTCdisable();

    RTCreset();

    tmp_reg |= RTC_MODE2_CTRL_MODE_CLOCK;        // set clock operating mode
    tmp_reg |= RTC_MODE2_CTRL_PRESCALER_DIV1024; // set prescaler to 1024 for MODE2
    tmp_reg &= ~RTC_MODE2_CTRL_MATCHCLR;         // disable clear on match

    //According to the datasheet RTC_MODE2_CTRL_CLKREP = 0 for 24h
    tmp_reg &= ~RTC_MODE2_CTRL_CLKREP; // 24h time representation

    RTC->MODE2.READREQ.reg &= ~RTC_READREQ_RCONT; // disable continuously mode

    RTC->MODE2.CTRL.reg = tmp_reg;
    while (RTCisSyncing())
        ;

    NVIC_EnableIRQ(RTC_IRQn); // enable RTC interrupt
    NVIC_SetPriority(RTC_IRQn, 0x00);

    RTC->MODE2.INTENSET.reg |= RTC_MODE2_INTENSET_ALARM0; // enable alarm interrupt
    RTC->MODE2.Mode2Alarm[0].MASK.bit.SEL = MATCH_OFF;    // default alarm match is off (disabled)

    while (RTCisSyncing())
        ;

    RTCenable();
    RTCresetRemove();

    // If desired and valid, restore the time value, else use first valid time value
    if ((validTime) && (oldTime.reg != 0L))
    {
        RTC->MODE2.CLOCK.reg = oldTime.reg;
    }
    else
    {
        RTC->MODE2.CLOCK.reg = RTC_MODE2_CLOCK_YEAR(DEFAULT_YEAR - 2000) | RTC_MODE2_CLOCK_MONTH(DEFAULT_MONTH) | RTC_MODE2_CLOCK_DAY(DEFAULT_DAY) | RTC_MODE2_CLOCK_HOUR(DEFAULT_HOUR) | RTC_MODE2_CLOCK_MINUTE(DEFAULT_MINUTE) | RTC_MODE2_CLOCK_SECOND(DEFAULT_SECOND);
    }

    while (RTCisSyncing())
        ;

    return true;
}

void RTC_SAMD21::adjust(const DateTime &dt)
{
    RTC_MODE2_CLOCK_Type newTime;

    newTime.bit.SECOND = dt.second();
    newTime.bit.MINUTE = dt.minute();
    newTime.bit.HOUR = dt.hour();
    newTime.bit.DAY = dt.day();
    newTime.bit.MONTH = dt.month();
    newTime.bit.YEAR = dt.year() - 2000;

    RTC->MODE2.CLOCK.reg = newTime.reg;
    while (RTCisSyncing())
        ;
}

DateTime RTC_SAMD21::now()
{
    RTCreadRequest();
    uint8_t ss = RTC->MODE2.CLOCK.bit.SECOND;
    uint8_t mm = RTC->MODE2.CLOCK.bit.MINUTE;
    uint8_t hh = RTC->MODE2.CLOCK.bit.HOUR;
    uint8_t d = RTC->MODE2.CLOCK.bit.DAY;
    uint8_t m = RTC->MODE2.CLOCK.bit.MONTH;
    uint16_t y = RTC->MODE2.CLOCK.bit.YEAR;

    return DateTime(y, m, d, hh, mm, ss);
}

DateTime RTC_SAMD21::alarm()
{

    uint8_t ss = RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND;
    uint8_t mm = RTC->MODE2.Mode2Alarm[0].ALARM.bit.MINUTE;
    uint8_t hh = RTC->MODE2.Mode2Alarm[0].ALARM.bit.HOUR;
    uint8_t d = RTC->MODE2.Mode2Alarm[0].ALARM.bit.DAY;
    uint8_t m = RTC->MODE2.Mode2Alarm[0].ALARM.bit.MONTH;
    uint16_t y = RTC->MODE2.Mode2Alarm[0].ALARM.bit.YEAR;

    return DateTime(y, m, d, hh, mm, ss);
}

void RTC_SAMD21::setAlarm(const DateTime &dt)
{
    RTC_MODE2_CLOCK_Type alarmTime;

    alarmTime.bit.SECOND = dt.second();
    alarmTime.bit.MINUTE = dt.minute();
    alarmTime.bit.HOUR = dt.hour();
    alarmTime.bit.DAY = dt.day();
    alarmTime.bit.MONTH = dt.month();
    alarmTime.bit.YEAR = dt.year() - 2000;

    RTC->MODE2.Mode2Alarm[0].ALARM.reg = alarmTime.reg;
}

void RTC_SAMD21::enableAlarm(Alarm_Match match)
{
    RTC->MODE2.Mode2Alarm[0].MASK.bit.SEL = match;
    while (RTCisSyncing())
        ;
}

void RTC_SAMD21::disableAlarm()
{
    RTC->MODE2.Mode2Alarm[0].MASK.bit.SEL = 0x00;
    while (RTCisSyncing())
        ;
}

void RTC_SAMD21::attachInterrupt(rtcCallBack callback)
{
    RTC_callBack = callback;
}

void RTC_SAMD21::detachInterrupt()
{
    RTC_callBack = NULL;
}

void RTC_SAMD21::standbyMode()
{
    // Entering standby mode when connected
    // via the native USB port causes issues.
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
}

/* Attach peripheral clock to 32k oscillator */
void RTC_SAMD21::configureClock()
{
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(4);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY)
        ;
#ifdef CRYSTALLESS
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_DIVSEL);
#else
    GCLK->GENCTRL.reg = (GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_XOSC32K | GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_DIVSEL);
#endif
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY)
        ;
    GCLK->CLKCTRL.reg = (uint32_t)((GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2 | (RTC_GCLK_ID << GCLK_CLKCTRL_ID_Pos)));
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;
}

/*
 * Private Utility Functions
 */

/* Configure the 32768Hz Oscillator */
void RTC_SAMD21::config32kOSC()
{
#ifndef CRYSTALLESS
    SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_ONDEMAND |
                           SYSCTRL_XOSC32K_RUNSTDBY |
                           SYSCTRL_XOSC32K_EN32K |
                           SYSCTRL_XOSC32K_XTALEN |
                           SYSCTRL_XOSC32K_STARTUP(6) |
                           SYSCTRL_XOSC32K_ENABLE;
#endif
}

/* Synchronise the CLOCK register for reading*/
inline void RTC_SAMD21::RTCreadRequest()
{

    RTC->MODE2.READREQ.reg = RTC_READREQ_RREQ;
    while (RTCisSyncing())
        ;
}

/* Wait for sync in write operations */
inline bool RTC_SAMD21::RTCisSyncing()
{
    return (RTC->MODE2.STATUS.bit.SYNCBUSY);
}

void RTC_SAMD21::RTCdisable()
{
    RTC->MODE2.CTRL.reg &= ~RTC_MODE2_CTRL_ENABLE; // disable RTC
    while (RTCisSyncing())
        ;
}

void RTC_SAMD21::RTCenable()
{
    RTC->MODE2.CTRL.reg |= RTC_MODE2_CTRL_ENABLE; // enable RTC
    while (RTCisSyncing())
        ;
}

void RTC_SAMD21::RTCreset()
{
    RTC->MODE2.CTRL.reg |= RTC_MODE2_CTRL_SWRST; // software reset
    while (RTCisSyncing())
        ;
}

void RTC_SAMD21::RTCresetRemove()
{
    RTC->MODE2.CTRL.reg &= ~RTC_MODE2_CTRL_SWRST; // software reset remove
    while (RTCisSyncing())
        ;
}

extern "C"
{
    void RTC_Handler(void)
    {
        if (RTC_callBack != NULL)
        {
            RTC_callBack();
        }

        RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_ALARM0; // must clear flag at end
    }
}
#endif