#ifndef __RTC_H
#define __RTC_H

#define DEV_MAJOR_VERSION 19

#define CMOS_CMD_PORT 0x70
#define CMOS_DATA_PORT 0x71

// Register numbers, you send those to the CMOS command port
#define RTC_SECONDS      0x00
#define RTC_MINUTES      0x02
#define RTC_HOURS        0x04
#define RTC_DAY_OF_WEEK  0x06
#define RTC_DAY_OF_MONTH 0x07
#define RTC_MONTH        0x08
#define RTC_YEAR         0x09

#define RTC_STATUS_A     0x0A // Indicates if there is an update in progress
#define RTC_STATUS_B     0x0B // indicates number format, could be binary or BCD mode

#define RTC_UIP          0x80 // Status A returns most relevant bit as 1 if there is an update in progress, so 0x80 = $1000_0000 
#define RTC_BCD          0x04 // Status B returns $0000_0100 if using BCD

#endif