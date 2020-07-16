# Seeed_Arduino_RTC  [![Build Status](https://travis-ci.com/Seeed-Studio/Seeed_Arduino_RTC.svg?branch=master)](https://travis-ci.com/Seeed-Studio/Seeed_Arduino_RTC)
## Introduction 
Seeed Arduino RTC is a Arduino library for RTC Moudle. At present, samd21 is supported and samd51 has built-in RTC module. 
## How to install  Arduino Library
please refer [here](https://wiki.seeedstudio.com/How_to_install_Arduino_Library/).

## Usage

```C++
#include "RTC_SAMD21.h"
#include "DateTime.h"

RTC_SAMD21 rtc;
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

}
void loop()
{
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

}

```

## API Reference
- **begin(*void*) : bool**
```C++
rtc.begin()
```
- **now(*void*) : DateTime**
```C++
DateTime now = rtc.now();
```
- **adjust(*const DateTime*) : void**
```C++
DateTime now = DateTime(F(__DATE__), F(__TIME__));
rtc.adjust(now);
DateTime now = DateTime(2020, 7, 15, 0, 0, 0);
rtc.adjust(now);
```
- **setAlarm(*const DateTime*) : void**
```C++
DateTime alarm = DateTime(2020, 7, 15, 18, 0, 0);
rtc.setAlarm(now);
```
- **alarm(*void*) : DateTime**
```C++
DateTime now = rtc.alarm();
```
- **disableAlarm(*uint8_t*) : void**
```C++
rtc.disableAlarm(0);
```
- **enableAlarm(*uint8_t*) : void**
```C++
rtc.enableAlarm(0);
```
- **attachInterrupt(*rtcCallBack*) : void**
```C++
rtc.attachInterrupt(rtcCallBack);
```
- **detachInterrupt(*void*) : void**
```C++
rtc.detachInterrupt();
```

----
## License
This software is written by seeed studio<br>
and is licensed under [The MIT License](http://opensource.org/licenses/mit-license.php). Check License.txt for more information.<br>

Contributing to this software is warmly welcomed. You can do this basically by<br>
[forking](https://help.github.com/articles/fork-a-repo), committing modifications and then [pulling requests](https://help.github.com/articles/using-pull-requests) (follow the links above<br>
for operating guide). Adding change log and your contact into file header is encouraged.<br>
Thanks for your contribution.

Seeed Studio is an open hardware facilitation company based in Shenzhen, China. <br>
Benefiting from local manufacture power and convenient global logistic system, <br>
we integrate resources to serve new era of innovation. Seeed also works with <br>
global distributors and partners to push open hardware movement.<br>