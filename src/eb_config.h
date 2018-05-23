#ifndef EVEBOT_CONFIG_H
#define EVEBOT_CONFIG_H

#include "eb_common.h"

typedef enum eb_mode_t
{
    EB_MODE_NONE = 0,
    EB_MODE_TEST,
    EB_MODE_AUTOPILOT,
    EB_MODE_MISSION,
    EB_MODE_DSCAN,
} eb_mode;

typedef struct eb_config_t
{
    char charname[32];
    eb_mode mode;
    char images_conf_path[MAX_PATH];
    char layout_conf_path[MAX_PATH];
    bool shutdown_system;

} eb_config;

bool eb_config_load(int argc, char** argv, eb_config* config);

#endif // EVEBOT_CONFIG_H
