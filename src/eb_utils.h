#ifndef EVEBOT_UTILS_H
#define EVEBOT_UTILS_H

#include "eb_common.h"

void eb_utils_str_free(void* str);
char* eb_utils_str_wtou8(wchar_t* wstr, int len);  // len must include null terminator or be -1
wchar_t* eb_utils_str_u8tow(const char* ustr, int len);
char* eb_utils_str_u8toa(const char* ustr, int len);
char* eb_utils_str_atou8(const char* astr, int len);

const char* eb_utils_get_app_dir();


#endif // EVEBOT_UTILS_H
