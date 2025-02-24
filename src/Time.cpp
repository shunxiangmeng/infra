#include <chrono>
#include <algorithm>
#include <string.h>
#include "infra/include/Timestamp.h"
#include "infra/include/NtpTime.h"

namespace infra {

int64_t getCurrentTimeNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t getCurrentTimeUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t getCurrentTimeS() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string getCurrentTime() {
    //auto now = std::chrono::high_resolution_clock::now();
    //std::time_t now_c = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //return std::ctime(&now_c);
    time_t now = time(NULL);
    struct tm* local;
    //local = std::gmtime(&now);  utc time
    local = localtime(&now);
    char time_str[128];
    size_t size = strftime(time_str, sizeof(time_str), "%Y-%m-%d %X", local);
    return std::string(time_str);
}

#define OSD_FMT_SPACE " "
#define OSD_FMT_TIME0 "24hour"
#define OSD_FMT_TIME1 "12hour"
#define OSD_FMT_WEEK0 "WEEKCN"
#define OSD_FMT_WEEK1 "WEEK"
#define OSD_FMT_CHR  "CHR"
#define OSD_FMT_YMD0 "YYYY-MM-DD"
#define OSD_FMT_YMD1 "MM-DD-YYYY"
#define OSD_FMT_YMD2 "DD-MM-YYYY"
#define OSD_FMT_YMD3 "YYYY/MM/DD"
#define OSD_FMT_YMD4 "MM/DD/YYYY"
#define OSD_FMT_YMD5 "DD/MM/YYYY"

std::string getCurrentDateTime(const char *fmt) {
    char year[8] = {0}, month[4] = {0}, day[4] = {0};
    char week[16] = {0}, hms[12] = {0};
    char w_ymd[16] = {0};
    char w_week[16] = {0};
    int wid = -1;
#ifndef _WIN32
    time_t curtime;
    curtime = time(0);
    strftime(year, sizeof(year), "%Y", localtime(&curtime));
    strftime(month, sizeof(month), "%m", localtime(&curtime));
    strftime(day, sizeof(day), "%d", localtime(&curtime));

    if (strstr(fmt, OSD_FMT_TIME0)) {
        strftime(hms, sizeof(hms), "%H:%M:%S", localtime(&curtime));
    } else if (strstr(fmt, OSD_FMT_TIME1)) {
        strftime(hms, sizeof(hms), "%I:%M:%S %p", localtime(&curtime));
    }

    if (strstr(fmt, OSD_FMT_WEEK0)) {
        strftime(week, sizeof(week), "%u", localtime(&curtime));
        wid = week[0] - '0';
        switch (wid) {
        case 1:
            sprintf(w_week, " 星期一");
            break;
        case 2:
            sprintf(w_week, " 星期二");
            break;
        case 3:
            sprintf(w_week, " 星期三");
            break;
        case 4:
            sprintf(w_week, " 星期四");
            break;
        case 5:
            sprintf(w_week, " 星期五");
            break;
        case 6:
            sprintf(w_week, " 星期六");
            break;
        case 7:
            sprintf(w_week, " 星期日");
            break;
        default:
            sprintf(w_week, " 星期*");
            break;
        }
    } else if (strstr(fmt, OSD_FMT_WEEK1)) {
        strftime(week, sizeof(week), "%A", localtime(&curtime));
        sprintf(w_week, " %s", week);
    }

    if (strstr(fmt, OSD_FMT_CHR)) {
        if (strstr(fmt, OSD_FMT_YMD0))
            sprintf(w_ymd, "%s-%s-%s", year, month, day);
        else if (strstr(fmt, OSD_FMT_YMD1))
            sprintf(w_ymd, "%s-%s-%s", month, day, year);
        else if (strstr(fmt, OSD_FMT_YMD2))
            sprintf(w_ymd, "%s-%s-%s", day, month, year);
        else if (strstr(fmt, OSD_FMT_YMD3))
            sprintf(w_ymd, "%s/%s/%s", year, month, day);
        else if (strstr(fmt, OSD_FMT_YMD4))
            sprintf(w_ymd, "%s/%s/%s", month, day, year);
        else if (strstr(fmt, OSD_FMT_YMD5))
            sprintf(w_ymd, "%s/%s/%s", day, month, year);
    } else {
        if (strstr(fmt, OSD_FMT_YMD0))
            sprintf(w_ymd, "%s年%s月%s日", year, month, day);
        else if (strstr(fmt, OSD_FMT_YMD1))
            sprintf(w_ymd, "%s月%s日%s年", month, day, year);
        else if (strstr(fmt, OSD_FMT_YMD2))
            sprintf(w_ymd, "%s日%s月%s年", day, month, year);
    }
#endif
    char result[40] = {0};
    snprintf(result, sizeof(result), "%s%s %s", w_ymd, w_week, hms);
    return std::string(result);
}

std::string convertTimestampToString(time_t timestamp) {
    struct tm* local;
    local = localtime(&timestamp);
    char time_str[128];
    size_t size = strftime(time_str, sizeof(time_str), "%Y-%m-%d %X", local);
    return std::string(time_str);
}

NtpTime convertTimestampToNtpTime(Timestamp timestamp) {
    int64_t now_us = timestamp.micros();
    uint32_t seconds = (uint32_t)(now_us / 1000000) + kNtpJan1970;
    uint32_t fractions = static_cast<uint32_t>((now_us % 1000000) * kMagicNtpFractionalUnit / 1000000);
    return NtpTime(seconds, fractions);
}

TimeDelta compactNtpRttToTimeDelta(uint32_t compact_ntp_interval) {
    static TimeDelta kMinRtt = TimeDelta::millis(1);
    // Interval to convert expected to be positive, e.g. RTT or delay.
    // Because interval can be derived from non-monotonic ntp clock,
    // it might become negative that is indistinguishable from very large values.
    // Since very large RTT/delay is less likely than non-monotonic ntp clock,
    // such value is considered negative and converted to minimum value of 1ms.
    if (compact_ntp_interval > 0x80000000) {
        return kMinRtt;
    }
    // Convert to 64bit value to avoid multiplication overflow.
    int64_t value = static_cast<int64_t>(compact_ntp_interval);
    // To convert to TimeDelta need to divide by 2^16 to get seconds,
    // then multiply by 1'000'000 to get microseconds. To avoid float operations,
    // multiplication and division are swapped.
    int64_t us = divideRoundToNearest(value * kNumMicrosecsPerSec, 1 << 16);
    // Small RTT value is considered too good to be true and increased to 1ms.
    return std::max(TimeDelta::micros(us), kMinRtt);
}

}