#include "eb_image.h"
#include "eb_utils.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

static COLORREF eb_image_get_pixel_color(uint8_t* data, int channel, int x, int y, int w, int h);
static void eb_image_set_pixel_color(uint8_t* data, int channel, int x, int y, int w, int h, COLORREF clr);
static bool eb_image_compare_block(uint8_t* p, int cp, int wp, int hp, uint8_t* q, int cq, int wq, int hq, int x, int y, int deviation);
static bool eb_image_compare_color(COLORREF ca, COLORREF cb, int deviation);
static bool eb_image_color_in_range(COLORREF clr, COLORREF range_begin, COLORREF range_end);

eb_image* eb_image_new(void* user_data)
{
    eb_image* new_image = (eb_image*)malloc(sizeof(eb_image));
    if (new_image == NULL)
        return NULL;

    new_image->data = NULL;
    new_image->width = new_image->height = 0;
    new_image->channels = 0;
    new_image->load_from_file = false;
    new_image->user_data = user_data;

    return new_image;
}

void eb_image_delete(eb_image* image)
{
    if (image == NULL)
        return;

    free(image->data);
    free(image);
}

bool eb_image_is_empty(eb_image* image)
{
    assert(image != NULL);
    return image->data == NULL;
}

bool eb_image_load_from_file(eb_image* image, const char* filepath)
{
    assert(image != NULL && filepath != NULL);

    if (image->data != NULL) {
        free(image->data);
        image->data = NULL;
        image->width = image->height = 0;
        image->channels = 0;
    }

    char* filepath_a = eb_utils_str_u8toa(filepath, -1);
    
    int result = false;
    int channels_in_file;
    image->data = stbi_load(filepath_a, &image->width, &image->height, &channels_in_file, 4);
    if (image->data != NULL) {
        image->channels = 4;  // we desire 4
        image->load_from_file = true;
        result = true;
    }

    eb_utils_str_free(filepath_a);

    return result;
}

bool eb_image_save_to_file(eb_image* image, const char* filepath)
{
    assert(image != NULL && filepath != NULL);

    if (image->data == NULL || image->width < 0 || image->height < 0)
        return false;

    char* filepath_a = eb_utils_str_u8toa(filepath, -1);
    bool result = (stbi_write_png(filepath_a, image->width, image->height, image->channels, image->data, image->channels * image->width) != 0);
    eb_utils_str_free(filepath_a);

    return result;
}

