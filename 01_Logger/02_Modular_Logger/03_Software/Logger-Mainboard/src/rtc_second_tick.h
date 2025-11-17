#ifndef RTC_SECOND_TICK_H
#define RTC_SECOND_TICK_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Checks whether the currently scheduled RTC timestamp has been reached.
 *
 * The function evaluates the most recently set target timestamp and compares
 * it with the current RTC time. If the timestamp has been reached or passed,
 * the function returns @c true. Without a previously scheduled timestamp,
 * @c false is returned.
 *
 * @param resetTimeDone If @c true, any scheduled timestamp is discarded.
 * @return @c true when the scheduled timestamp has been reached; otherwise @c false.
 */
bool rtcSecondTickElapsed(bool resetTimeDone = false);

/**
 * @brief Schedules the next RTC timestamp relative to the previous plan.
 *
 * If no timestamp is tracked yet, @p secondsToAdd is added to @c rtc.now().
 * If a timestamp already exists, @p secondsToAdd is added to that value.
 * Optionally, @p fromNow can enforce that scheduling always starts from the
 * current RTC time.
 *
 * @param secondsToAdd Number of seconds added to the base timestamp.
 * @param fromNow If @c true, always use the current RTC time as the base.
 * @return void
 */
void updateRtcSecondTick(uint32_t secondsToAdd, bool fromNow = false);

#endif // RTC_SECOND_TICK_H
