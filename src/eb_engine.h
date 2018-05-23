#ifndef EVEBOT_ENGINE_H
#define EVEBOT_ENGINE_H

#include "eb_common.h"
#include "eb_config.h"

bool eb_engine_init(eb_config* config);
void eb_engine_shutdown();
bool eb_engine_launch();

bool eb_engine_idle(int milliseconds);
bool eb_engine_check_exit();

#endif // EVEBOT_ENGINE_H
