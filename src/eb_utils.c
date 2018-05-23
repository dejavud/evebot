#include "eb_utils.h"

static char s_app_dir[MAX_PATH] = { 0 };

void eb_utils_str_free(void* str)
{
    free(str);
}

char* eb_utils_str_wtou8(wchar_t* wstr, int len)
{
    assert(wstr != NULL && len != 0);

    int num_required = WideCharToMultiByte(CP_UTF8, 0, wstr, len, NULL, 0, NULL, NULL);
    if (num_required == 0)
        return NULL;

    char* result = (char*)malloc(num_required);
    int num_written = WideCharToMultiByte(CP_UTF8, 0, wstr, len, result, num_required, NULL, NULL);
    assert(num_written == num_required);
    
    return result;
}

wchar_t* eb_utils_str_u8tow(const char* ustr, int len)
{
    assert(ustr != NULL && len != 0);

    int num_required = MultiByteToWideChar(CP_UTF8, 0, ustr, len, NULL, 0);
    if (num_required == 0)
        return NULL;

    wchar_t* result = (wchar_t*)malloc(num_required * sizeof(wchar_t));
    int num_written = MultiByteToWideChar(CP_UTF8, 0, ustr, len, result, num_required);
    assert(num_written == num_required);

    return result;
}

char* eb_utils_str_u8toa(const char* ustr, int len)
{
    assert(ustr != NULL && len != 0);
    
    int num_required = MultiByteToWideChar(CP_UTF8, 0, ustr, len, NULL, 0);
    if (num_required == 0)
        return NULL;

    wchar_t* wstr = (wchar_t*)malloc(num_required * sizeof(wchar_t));
    int num_written = MultiByteToWideChar(CP_UTF8, 0, ustr, len, wstr, num_required);
    assert(num_written == num_required);

    num_required = WideCharToMultiByte(CP_ACP, 0, wstr, len, NULL, 0, NULL, NULL);
    if (num_required == 0) {
        free(wstr);
        return NULL;
    }

    char* result = (char*)malloc(num_required);
    num_written = WideCharToMultiByte(CP_ACP, 0, wstr, len, result, num_required, NULL, NULL);
    assert(num_written == num_required);
    free(wstr);

    return result;
}

char* eb_utils_str_atou8(const char* astr, int len)
{
    assert(astr != NULL && len != 0);

    int num_required = MultiByteToWideChar(CP_ACP, 0, astr, len, NULL, 0);
    if (num_required == 0)
        return NULL;

    wchar_t* wstr = (wchar_t*)malloc(num_required * sizeof(wchar_t));
    int num_written = MultiByteToWideChar(CP_ACP, 0, astr, len, wstr, num_required);
    assert(num_written == num_required);

    num_required = WideCharToMultiByte(CP_UTF8, 0, wstr, len, NULL, 0, NULL, NULL);
    if (num_required == 0) {
        free(wstr);
        return NULL;
    }

    char* result = (char*)malloc(num_required);
    num_written = WideCharToMultiByte(CP_UTF8, 0, wstr, len, result, num_required, NULL, NULL);
    assert(num_written == num_required);
    free(wstr);

    return result;
}

const char* eb_utils_get_app_dir()
{
    if (s_app_dir[0] != _T('\0')) // already initialized
        return s_app_dir;

    wchar_t dir[MAX_PATH] = { 0 };
    DWORD n = GetModuleFileNameW(NULL, dir, MAX_PATH);
    assert(n > 0 && n < MAX_PATH);

    wchar_t* p = NULL;
    (p = wcsrchr(dir, L'/')) || (p = wcsrchr(dir, L'\\'));
    if (p != NULL) 
        *p = L'\0';

    char* tmp = eb_utils_str_wtou8(dir, -1);
    strncpy(s_app_dir, tmp, MAX_PATH);
    eb_utils_str_free(tmp);

    return s_app_dir;
}

