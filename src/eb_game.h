#ifndef EVEBOT_GAME_H
#define EVEBOT_GAME_H

#include "eb_common.h"

typedef void* eb_game_instance;

eb_game_instance eb_game_create_instance(HWND hwnd, const char* charname);
void eb_game_destroy_instance(eb_game_instance instance);

const char* eb_game_charname(eb_game_instance instance);

bool eb_game_refresh_screenshot(eb_game_instance instance);

bool eb_game_is_in_station(eb_game_instance instance);
bool eb_game_undock(eb_game_instance instance);
bool eb_game_have_set_waypoint(eb_game_instance instance);
bool eb_game_is_in_space(eb_game_instance instance);
bool eb_game_is_warping(eb_game_instance instance);
bool eb_game_is_motionless(eb_game_instance instance);
bool eb_game_jump_to_next_waypoint(eb_game_instance instance);

bool eb_game_accept_distribution_mission(eb_game_instance instance, int* mission_type);
bool eb_game_take_all_items(eb_game_instance instance);
bool eb_game_set_return_waypoints(eb_game_instance instance, int mission_type);
bool eb_game_complete_distribution_mission(eb_game_instance instance);
bool eb_game_decline_distribution_mission(eb_game_instance instance);

bool eb_game_dscan_watch(eb_game_instance instance, int* alarm);
void eb_game_alarm_sound();

void eb_game_move_camera_slightly(eb_game_instance instance);
bool eb_game_save_screenshot(eb_game_instance instance, const char* filepath);

#endif // EVEBOT_GAME_H
