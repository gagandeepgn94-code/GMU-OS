#include <stdint.h>
#include <stdbool.h>
#include <gmu/rtc.h>
#include <gmu/timer.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDR, reg & 0x7F);
    io_wait();
    return inb(CMOS_DATA);
}

static int cmos_is_updating(void) {
    outb(CMOS_ADDR, 0x0A);
    io_wait();
    return (inb(CMOS_DATA) & 0x80);
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static const char *month_names[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const char *weekday_names[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char *weekday_short[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *month_short[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static gmu_time_t cached_time = {
    .year = 2026,
    .month = 11,
    .day = 24,
    .hour = 9,
    .minute = 1,
    .second = 0,
    .weekday = 2 /* Tuesday */
};

int gmu_rtc_day_of_week(int year, int month, int day) {
    /* Sakamoto's algorithm: returns 0 = Sunday, 1 = Monday, ..., 6 = Saturday */
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

bool gmu_rtc_is_leap_year(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

int gmu_rtc_days_in_month(int year, int month) {
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 30;
    if (month == 2 && gmu_rtc_is_leap_year(year)) return 29;
    return days[month - 1];
}

const char *gmu_rtc_month_name(int month) {
    if (month < 1 || month > 12) return "Unknown";
    return month_names[month - 1];
}

const char *gmu_rtc_weekday_name(int weekday) {
    if (weekday < 0 || weekday > 6) return "Unknown";
    return weekday_names[weekday];
}

void gmu_rtc_init(void) {
    gmu_time_t t;
    if (gmu_rtc_read(&t)) {
        cached_time = t;
    }
}

bool gmu_rtc_read(gmu_time_t *out_time) {
    if (!out_time) return false;

    /* Wait until RTC is not updating */
    int timeout = 20000;
    while (cmos_is_updating() && --timeout > 0) {
        __asm__ volatile ("pause");
    }

    uint8_t s, m, h, d, mo, y, cen;
    uint8_t s2, m2, h2, d2, mo2, y2;

    int attempts = 0;
    do {
        s = cmos_read(0x00);
        m = cmos_read(0x02);
        h = cmos_read(0x04);
        d = cmos_read(0x07);
        mo = cmos_read(0x08);
        y = cmos_read(0x09);
        cen = cmos_read(0x32);

        timeout = 20000;
        while (cmos_is_updating() && --timeout > 0) {
            __asm__ volatile ("pause");
        }

        s2 = cmos_read(0x00);
        m2 = cmos_read(0x02);
        h2 = cmos_read(0x04);
        d2 = cmos_read(0x07);
        mo2 = cmos_read(0x08);
        y2 = cmos_read(0x09);
        attempts++;
    } while ((s != s2 || m != m2 || h != h2 || d != d2 || mo != mo2 || y != y2) && attempts < 5);

    uint8_t reg_b = cmos_read(0x0B);

    /* Convert BCD to binary if needed */
    if (!(reg_b & 0x04)) {
        s = bcd_to_bin(s);
        m = bcd_to_bin(m);
        h = ((h & 0x7F) != h) ? (bcd_to_bin(h & 0x7F) | 0x80) : bcd_to_bin(h);
        d = bcd_to_bin(d);
        mo = bcd_to_bin(mo);
        y = bcd_to_bin(y);
        if (cen) cen = bcd_to_bin(cen);
    }

    /* Convert 12 hour to 24 hour if necessary */
    if (!(reg_b & 0x02)) {
        if (h & 0x80) {
            h = ((h & 0x7F) + 12) % 24;
        } else if (h == 12) {
            h = 0;
        }
    }

    /* Determine full 4-digit year */
    uint16_t full_year;
    if (cen >= 19 && cen <= 21) {
        full_year = (uint16_t)cen * 100 + y;
    } else if (y < 80) {
        full_year = 2000 + y;
    } else {
        full_year = 1900 + y;
    }

    /* Range validation */
    if (mo >= 1 && mo <= 12 && d >= 1 && d <= 31 && h <= 23 && m <= 59 && s <= 59 && full_year >= 2000 && full_year <= 2100) {
        cached_time.second = s;
        cached_time.minute = m;
        cached_time.hour = h;
        cached_time.day = d;
        cached_time.month = mo;
        cached_time.year = full_year;
        cached_time.weekday = (uint8_t)gmu_rtc_day_of_week(full_year, mo, d);
    }

    *out_time = cached_time;
    return true;
}

void gmu_rtc_get_time_str(char *buf, bool with_seconds) {
    if (!buf) return;
    gmu_time_t t;
    gmu_rtc_read(&t);

    buf[0] = '0' + (t.hour / 10);
    buf[1] = '0' + (t.hour % 10);
    buf[2] = ':';
    buf[3] = '0' + (t.minute / 10);
    buf[4] = '0' + (t.minute % 10);

    if (with_seconds) {
        buf[5] = ':';
        buf[6] = '0' + (t.second / 10);
        buf[7] = '0' + (t.second % 10);
        buf[8] = '\0';
    } else {
        buf[5] = '\0';
    }
}

void gmu_rtc_get_date_str(char *buf) {
    if (!buf) return;
    gmu_time_t t;
    gmu_rtc_read(&t);

    /* "Tue, Nov 24, 2026" */
    int p = 0;
    const char *wn = weekday_short[t.weekday % 7];
    while (*wn) buf[p++] = *wn++;
    buf[p++] = ',';
    buf[p++] = ' ';

    const char *mn = month_short[(t.month - 1) % 12];
    while (*mn) buf[p++] = *mn++;
    buf[p++] = ' ';

    if (t.day >= 10) buf[p++] = '0' + (t.day / 10);
    buf[p++] = '0' + (t.day % 10);
    buf[p++] = ',';
    buf[p++] = ' ';

    buf[p++] = '0' + (t.year / 1000);
    buf[p++] = '0' + ((t.year / 100) % 10);
    buf[p++] = '0' + ((t.year / 10) % 10);
    buf[p++] = '0' + (t.year % 10);
    buf[p] = '\0';
}

void gmu_rtc_get_full_str(char *buf) {
    if (!buf) return;
    gmu_time_t t;
    gmu_rtc_read(&t);

    /* "Tue Nov 24  09:01:23" */
    int p = 0;
    const char *wn = weekday_short[t.weekday % 7];
    while (*wn) buf[p++] = *wn++;
    buf[p++] = ' ';

    const char *mn = month_short[(t.month - 1) % 12];
    while (*mn) buf[p++] = *mn++;
    buf[p++] = ' ';

    if (t.day >= 10) buf[p++] = '0' + (t.day / 10);
    buf[p++] = '0' + (t.day % 10);
    buf[p++] = ' ';
    buf[p++] = ' ';

    buf[p++] = '0' + (t.hour / 10);
    buf[p++] = '0' + (t.hour % 10);
    buf[p++] = ':';
    buf[p++] = '0' + (t.minute / 10);
    buf[p++] = '0' + (t.minute % 10);
    buf[p++] = ':';
    buf[p++] = '0' + (t.second / 10);
    buf[p++] = '0' + (t.second % 10);
    buf[p] = '\0';
}

void gmu_rtc_get_tz_time(const gmu_time_t *src, gmu_time_t *dst, int tz_offset_minutes) {
    if (!src || !dst) return;
    *dst = *src;

    int total_min = (int)src->hour * 60 + src->minute + tz_offset_minutes;
    int days_delta = 0;

    while (total_min < 0) {
        total_min += 24 * 60;
        days_delta--;
    }
    while (total_min >= 24 * 60) {
        total_min -= 24 * 60;
        days_delta++;
    }

    dst->hour = (uint8_t)(total_min / 60);
    dst->minute = (uint8_t)(total_min % 60);

    if (days_delta != 0) {
        int new_day = (int)dst->day + days_delta;
        if (new_day < 1) {
            int prev_m = (dst->month == 1) ? 12 : dst->month - 1;
            int prev_y = (dst->month == 1) ? dst->year - 1 : dst->year;
            dst->day = (uint8_t)gmu_rtc_days_in_month(prev_y, prev_m);
            dst->month = (uint8_t)prev_m;
            dst->year = (uint16_t)prev_y;
        } else if (new_day > gmu_rtc_days_in_month(dst->year, dst->month)) {
            dst->day = 1;
            if (dst->month == 12) {
                dst->month = 1;
                dst->year++;
            } else {
                dst->month++;
            }
        } else {
            dst->day = (uint8_t)new_day;
        }
        dst->weekday = (uint8_t)gmu_rtc_day_of_week(dst->year, dst->month, dst->day);
    }
}
