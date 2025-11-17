#include "rtc_second_tick.h"

#include "init.h"

namespace
{
    DateTime g_nextRtcTime;
    bool g_hasNextRtcTime = false;
}

bool rtcSecondTickElapsed(bool resetTimeDone)
{
    if (resetTimeDone)
    {
        g_hasNextRtcTime = false;
        return false;
    }

    if (!g_hasNextRtcTime)
    {
        return false;
    }

    const DateTime now = rtc.now();

    if (static_cast<int32_t>(now.unixtime()) >=
        static_cast<int32_t>(g_nextRtcTime.unixtime()))
    {
        g_hasNextRtcTime = false;
        return true;
    }

    return false;
}

void updateRtcSecondTick(uint32_t secondsToAdd, bool fromNow)
{
    DateTime baseTime;

    if (!g_hasNextRtcTime || fromNow)
    {
        baseTime = rtc.now();
    }
    else
    {
        baseTime = g_nextRtcTime;
    }

    DateTime candidate = baseTime + TimeSpan(secondsToAdd);

    const DateTime now = rtc.now();
    if (!g_hasNextRtcTime || fromNow ||
        static_cast<int32_t>(candidate.unixtime()) >
            static_cast<int32_t>(now.unixtime()))
    {
        g_nextRtcTime = candidate;
    }
    else
    {
        g_nextRtcTime = now + TimeSpan(secondsToAdd);
    }

    g_hasNextRtcTime = true;
}
