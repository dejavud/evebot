#include "eb_config.h"
#include "eb_utils.h"
#include "eb_log.h"
#include "sxmlc/sxmlc.h"

#define DEFAULT_CONFIG_PATH "config/config.xml"

#define TAG_CONFIG          "config"
#define TAG_CHARNAME        "charname"
#define TAG_MODE            "mode"
#define TAG_IMAGES_CONFIG   "images_conf"
#define TAG_LAYOUT_CONFIG   "layout_conf"

#define ATTR_NAME           "name"
#define ATTR_MODE           "mode"
#define ATTR_PATH           "path"

static void eb_config_get_path(int argc, char** argv, char* path, int len);
static bool eb_config_load_from_file(const char* config_path, eb_config* config);
static bool eb_config_load_from_cmdline(int argc, char** argv, eb_config* config);
static void eb_config_cmdline_usage();

bool eb_config_load(int argc, char** argv, eb_config* config)
{
    assert(config != NULL);

    char config_path[MAX_PATH] = { 0 };
    eb_config_get_path(argc, argv, config_path, MAX_PATH);

    if (!eb_config_load_from_file(config_path, config)) {
        LOGE("Load config file failed.\n");
        return false;
    }

    if (!eb_config_load_from_cmdline(argc, argv, config)) {
        eb_config_cmdline_usage();
        return false;
    }

    return true;
}

static void eb_config_get_path(int argc, char** argv, char* path, int len)
{
    assert(argv != NULL && *argv != NULL);

    strncpy(path, DEFAULT_CONFIG_PATH, MAX_PATH);

    for (int i = 0; i < argc; i++) {
        const char* p = argv[i];
        if (p[0] != '-')
            continue;
        if (p[1] == 'c' && p[2] == '\0') {
            i++;
            char* tmp = eb_utils_str_atou8(argv[i], -1);
            strncpy(path, tmp, len);
            eb_utils_str_free(tmp);
        }
    }
}

static bool eb_config_load_from_file(const char* config_path, eb_config* config)
{
    assert(config_path != NULL && config != NULL);

    bool result = false;

    char* config_path_a = eb_utils_str_u8toa(config_path, -1);

    XMLDoc doc;
    XMLDoc_init(&doc);

    do {
        if (!XMLDoc_parse_file_DOM(config_path_a, &doc))
            break;

        XMLNode* root = XMLDoc_root(&doc);
        if (strcmp(root->tag, TAG_CONFIG) != 0)  // sanity check
            break;

        int count = XMLNode_get_children_count(root);
        if (count <= 0)
            break;

        for (int i = 0; i < count; i++) {
            XMLNode* node = XMLNode_get_child(root, i);
            if (node == NULL)
                continue;

            if (strcmp(node->tag, TAG_CHARNAME) == 0) {
                for (int j = 0; j < node->n_attributes; j++) {
                    if (strcmp(node->attributes[j].name, ATTR_NAME) == 0) {
                        strncpy(config->charname, node->attributes[j].value, 32);
                    }
                }
            }
            else if (strcmp(node->tag, TAG_MODE) == 0) {
                for (int j = 0; j < node->n_attributes; j++) {
                    if (strcmp(node->attributes[j].name, ATTR_MODE) == 0) {
                        char* mode = node->attributes[j].value;
                        if (strcmp(mode, "test") == 0)
                            config->mode = EB_MODE_TEST;
                        else if (strcmp(mode, "autopilot") == 0)
                            config->mode = EB_MODE_AUTOPILOT;
                        else if (strcmp(mode, "mission") == 0)
                            config->mode = EB_MODE_MISSION;
                        else if (strcmp(mode, "dscan") == 0)
                            config->mode = EB_MODE_DSCAN;
                        else
                            config->mode = EB_MODE_NONE;
                    }
                }
            }
            else if (strcmp(node->tag, TAG_IMAGES_CONFIG) == 0) {
                for (int j = 0; j < node->n_attributes; j++) {
                    if (strcmp(node->attributes[j].name, ATTR_PATH) == 0) {
                        strncpy(config->images_conf_path, node->attributes[j].value, MAX_PATH);
                    }
                }
            }
            else if (strcmp(node->tag, TAG_LAYOUT_CONFIG) == 0) {
                for (int j = 0; j < node->n_attributes; j++) {
                    if (strcmp(node->attributes[j].name, ATTR_PATH) == 0) {
                        strncpy(config->layout_conf_path, node->attributes[j].value, MAX_PATH);
                    }
                }
            }
        }

        result = true;
    } while (false);

    XMLDoc_free(&doc);
    eb_utils_str_free(config_path_a);

    return result;
}

static bool eb_config_load_from_cmdline(int argc, char** argv, eb_config* config)
{
    assert(argv != NULL && *argv != NULL && config != NULL);

    bool result = true;

    for (int i = 0; i < argc; i++) {
        const char* p = argv[i];
        if (p[0] != '-' && p[2] != '\0')
            continue;
        switch (p[1]) {
        case 'a':
            config->mode = EB_MODE_AUTOPILOT;
            break;
        case 'd':
            config->mode = EB_MODE_DSCAN;
            break;
        case 'm':
            config->mode = EB_MODE_MISSION;
            break;
        case 's':
            config->shutdown_system = true;
            break;
        default:
            result = false;
            break;
        }
    }

    return result;
}

static void eb_config_cmdline_usage()
{
    LOGI("Usage: evebot [OPTION]...\n");
    LOGI("Options:\n");
    LOGI("-c  specify the config file path\n");
    LOGI("-s  shutdown system after bot exit\n");
    // TODO
}
