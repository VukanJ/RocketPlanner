#include "Calendar.h"
#include <format>

void DateFormat::normalize() {
    if (second >= kKerbalMinuteSeconds) {
        minute += second / kKerbalMinuteSeconds;
        second %= kKerbalMinuteSeconds;
    }
    else if (second < 0) {
        minute -= (-second + kKerbalMinuteSeconds - 1) / kKerbalMinuteSeconds;
        second = (second % kKerbalMinuteSeconds + kKerbalMinuteSeconds) % kKerbalMinuteSeconds;
    }
    if (minute >= kKerbalHourMinutes) {
        hour += minute / kKerbalHourMinutes;
        minute %= kKerbalHourMinutes;
    }
    else if (minute < 0) {
        hour -= (-minute + kKerbalHourMinutes - 1) / kKerbalHourMinutes;
        minute = (minute % kKerbalHourMinutes + kKerbalHourMinutes) % kKerbalHourMinutes;
    }
    if (hour >= kKerbalDayHours) {
        day += hour / kKerbalDayHours;
        hour %= kKerbalDayHours;
    }
    else if (hour < 0) {
        day -= (-hour + kKerbalDayHours - 1) / kKerbalDayHours;
        hour = (hour % kKerbalDayHours + kKerbalDayHours) % kKerbalDayHours;
    }
    if (day > kKerbalYearDays) {
        year += (day - 1) / kKerbalYearDays;
        day = ((day - 1) % kKerbalYearDays) + 1;
    }
    else if (day < 1) {
        year -= (-day + kKerbalYearDays) / kKerbalYearDays;
        day = ((day - 1) % kKerbalYearDays + kKerbalYearDays) % kKerbalYearDays + 1;
    }
}

DateFormat Date::getYDHMS() const {
    return { year(), day(), hour(), minute(), second() };
}

int KerbalDate::year()   const { return static_cast<int>(totalSeconds / kKerbalYearSeconds) + 1; }
int KerbalDate::day()    const { return static_cast<int>((totalSeconds / kKerbalDaySeconds) % kKerbalYearDays) + 1; }
int KerbalDate::hour()   const { return static_cast<int>((totalSeconds / kKerbalHourSeconds) % kKerbalDayHours); }
int KerbalDate::minute() const { return static_cast<int>((totalSeconds / kKerbalMinuteSeconds) % kKerbalHourMinutes); }
int KerbalDate::second() const { return static_cast<int>(totalSeconds % kKerbalMinuteSeconds); }

void KerbalDate::set(int y, int d, int h, int m, int s) {
    totalSeconds = static_cast<int64_t>(y - 1) * kKerbalYearSeconds
                 + static_cast<int64_t>(d - 1) * kKerbalDaySeconds
                 + h * kKerbalHourSeconds + m * kKerbalMinuteSeconds + s;
}

std::string Date::toString() const {
    return std::format("{:04d}-{:03d} {:02d}:{:02d}:{:02d}", year(), day(), hour(), minute(), second());
}
