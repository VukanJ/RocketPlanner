#ifndef CALENDAR_H
#define CALENDAR_H

#include <cstdint>
#include <string>
#include <tuple>
#include <string>

constexpr int64_t kKerbalYearDays = 426;
constexpr int64_t kKerbalDayHours = 6;
constexpr int64_t kKerbalHourMinutes = 60;
constexpr int64_t kKerbalMinuteSeconds = 60;

constexpr int64_t kKerbalHourSeconds = kKerbalHourMinutes * kKerbalMinuteSeconds;
constexpr int64_t kKerbalDaySeconds  = kKerbalDayHours * kKerbalHourSeconds;
constexpr int64_t kKerbalYearSeconds = kKerbalYearDays * kKerbalDaySeconds;

struct DateFormat {
    int year = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    int second = 0;

    void normalize();
};

class Date {
public:
    Date() = default;
    Date(int64_t seconds) : totalSeconds(seconds) {}
    Date(int y, int d, int h, int m, int s);
    Date(const DateFormat& df);

    virtual int year() const = 0;
    virtual int day() const = 0;
    virtual int hour() const = 0;
    virtual int minute() const = 0;
    virtual int second() const = 0;

    virtual DateFormat getYDHMS() const;

    virtual void set(int y, int d, int h, int m, int s) = 0;

    inline int64_t toSeconds() const { return totalSeconds; }
    inline operator int64_t() const { return totalSeconds; }
    Date& operator+=(int64_t seconds) { totalSeconds += seconds; return *this; }

    std::string toString() const;

protected:
    int64_t totalSeconds = 0;
};

inline int64_t operator+(const Date& lhs, int64_t rhs) { return int64_t(lhs) + rhs; }

class KerbalDate : public Date {
public:
    KerbalDate() = default;
    KerbalDate(int64_t seconds) : Date(seconds) { }
    KerbalDate(int y, int d, int h, int m, int s) { set(y, d, h, m, s); }
    KerbalDate(const DateFormat& df) { set(df.year, df.day, df.hour, df.minute, df.second); }
    
    int year() const override;
    int day() const override;
    int hour() const override;
    int minute() const override;
    int second() const override;

    void set(int y, int d, int h, int m, int s) override;
};

#endif // CALENDAR_H