bool eb_image_capture_window(eb_image* image, HWND hwnd)
{
    assert(image != NULL && hwnd != NULL);

    if (!IsWindow(hwnd))
        return false;

    WINDOWPLACEMENT wp = { 0 };
    wp.length = sizeof(WINDOWPLACEMENT);
    if (!GetWindowPlacement(hwnd, &wp))
        return false;

    bool minimized = false;
    if (wp.showCmd == SW_SHOWMINIMIZED)
        minimized = true;

    LONG pre_style = 0;
    if (minimized) {
        //pre_style = GetWindowLong(hwnd, GWL_EXSTYLE);
        //SetWindowLong(hwnd, GWL_EXSTYLE, pre_style | WS_EX_LAYERED);
        //SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    }

    bool result = false;
    do {
        RECT wndrect = { 0 };
        if (!GetWindowRect(hwnd, &wndrect))
            break;

        int w = wndrect.right - wndrect.left;
        int h = wndrect.bottom - wndrect.top;
        if (w <= 0 || h <= 0)
            break;

        if (image->data != NULL) {
            free(image->data);
            image->data = NULL;
            image->width = image->height = 0;
            image->channels = 0;
        }

        HDC hwnddc = GetDC(hwnd);
        assert(hwnddc != NULL);
        HDC hmemdc = CreateCompatibleDC(hwnddc);
        HBITMAP hbmp = CreateCompatibleBitmap(hwnddc, w, h);
        assert(hmemdc != NULL && hbmp != NULL);
        HGDIOBJ preobj = SelectObject(hmemdc, hbmp);

        do {
            /*
            if (!BitBlt(hmemdc, 0, 0, w, h, hwnddc, 0, 0, SRCCOPY)) {
            LOGD("*** %d\n", GetLastError());
            break;
            }
            */
            if (!PrintWindow(hwnd, hmemdc, PW_CLIENTONLY))
                break;


            BITMAP bminfo = { 0 };
            if (GetObject(hbmp, sizeof(bminfo), &bminfo) == 0)
                break;

            BITMAPINFO bi = { 0 };
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            if (GetDIBits(hwnddc, hbmp, 0, bminfo.bmHeight, NULL, &bi, DIB_RGB_COLORS) == 0)
                break;

            bi.bmiHeader.biHeight = -bminfo.bmHeight; // top-down ordering of lines
            bi.bmiHeader.biCompression = BI_RGB; // no compression
            image->data = (uint8_t*)malloc(bi.bmiHeader.biSizeImage);
            if (GetDIBits(hwnddc, hbmp, 0, bminfo.bmHeight, image->data, &bi, DIB_RGB_COLORS) == 0)
                break;

            // convert data in dib to stb image data structure
            uint8_t* data = image->data;
            for (int i = 0; i < bminfo.bmWidth * bminfo.bmHeight; i++) {
                // swap red and blue
                uint8_t tmp = data[i * 4];
                data[i * 4] = data[i * 4 + 2];
                data[i * 4 + 2] = tmp;
            }

            image->width = bminfo.bmWidth;
            image->height = bminfo.bmHeight;
            image->channels = bi.bmiHeader.biBitCount / 8;

            result = true;
        } while (false);

        SelectObject(hmemdc, preobj);
        DeleteDC(hmemdc);
        DeleteObject(hbmp);
        ReleaseDC(hwnd, hwnddc);

    } while (false);

    if (minimized) {
        //ShowWindow(hwnd, SW_MINIMIZE);
        //SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        //SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
        //SetWindowLong(hwnd, GWL_EXSTYLE, pre_style);
    }

    return result;
}

bool eb_image_search(const eb_image* src, const eb_image* target, RECT rc, POINT* pt, int deviation)
{
    assert(src != NULL && target != NULL && pt != NULL);
    assert(src->data != NULL && src->width > 0 && src->height > 0);
    assert(target->data != NULL && target->width > 0 && target->height > 0);
    assert(rc.left >= 0 && rc.top >= 0 && rc.right <= src->width && rc.bottom <= src->height);

    int sw = src->width;
    int sh = src->height;
    int tw = target->width;
    int th = target->height;

    if (tw > sw || th > sh)
        return false;

    uint8_t* p = src->data;
    int cp = src->channels;
    uint8_t* q = target->data;
    int cq = target->channels;

    int w = rc.right < (sw - tw) ? rc.right : (sw - tw);
    int h = rc.bottom < (sh - th) ? rc.bottom : (sh - th);
    for (int y = rc.top; y < h; y++) {
        for (int x = rc.left; x < w; x++) {
            if (eb_image_compare_block(p, cp, sw, sh, q, cq, tw, th, x, y, deviation)) {
                pt->x = x;
                pt->y = y;
                return true;
            }
        }
    }

    return false;
}

bool eb_image_search_entire(const eb_image* src, const eb_image* target, POINT* pt, int deviation)
{
    RECT rc = { 0, 0, src->width, src->height };
    return eb_image_search(src, target, rc, pt, deviation);
}

bool eb_image_search_line(const eb_image* src, COLORREF clr, int width, RECT rc, POINT* pt, int deviation)
{
    assert(src != NULL);
    assert(width <= src->width);

    int w = rc.right < (src->width - width) ? rc.right : (src->width - width);
    int h = rc.bottom < (src->height - 1) ? rc.bottom : (src->height - 1);

    for (int y = rc.top; y < h; y++) {
        for (int x = rc.left; x < w; x++) {
            COLORREF c = eb_image_get_pixel_color(src->data, src->channels, x, y, src->width, src->height);
            if (!eb_image_compare_color(clr, c, deviation))
                continue;
            bool result = true;
            for (int i = 0;  i < width; i++) {
                if (c != eb_image_get_pixel_color(src->data, src->channels, x + i, y, src->width, src->height)) {
                    result = false;
                    break;
                }
            }
            if (result) {
                pt->x = x;
                pt->y = y;
                return true;
            }
        }
    }
    return false;
}

void eb_image_binarize(eb_image* image, COLORREF range_begin, COLORREF range_end)
{
    assert(image != NULL && image->data != NULL);

    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            COLORREF clr = eb_image_get_pixel_color(image->data, image->channels, x, y, image->width, image->height);
            if (eb_image_color_in_range(clr, range_begin, range_end))
                eb_image_set_pixel_color(image->data, image->channels, x, y, image->width, image->height, RGB(0, 0, 0));
            else
                eb_image_set_pixel_color(image->data, image->channels, x, y, image->width, image->height, RGB(255, 255, 255));
        }
    }
}

