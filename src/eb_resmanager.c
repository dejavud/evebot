#include "eb_resmanager.h"
#include "eb_utils.h"
#include "c_hashmap/hashmap.h"
#include "sxmlc/sxmlc.h"

#define TAG_IMAGES      "images"
#define TAG_IMG         "img"
#define TAG_LAYOUT      "layout"
#define TAG_AREA        "area"

#define ATTR_ID         "id"
#define ATTR_FILENAME   "filename"
#define ATTR_RECT       "rect"

#define DEFAULT_IMAGE_DIR   "images"

#define ALARM_SOUND_PATH    "sounds/alarm.wav"
static char* s_alarm_sound_path;

typedef struct eb_area_t
{
    char* id;
    RECT rc;
} eb_area;

map_t s_images_cache;
map_t s_layout_cache;

static bool eb_resmanager_load_images(const char* images_conf_path);
static bool eb_resmanager_load_layout(const char* layout_conf_path);
static int eb_clean_images_cache(any_t item, any_t data);
static int eb_clean_layout_cache(any_t item, any_t data);

bool eb_resmanager_load(const char* images_conf_path, const char* layout_conf_path)
{
    s_images_cache = hashmap_new();
    if (!eb_resmanager_load_images(images_conf_path))
        return false;

    s_layout_cache = hashmap_new();
    if (!eb_resmanager_load_layout(layout_conf_path))
        return false;

    s_alarm_sound_path = (char*)malloc(MAX_PATH);
    snprintf(s_alarm_sound_path, MAX_PATH, "%s\\%s", eb_utils_get_app_dir(), ALARM_SOUND_PATH);

    return true;
}

void eb_resmanager_clean()
{
    if (s_images_cache != NULL) {
        hashmap_iterate(s_images_cache, eb_clean_images_cache, NULL);
        hashmap_free(s_images_cache);
    }

    if (s_layout_cache != NULL) {
        hashmap_iterate(s_layout_cache, eb_clean_layout_cache, NULL);
        hashmap_free(s_layout_cache);
    }

    free(s_alarm_sound_path);
    s_alarm_sound_path = NULL;
}

eb_image* eb_resmanager_get_image(char* resid)
{
    assert(s_images_cache != NULL);

    eb_image* image = NULL;
    hashmap_get(s_images_cache, resid, &image);
    return image;
}

RECT eb_resmanager_get_layout(char* resid)
{
    assert(s_layout_cache != NULL);

    eb_area* area = NULL;
    hashmap_get(s_layout_cache, resid, &area);
    assert(area != NULL);
    return area->rc;
}

const char* eb_resmanager_get_alarm_sound_path()
{
    assert(s_alarm_sound_path != NULL);
    return s_alarm_sound_path;
}

static bool eb_resmanager_load_images(const char* images_conf_path)
{
    assert(images_conf_path != NULL);

    bool result = false;

    char* images_conf_path_a = eb_utils_str_u8toa(images_conf_path, -1);

    XMLDoc doc;
    XMLDoc_init(&doc);

    do {
        if (!XMLDoc_parse_file_DOM(images_conf_path_a, &doc))
            break;

        XMLNode* root = XMLDoc_root(&doc);
        if (strcmp(root->tag, TAG_IMAGES) != 0)
            break;

        int count = XMLNode_get_children_count(root);
        if (count <= 0)
            break;

        for (int i = 0; i < count; i++) {
            XMLNode* node = XMLNode_get_child(root, i);
            if (node == NULL)
                continue;

            if (strcmp(node->tag, TAG_IMG) == 0) {
                char* id = NULL;
                char* filename = NULL;
                XMLNode_get_attribute(node, ATTR_ID, &id);
                XMLNode_get_attribute(node, ATTR_FILENAME, &filename);

                eb_image* image = eb_image_new(id);

                char filepath[MAX_PATH] = { 0 };
                snprintf(filepath, MAX_PATH, "%s\\%s", DEFAULT_IMAGE_DIR, filename);
                if (!eb_image_load_from_file(image, filepath)) {
                    __free(id);
                    __free(filename);
                    eb_image_delete(image);
                    continue;
                }
                __free(filename);

                hashmap_put(s_images_cache, id, image);
            }
        }

        result = true;
    } while (false);

    XMLDoc_free(&doc);
    eb_utils_str_free(images_conf_path_a);

    return result;
}

static bool eb_resmanager_load_layout(const char* layout_conf_path)
{
    assert(layout_conf_path != NULL);

    bool result = false;

    char* layout_conf_path_a = eb_utils_str_u8toa(layout_conf_path, -1);

    XMLDoc doc;
    XMLDoc_init(&doc);
    do {
        if (!XMLDoc_parse_file_DOM(layout_conf_path_a, &doc))
            break;

        XMLNode* root = XMLDoc_root(&doc);
        if (strcmp(root->tag, TAG_LAYOUT) != 0)
            break;

        int count = XMLNode_get_children_count(root);
        if (count <= 0)
            break;

        for (int i = 0; i < count; i++) {
            XMLNode* node = XMLNode_get_child(root, i);
            if (node == NULL)
                continue;

            if (strcmp(node->tag, TAG_AREA) == 0) {
                char* id = NULL;
                char* rect = NULL;
                XMLNode_get_attribute(node, ATTR_ID, &id);
                XMLNode_get_attribute(node, ATTR_RECT, &rect);

                RECT rc;
                if (sscanf(rect, "%d,%d,%d,%d", &rc.left, &rc.top, &rc.right, &rc.bottom) != 4)
                    continue;

                __free(rect);

                eb_area* area = (eb_area*)malloc(sizeof(eb_area));
                area->id = id;
                area->rc = rc;

                hashmap_put(s_layout_cache, id, area);
            }
        }

        result = true;
    } while (false);

    XMLDoc_free(&doc);
    eb_utils_str_free(layout_conf_path_a);

    return result;
}

static int eb_clean_images_cache(any_t item, any_t data)
{
    eb_image* image = (eb_image*)data;
    assert(image != NULL);

    assert(image->user_data != NULL);
    __free(image->user_data);
    eb_image_delete(image);

    return MAP_OK;
}

static int eb_clean_layout_cache(any_t item, any_t data)
{
    eb_area* area = (eb_area*)data;
    assert(area != NULL);

    assert(area->id != NULL);
    __free(area->id);
    free(area);

    return MAP_OK;
}
