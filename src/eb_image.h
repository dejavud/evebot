#ifndef EVEBOT_IMAGE_H
#define EVEBOT_IMAGE_H

#include "eb_common.h"

typedef struct eb_image_t
{
    uint8_t* data;

    int width;
    int height;
    int channels;
    bool load_from_file; // otherwise from capture
    
    void* user_data;
} eb_image;

eb_image* eb_image_new(void* user_data);
void eb_image_delete(eb_image* image);
bool eb_image_is_empty(eb_image* image);

bool eb_image_load_from_file(eb_image* image, const char* filepath);
bool eb_image_save_to_file(eb_image* image, const char* filepath);

bool eb_image_capture_window(eb_image* image, HWND hwnd);

bool eb_image_search(const eb_image* src, const eb_image* target, RECT rc, POINT* pt, int deviation);
bool eb_image_search_entire(const eb_image* src, const eb_image* target, POINT* pt, int deviation);
bool eb_image_search_line(const eb_image* src, COLORREF clr, int width, RECT rc, POINT* pt, int deviation);

void eb_image_binarize(eb_image* image, COLORREF range_begin, COLORREF range_end);


#endif // EVEBOT_IMAGE_H
