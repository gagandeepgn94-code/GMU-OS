#ifndef GMU_RTC_H
#define GMU_RTC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t year;    /* e.g. 2026 */
    uint8_t  month;   /* 1 - 12 */
    uint8_t  day;     /* 1 - 31 */
    uint8_t  hour;    /* 0 - 23 */
    uint8_t  minute;  /* 0 - 59 */
    uint8_t  second;  /* 0 - 59 */
    uint8_t  weekday; /* 0 = Sunday, 1 = Monday ... 6 = Saturday */
} gmu_time_t;

void gmu_rtc_init(void);
bool gmu_rtc_read(gmu_time_t *out_time);
void gmu_rtc_get_time_str(char *buf, bool with_seconds);
void gmu_rtc_get_date_str(char *buf);
void gmu_rtc_get_full_str(char *buf);

int gmu_rtc_day_of_week(int year, int month, int day);
int gmu_rtc_days_in_month(int year, int month);
bool gmu_rtc_is_leap_year(int year);
const char *gmu_rtc_month_name(int month);
const char *gmu_rtc_weekday_name(int weekday);

void gmu_rtc_get_tz_time(const gmu_time_t *src, gmu_time_t *dst, int tz_offset_minutes);

#endif
