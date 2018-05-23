#ifndef EVEBOT_LOG_H
#define EVEBOT_LOG_H

#include "eb_common.h"

typedef enum eb_log_level_t
{
    EB_LOG_LEVEL_ERROR = 0,
    EB_LOG_LEVEL_WARNING,
    EB_LOG_LEVEL_INFO,
    EB_LOG_LEVEL_DEBUG,
} eb_log_level;

bool eb_log(int level, const char* format, ...);
bool eb_log_v(int level, const char* format, va_list args);

void eb_log_set_level(int level);

// shortcut
#define LOGI(...) eb_log(EB_LOG_LEVEL_INFO, __VA_ARGS__)
#define LOGW(...) eb_log(EB_LOG_LEVEL_WARNING, __VA_ARGS__)
#define LOGE(...) eb_log(EB_LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOGD(...) eb_log(EB_LOG_LEVEL_DEBUG, __VA_ARGS__)


#ifdef EB_PERFORMANCE_TEST
#define EB_PERFORMANCE_TEST_BEGIN  DWORD eb_performance_test_tick = GetTickCount();
#define EB_PERFORMANCE_TEST_END(x) LOGD(_T("[%s] Total time: %.2lfs\n"), x, (GetTickCount() - eb_performance_test_tick) / 1000.0);
#else
#define EB_PERFORMANCE_TEST_BEGIN
#define EB_PERFORMANCE_TEST_END(x)
#endif // DEBUG


#endif // EVEBOT_LOG_H