static COLORREF eb_image_get_pixel_color(uint8_t* data, int channel, int x, int y, int w, int h)
{
    assert(data != NULL && w > 0 && h > 0);
    assert(x >= 0 && x < w);
    assert(y >= 0 && y < h);

    BYTE red = data[(channel * w) * y + channel * x];
    BYTE green = data[(channel * w) * y + channel * x + 1];
    BYTE blue = data[(channel * w) * y + channel * x + 2];

    return RGB(red, green, blue);
}

static void eb_image_set_pixel_color(uint8_t* data, int channel, int x, int y, int w, int h, COLORREF clr)
{
    assert(data != NULL && w > 0 && h > 0);
    assert(x >= 0 && x < w);
    assert(y >= 0 && y < h);

    data[(channel * w) * y + channel * x] = GetRValue(clr);
    data[(channel * w) * y + channel * x + 1] = GetGValue(clr);
    data[(channel * w) * y + channel * x + 2] = GetBValue(clr);
}

static bool eb_image_compare_block(uint8_t* p, int cp, int wp, int hp, uint8_t* q, int cq, int wq, int hq, int x, int y, int deviation)
{
    assert(p != NULL && q != NULL && wp > 0 && hp > 0 && wq > 0 && hq > 0);

    // choose two point to fast filter
    COLORREF c1a = eb_image_get_pixel_color(p, cp, x + 0, y + 0, wp, hp);
    COLORREF c1b = eb_image_get_pixel_color(q, cq, 0, 0, wq, hq);
    if (!eb_image_compare_color(c1a, c1b, deviation))
        return false;

    COLORREF c2a = eb_image_get_pixel_color(p, cp, x + (wq / 2), y + (hq / 2), wp, hp);
    COLORREF c2b = eb_image_get_pixel_color(q, cq, wq / 2, hq / 2, wq, hq);
    if (!eb_image_compare_color(c2a, c2b, deviation))
        return false;

    bool result = true;
    for (int j = 0; j < hq; j++) {
        for (int i = 0; i < wq; i++) {
            COLORREF ca = eb_image_get_pixel_color(p, cp, x + i, y + j, wp, hp);
            COLORREF cb = eb_image_get_pixel_color(q, cq, i, j, wq, hq);
            if (!eb_image_compare_color(ca, cb, deviation)) {
                result = false;
                break;
            }
        }
    }

    return result;
}

static bool eb_image_compare_color(COLORREF ca, COLORREF cb, int deviation)
{
    return (abs(GetRValue(ca) - GetRValue(cb)) < deviation) && (abs(GetGValue(ca) - GetGValue(cb)) < deviation) && (abs(GetBValue(ca) - GetBValue(cb)) < deviation);
}

static bool eb_image_color_in_range(COLORREF clr, COLORREF range_begin, COLORREF range_end)
{
    int r = GetRValue(clr);
    int g = GetGValue(clr);
    int b = GetBValue(clr);
    return r >= GetRValue(range_begin) && r <= GetRValue(range_end) && g >= GetGValue(range_begin) && g <= GetGValue(range_end) && b >= GetBValue(range_begin) && b <= GetBValue(range_end);
}
