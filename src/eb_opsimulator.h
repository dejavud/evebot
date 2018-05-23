#ifndef EVEBOT_OPSIMULATOR_H
#define EVEBOT_OPSIMULATOR_H

#include "eb_common.h"

void eb_opsimulator_mouse_move(HWND hwnd, POINT pt);
void eb_opsimulator_mouse_lclick(HWND hwnd, POINT pt);
void eb_opsimulator_mouse_rclick(HWND hwnd, POINT pt);
void eb_opsimulator_mouse_dblclick(HWND hwnd, POINT pt);
void eb_opsimulator_mouse_drag_and_drop(HWND hwnd, POINT src, POINT dest);

void eb_opsimulator_press_key(HWND hwnd, UINT vkCode, bool ctrl, bool shift, bool alt, int hold_time);

#endif // EVEBOT_OPSIMULATOR_H
