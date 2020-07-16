#include "RTC_SAMD51.h"
#include "DateTime.h"

RTC_SAMD51 rtc;
void setup()
{
    rtc.begin();

    Serial.begin(115200);

    while (!Serial)
    {
        ;
    }

    DateTime now = DateTime(F(__DATE__), F(__TIME__));
    Serial.println("adjust time!");
    rtc.adjust(now);

    now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    DateTime alarm0 = DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second() + 5);

    rtc.setAlarm(0, alarm0);               // match after 5 seconds
    rtc.enableAlarm(0, rtc.MATCH_HHMMSS); // match Every Day

    DateTime alarm1 = DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second() + 10);
    rtc.setAlarm(1, alarm1);               // match after 10 seconds
    rtc.enableAlarm(1, rtc.MATCH_HHMMSS); // match Every Day

    rtc.attachInterrupt(alarmMatch); // callback whlie alarm is match
}

void loop()
{
}

void alarmMatch(uint32_t flag)
{
    Serial.printf("flag: %u\n\r", flag);
    if (flag & 1)
    {
        Serial.println("Alarm0 Match!");
        DateTime now = rtc.now();
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print(" ");
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();
    }
    if (flag & 2)
    {
        Serial.println("Alarm1 Match!");
        DateTime now = rtc.now();
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print(" ");
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.println();
    }
}
