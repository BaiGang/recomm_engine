// Copyright 2012. Jike.com. All rights reserved.
// Author: baiyanbing@jike.com (Yan-Bing Bai)
// Descritpion: A timer with precision us

#ifndef _UTIL_BASE_TIMER_H_
#define _UTIL_BASE_TIMER_H_
#include <sys/time.h>
#include <time.h>
#include <string>
namespace util {
class Timer {
 public :
  // Begin timing
  void Begin();
  // End timing
  void End();
  // Get the time in us
  int64_t GetUs();
  // Get the time in ms
  int64_t GetMs();
  // Get the time in second
  int64_t GetS();
  // Get Current time in us
  static int64_t CurrentTimeInUs();
  static int64_t CurrentTimeInMs();
  static int64_t CurrentTimeInS();
  static int32_t CurrentYear(const int& day_offset = 0);
  static int32_t CurrentMonth(const int& day_offset = 0);
  static int32_t CurrentDay(const int& day_offset = 0);
  static std::string LocalTimeStrHMS(const time_t& tt);
  static std::string LocalTimeStr(const time_t& tt);
  // tranform the day time.
  // for example: 2013-04-25 09:00:00  trans to 2013-04-25 00:00:00
  static time_t GetDayTime(const time_t& tt);
  // for example 2013-04-25 09:30:31  trans to 2013-04-25 09:00:00
  static time_t GetHourTime(const time_t& tt);
 private :
  // The start time point
  int64_t begin_point_;
  // The end time point
  int64_t end_point_;
};
};  // namespace util
#endif  // _UTIL_BASE_TIMER_H_
