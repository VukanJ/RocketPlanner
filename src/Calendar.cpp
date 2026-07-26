#include "Calendar.h"
#include <algorithm>
#include <format>

std::tuple<int, int, int, int, int> Date::getYDHMS() const {
    return std::make_tuple(year(), day(), hour(), minute(), second());
}

constexpr int64_t kKerbalYearDays = 426;
constexpr int64_t kKerbalDayHours = 6;
constexpr int64_t kKerbalHourMinutes = 60;
constexpr int64_t kKerbalMinuteSeconds = 60;

constexpr int64_t kKerbalHourSeconds = kKerbalHourMinutes * kKerbalMinuteSeconds;
constexpr int64_t kKerbalDaySeconds  = kKerbalDayHours * kKerbalHourSeconds;
constexpr int64_t kKerbalYearSeconds = kKerbalYearDays * kKerbalDaySeconds;

int KerbalDate::year()   const { return static_cast<int>(totalSeconds / kKerbalYearSeconds) + 1; }
int KerbalDate::day()    const { return static_cast<int>((totalSeconds / kKerbalDaySeconds) % kKerbalYearDays) + 1; }
int KerbalDate::hour()   const { return static_cast<int>((totalSeconds / kKerbalHourSeconds) % kKerbalDayHours); }
int KerbalDate::minute() const { return static_cast<int>((totalSeconds / kKerbalMinuteSeconds) % kKerbalHourMinutes); }
int KerbalDate::second() const { return static_cast<int>(totalSeconds % kKerbalMinuteSeconds); }

void KerbalDate::set(int y, int d, int h, int m, int s) {
    totalSeconds = static_cast<int64_t>(std::max(y, 1) - 1) * kKerbalYearSeconds
                 + static_cast<int64_t>(d - 1) * kKerbalDaySeconds
                 + h * kKerbalHourSeconds + m * kKerbalMinuteSeconds + s;
}

std::string Date::toString() const {
    return std::format("{:04d}-{:03d} {:02d}:{:02d}:{:02d}",
                       year(), day(), hour(), minute(), second());
}
