#ifndef EVEBOT_TASK_H
#define EVEBOT_TASK_H

#include "eb_common.h"
#include "eb_game.h"

void eb_task_test(eb_game_instance instance);
void eb_task_autopilot(eb_game_instance instance);
void eb_task_automission_distribution(eb_game_instance instance);
void eb_task_dscan_watch(eb_game_instance instance);

#endif // EVEBOT_TASK_H
