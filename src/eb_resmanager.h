#ifndef EVEBOT_RESMANAGER_H
#define EVEBOT_RESMANAGER_H

#include "eb_common.h"
#include "eb_res_defs.h"
#include "eb_image.h"

bool eb_resmanager_load(const char* images_conf_path, const char* layout_conf_path);
void eb_resmanager_clean();
eb_image* eb_resmanager_get_image(char* resid);
RECT eb_resmanager_get_layout(char* resid);
const char* eb_resmanager_get_alarm_sound_path();

#endif // EVEBOT_RESMANAGER_H
