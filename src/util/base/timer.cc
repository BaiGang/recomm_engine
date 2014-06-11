// Copyright 2012. Jike.com. All rights reserved.
// Author: baiyanbing@jike.com (Yan-Bing Bai)

#include "./timer.h"
#include "util/string/str_util.h"

namespace util {
void Timer::Begin() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  begin_point_ = tv.tv_sec * 1000000 + tv.tv_usec;
}

void Timer::End() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  end_point_ = tv.tv_sec * 1000000 + tv.tv_usec;
}

int64_t Timer::GetUs() {
  return end_point_ - begin_point_;
}

int64_t Timer::GetMs() {
  return (end_point_ - begin_point_) / 1000;
}

int64_t Timer::GetS() {
  return (end_point_ - begin_point_) / 1000000;
}
int64_t Timer::CurrentTimeInUs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}
int64_t Timer::CurrentTimeInMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec/1000;
}
int64_t Timer::CurrentTimeInS() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec / (1000*1000);
}
std::string Timer::LocalTimeStrHMS(const time_t& tt) {
  std::string ret;
  struct tm t;
  localtime_r(&tt, &t);
  StringAppendF(&ret, "%4d%02d%02d%02d%02d%02d",
                t.tm_year + 1900,
                t.tm_mon + 1,
                t.tm_mday,
                t.tm_hour,
                t.tm_min,
                t.tm_sec);
  return ret;
}
std::string Timer::LocalTimeStr(const time_t& tt) {
  std::string ret;
  struct tm t;
  localtime_r(&tt, &t);
  StringAppendF(&ret, "%4d-%02d-%02d %02d:%02d:%02d",
                t.tm_year + 1900,
                t.tm_mon + 1,
                t.tm_mday,
                t.tm_hour,
                t.tm_min,
                t.tm_sec);
  return ret;
}

int32_t Timer::CurrentYear(const int& day_offset) {
  time_t tt = time(NULL);
  tt += day_offset * 86400;
  struct tm t;
  localtime_r(&tt, &t);
  int32_t year = t.tm_year;
  return year + 1900;
}
int32_t Timer::CurrentMonth(const int& day_offset) {
  time_t tt = time(NULL);
  tt += day_offset * 86400;
  struct tm t;
  localtime_r(&tt, &t);
  return t.tm_mon + 1;
}
int32_t Timer::CurrentDay(const int32_t& day_offset) {
  time_t tt = time(NULL);
  // Adjust
  tt += day_offset * 86400;
  struct tm t;
  localtime_r(&tt, &t);
  return t.tm_mday;
}

time_t Timer::GetDayTime(const time_t& tt) {
  struct tm t;
  localtime_r(&tt, &t);
  t.tm_hour = 0;
  t.tm_min = 0;
  t.tm_sec = 0;
  time_t res = mktime(&t);
  return res;
}
time_t Timer::GetHourTime(const time_t& tt) {
  struct tm t;
  localtime_r(&tt, &t);
  t.tm_min = 0;
  t.tm_sec = 0;
  time_t res = mktime(&t);
  return res;
}
};  // namesapce util
