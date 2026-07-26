#include "Calendar.h"
#include <format>

void DateFormat::normalize(bool KerbalCalendar) {
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
    int startDay = KerbalCalendar ? kKerbalEpochDay : 0;
    if (day >= kKerbalYearDays) {
        year += (day - startDay) / kKerbalYearDays;
        day = ((day - startDay) % kKerbalYearDays) + startDay;
    }
    else if (day < startDay) {
        year -= (-day + kKerbalYearDays) / kKerbalYearDays;
        day = ((day - startDay) % kKerbalYearDays + kKerbalYearDays) % kKerbalYearDays + startDay;
    }
}

DateFormat Date::getYDHMS(bool asDuration) const {
    if (asDuration) {
        return { year() - 1, day() - 1, hour(), minute(), second() };
    }
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
