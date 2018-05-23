#include "eb_log.h"
#include "eb_utils.h"

int s_log_level = EB_LOG_LEVEL_INFO;

static bool eb_do_log(int level, const char* str, int len)
{
    assert(str != NULL);

    if (level > s_log_level)
        return true;

    wchar_t* wstr = eb_utils_str_u8tow(str, len);
    assert(wstr != NULL);
    if (level == EB_LOG_LEVEL_ERROR) {
        fwprintf(stderr, wstr);
    }
    else {
        fwprintf(stdout, wstr);
        fflush(stdout);
    }
    eb_utils_str_free(wstr);

    return true;
}

bool eb_log(int level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    bool r = eb_log_v(level, format, args);
    va_end(args);
    return r;
}

bool eb_log_v(int level, const char* format, va_list args)
{
    assert(format != NULL);

    int num_required = vsnprintf(NULL, 0, format, args);
    if (num_required <= 0)
        return false;

    char* buffer = (char*)malloc(num_required + 1); // +1 space for the additional terminating null character
    int num_written = vsnprintf(buffer, num_required + 1, format, args);
    assert(num_written == num_required);

    int buflen = num_required + 1;
    bool r = eb_do_log(level, buffer, buflen);
    free(buffer);

    return r;
}

void eb_log_set_level(int level)
{
    assert(level >= EB_LOG_LEVEL_ERROR && level <= EB_LOG_LEVEL_DEBUG);

    s_log_level = level;
}
