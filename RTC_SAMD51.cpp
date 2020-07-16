#ifdef __SAMD51__
#include "RTC_SAMD51.h"

#define rtcFuncPtr void(uint32)
voidFuncPtr RTC_callBack = NULL;

bool RTC_SAMD51::begin()
{
    configureClock();
    config32kOSC();

    uint16_t tmp_reg_a = 0;

    // If the RTC is in clock mode and the reset was
    // not due to POR or BOD, preserve the clock time
    // POR causes a reset anyway, BOD behaviour is?
    bool validTime = false;
    RTC_MODE2_CLOCK_Type oldTime;

    if (RTC->MODE2.CTRLA.reg & RTC_MODE2_CTRLA_MODE_CLOCK)
    {
        validTime = true;
        oldTime.reg = RTC->MODE2.CLOCK.reg;
    }

    RTCdisable();

    RTCreset();

    tmp_reg_a |= RTC_MODE2_CTRLA_MODE_CLOCK; // set clock operating mode
    tmp_reg_a |= RTC_MODE2_CTRLA_PRESCALER_DIV1024; // set prescaler to 1024 for MODE2
    tmp_reg_a &= ~RTC_MODE2_CTRLA_MATCHCLR;  // disable clear on match

    //According to the datasheet RTC_MODE2_CTRL_CLKREP = 0 for 24h
    tmp_reg_a &= ~RTC_MODE2_CTRLA_CLKREP; // 24h time representation


    RTC->MODE2.CTRLA.reg = tmp_reg_a;
    while (RTCisSyncing())
        ;

    NVIC_EnableIRQ(RTC_IRQn); // enable RTC interrupt
    NVIC_SetPriority(RTC_IRQn, 0x00);

    RTC->MODE2.INTENSET.reg |= RTC_MODE2_INTENSET_ALARM0; // enable alarm0 interrupt
    
    RTC->MODE2.Mode2Alarm[0].MASK.bit.SEL = MATCH_OFF;    // default alarm0 match is off (disabled)

    RTC->MODE2.INTENSET.reg |= RTC_MODE2_INTENSET_ALARM1; // enable alarm1 interrupt
    RTC->MODE2.Mode2Alarm[1].MASK.bit.SEL = MATCH_OFF;    // default alarm1 match is off (disabled)

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

void RTC_SAMD51::adjust(const DateTime &dt)
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

DateTime RTC_SAMD51::now()
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

DateTime RTC_SAMD51::alarm(uint8_t id)
{
    if (id >= 2)
        return DateTime();
    uint8_t ss = RTC->MODE2.Mode2Alarm[0].ALARM.bit.SECOND;
    uint8_t mm = RTC->MODE2.Mode2Alarm[0].ALARM.bit.MINUTE;
    uint8_t hh = RTC->MODE2.Mode2Alarm[0].ALARM.bit.HOUR;
    uint8_t d = RTC->MODE2.Mode2Alarm[0].ALARM.bit.DAY;
    uint8_t m = RTC->MODE2.Mode2Alarm[0].ALARM.bit.MONTH;
    uint16_t y = RTC->MODE2.Mode2Alarm[0].ALARM.bit.YEAR;

    return DateTime(y, m, d, hh, mm, ss);
}

void RTC_SAMD51::setAlarm(uint8_t id, const DateTime &dt)
{
    if (id >= 2)
        return;
    RTC_MODE2_CLOCK_Type alarmTime;

    alarmTime.bit.SECOND = dt.second();
    alarmTime.bit.MINUTE = dt.minute();
    alarmTime.bit.HOUR = dt.hour();
    alarmTime.bit.DAY = dt.day();
    alarmTime.bit.MONTH = dt.month();
    alarmTime.bit.YEAR = dt.year() - 2000;

    RTC->MODE2.Mode2Alarm[id].ALARM.reg = alarmTime.reg;
}

void RTC_SAMD51::enableAlarm(uint8_t id, Alarm_Match match)
{
    if (id >= 2)
        return;
    RTC->MODE2.Mode2Alarm[id].MASK.bit.SEL = match;
}

void RTC_SAMD51::disableAlarm(uint8_t id)
{
    if (id >= 2)
        return;
    RTC->MODE2.Mode2Alarm[id].MASK.bit.SEL = 0x00;
}

void RTC_SAMD51::attachInterrupt(voidFuncPtr callback)
{
    RTC_callBack = callback;
}

void RTC_SAMD51::detachInterrupt()
{
    RTC_callBack = NULL;
}

void RTC_SAMD51::standbyMode()
{
    // Entering standby mode when connected
    // via the native USB port causes issues.
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
}

/* Attach peripheral clock to 32k oscillator */
void RTC_SAMD51::configureClock()
{
   MCLK->APBAMASK.reg |= MCLK_APBAMASK_RTC | MCLK_APBAMASK_OSC32KCTRL;
}

/*
 * Private Utility Functions
 */

/* Configure the 32768Hz Oscillator */
void RTC_SAMD51::config32kOSC()
{
    void *hw = (void *)OSC32KCTRL;
    uint16_t calib = 0;

    calib = ((Osc32kctrl *)hw)->OSCULP32K.reg;
    calib = (calib & OSC32KCTRL_OSCULP32K_CALIB_Msk) >> OSC32KCTRL_OSCULP32K_CALIB_Pos;

    ((Osc32kctrl *)hw)->OSCULP32K.reg &= ~OSC32KCTRL_OSCULP32K_WRTLOCK;

    ((Osc32kctrl *)hw)->OSCULP32K.reg = calib;
    ((Osc32kctrl *)hw)->OSCULP32K.reg &= ~(OSC32KCTRL_OSCULP32K_EN32K);
    ((Osc32kctrl *)hw)->OSCULP32K.reg |= OSC32KCTRL_OSCULP32K_EN1K;

    ((Osc32kctrl *)hw)->OSCULP32K.reg |= OSC32KCTRL_OSCULP32K_WRTLOCK;

    ((Osc32kctrl *)hw)->RTCCTRL.reg = OSC32KCTRL_RTCCTRL_RTCSEL(OSC32KCTRL_RTCCTRL_RTCSEL_ULP1K_Val);

    while (!((Osc32kctrl *)hw)->STATUS.bit.XOSC32KRDY)
    {
        /* code */
    }
    
}

/* Synchronise the CLOCK register for reading*/
inline void RTC_SAMD51::RTCreadRequest()
{
    RTC->MODE2.CTRLA.reg |= RTC_MODE2_CTRLA_CLOCKSYNC;
    while (RTCisSyncing(RTC_MODE2_SYNCBUSY_SWRST | RTC_MODE2_SYNCBUSY_ENABLE | RTC_MODE2_SYNCBUSY_CLOCKSYNC))
        ;
}

/* Wait for sync in write operations */
inline bool RTC_SAMD51::RTCisSyncing(uint32_t flag)
{
    return (RTC->MODE2.SYNCBUSY.reg & flag);
}

void RTC_SAMD51::RTCdisable()
{
    RTC->MODE2.CTRLA.reg &= ~RTC_MODE2_CTRLA_ENABLE; // disable RTC
    while (RTCisSyncing(RTC_MODE2_SYNCBUSY_SWRST | RTC_MODE2_SYNCBUSY_ENABLE | RTC_MODE2_SYNCBUSY_CLOCKSYNC))
        ;
}

void RTC_SAMD51::RTCenable()
{
    RTC->MODE2.CTRLA.reg |= RTC_MODE2_CTRLA_ENABLE; // enable RTC
    while (RTCisSyncing(RTC_MODE2_SYNCBUSY_SWRST | RTC_MODE2_SYNCBUSY_ENABLE | RTC_MODE2_SYNCBUSY_CLOCKSYNC))
        ;
}

void RTC_SAMD51::RTCreset()
{
    RTC->MODE2.CTRLA.reg |= RTC_MODE2_CTRLA_SWRST; // software reset
    while (RTCisSyncing(RTC_MODE2_CTRLA_SWRST))
        ;
}

void RTC_SAMD51::RTCresetRemove()
{
    RTC->MODE2.CTRLA.reg &= ~RTC_MODE2_CTRLA_SWRST; // software reset remove
    while (RTCisSyncing(RTC_MODE2_CTRLA_SWRST))
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

        RTC->MODE2.INTFLAG.reg = RTC_MODE2_INTFLAG_ALARM0 | RTC_MODE2_INTFLAG_ALARM1; // must clear flag at end
    }
}
#endif