#include "eb_game.h"
#include "eb_utils.h"
#include "eb_image.h"
#include "eb_resmanager.h"
#include "eb_opsimulator.h"
#include "eb_engine.h"

typedef struct eb_game_context_t
{
    HWND hwnd;
    char charname[32];
    eb_image* screenshot;
    RECT wndrect;
} eb_game_context;

static int s_retry_count = 3;

static RECT eb_game_get_rect(int x, int y, char* layout_id);
static bool eb_game_multi_search(const eb_image* src, const eb_image* target, RECT rc, POINT* pt);
static bool eb_game_open_agent_conversation(eb_game_context* game, eb_image* agent_portrait, RECT* conversation_area);
static bool eb_game_select_context_menu_item(eb_game_context* game, POINT pt, char* menu_layout_id, eb_image* item_image);
static bool eb_game_is_a_transport_mission(eb_game_context* game, RECT conversation_objective_area);
static bool eb_game_is_a_lowsec_mission(eb_game_context* game, RECT conversation_objective_area);

eb_game_instance eb_game_create_instance(HWND hwnd, const char* charname)
{
    eb_game_context* new_game = (eb_game_context*)malloc(sizeof(eb_game_context));
    if (new_game == NULL)
        return NULL;

    new_game->hwnd = hwnd;
    strncpy(new_game->charname, charname, 32);
    new_game->screenshot = eb_image_new(NULL);
    assert(new_game->screenshot != NULL);

    eb_image_capture_window(new_game->screenshot, new_game->hwnd);
    int w = new_game->screenshot->width;
    int h = new_game->screenshot->height;
    SetRect(&new_game->wndrect, 0, 0, w, h);

    return new_game;
}

void eb_game_destroy_instance(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    eb_image_delete(game->screenshot);
    game->screenshot = NULL;
    free(game);
}

const char* eb_game_charname(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    return game->charname;
}

bool eb_game_refresh_screenshot(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    return eb_image_capture_window(game->screenshot, game->hwnd);
}

bool eb_game_is_in_station(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT pt = { 0 };
    eb_image* station_service_title = eb_resmanager_get_image(EB_RII_STATION_SERVICE_TITLE);
    assert(station_service_title != NULL);
    RECT station_panel_services = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_STATION_PANEL_SERVICES);

    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, station_service_title, station_panel_services, &pt))
            return true;

        eb_engine_idle(500);
    }

    return false;
}

bool eb_game_undock(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT pt = { 0 };
    RECT station_panel_top = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_STATION_PANEL_TOP);

    // first, click panel
    pt.x = station_panel_top.left + 10;
    pt.y = station_panel_top.top + 10;
    eb_opsimulator_mouse_lclick(game->hwnd, pt);
    eb_engine_idle(500);

    // search undock buttion
    eb_image* undock_btn = eb_resmanager_get_image(EB_RII_UNDOCK_BTN);
    assert(undock_btn != NULL);
    bool result = false;
    for (int i = 0; i < s_retry_count * 2; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, undock_btn, station_panel_top, &pt)) {
            result = true;
            break;
        }
        eb_engine_idle(500);
    }
    if (!result)
        return false;

    // click undock button
    pt.x += 30;
    pt.y += 15;
    eb_opsimulator_mouse_lclick(game->hwnd, pt);

    // wait and check for 3 seconds
    eb_image* abort_undock_text = eb_resmanager_get_image(EB_RII_ABORT_UNDOCK_TEXT);
    assert(abort_undock_text != NULL);
    for (int i = 0; i < s_retry_count * 2; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, abort_undock_text, station_panel_top, &pt))
            return true;
        eb_engine_idle(500);
    }

    return false;
}

bool eb_game_have_set_waypoint(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT pt = { 0 };
    RECT route_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_ROUTE);
    RECT overview_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_OVERVIEW);

    eb_image* overview_title = eb_resmanager_get_image(EB_RII_OVERVIEW_TITLE);
    assert(overview_title != NULL);
    eb_image* waypoint_jump_text = eb_resmanager_get_image(EB_RII_WAYPOINT_JUMP_TEXT);
    assert(waypoint_jump_text != NULL);
    eb_image* waypoint_no_destination_text = eb_resmanager_get_image(EB_RII_WAYPOINT_NO_DESTINATION_TEXT);
    assert(waypoint_no_destination_text != NULL);

    eb_image* overview_stargate = eb_resmanager_get_image(EB_RII_OVERVIEW_WAYPOINT_STARGATE);
    assert(overview_stargate != NULL);
    eb_image* overview_station = eb_resmanager_get_image(EB_RII_OVERVIEW_WAYPOINT_STATION);
    assert(overview_station != NULL);

    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;

        if (eb_game_multi_search(game->screenshot, overview_title, overview_area, &pt)) {
            // in space
            RECT overview_icons_area = eb_game_get_rect(pt.x, pt.y, EB_RLI_OVERVIEW_ICON);

            if (eb_game_multi_search(game->screenshot, overview_stargate, overview_icons_area, &pt))
                return true;

            if (eb_game_multi_search(game->screenshot, overview_station, overview_icons_area, &pt))
                return true;
        }

        if (eb_game_multi_search(game->screenshot, waypoint_no_destination_text, route_area, &pt))
            return false;

        if (eb_game_multi_search(game->screenshot, waypoint_jump_text, route_area, &pt))
            return true;

        eb_engine_idle(500);
    }

    return false;
}

bool eb_game_is_in_space(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT pt = { 0 };
    eb_image* overview_title = eb_resmanager_get_image(EB_RII_OVERVIEW_TITLE);
    assert(overview_title != NULL);
    eb_image* hud_full_speed = eb_resmanager_get_image(EB_RII_HUD_FULL_SPEED);
    assert(hud_full_speed != NULL);

    RECT overview_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_OVERVIEW);
    RECT hud_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_HUD);
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;

        if (eb_game_multi_search(game->screenshot, overview_title, overview_area, &pt))
            return true;

        if (eb_game_multi_search(game->screenshot, hud_full_speed, hud_area, &pt))
            return true;

        eb_engine_idle(500);
    }

    return false;
}

bool eb_game_is_warping(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT pt = { 0 };
    eb_image* hud_warping = eb_resmanager_get_image(EB_RII_HUD_WARPING);
    assert(hud_warping != NULL);
    RECT hud_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_HUD);
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, hud_warping, hud_area, &pt))
            return true;

        eb_engine_idle(500);
    }

    return false;
}

bool eb_game_is_motionless(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT pt = { 0 };
    eb_image* hud_warping = eb_resmanager_get_image(EB_RII_HUD_MOTIONLESS);
    assert(hud_warping != NULL);
    RECT hud_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_HUD);
    if (!eb_image_capture_window(game->screenshot, game->hwnd))
        return false;
    if (eb_game_multi_search(game->screenshot, hud_warping, hud_area, &pt))
        return true;

    return false;
}

bool eb_game_jump_to_next_waypoint(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT pt = { 0 };
    eb_image* overview_title = eb_resmanager_get_image(EB_RII_OVERVIEW_TITLE);
    assert(overview_title != NULL);
    RECT overview_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_OVERVIEW);
    bool result = false;
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            break;

        if (eb_game_multi_search(game->screenshot, overview_title, overview_area, &pt)) {
            result = true;
            break;
        }
        eb_engine_idle(500);
    }
    if (!result)
        return false;

    RECT overview_icons_area = eb_game_get_rect(pt.x, pt.y, EB_RLI_OVERVIEW_ICON);

    POINT overview_title_pt = { pt.x + 30, pt.y + 5 };
    eb_opsimulator_mouse_lclick(game->hwnd, overview_title_pt);
    eb_engine_idle(500);

    eb_image* overview_stargate = eb_resmanager_get_image(EB_RII_OVERVIEW_WAYPOINT_STARGATE);
    assert(overview_stargate != NULL);
    eb_image* overview_station = eb_resmanager_get_image(EB_RII_OVERVIEW_WAYPOINT_STATION);
    assert(overview_station != NULL);
    result = false;
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, overview_stargate, overview_icons_area, &pt)) {
            result = true;
            break;
        }
        if (eb_game_multi_search(game->screenshot, overview_station, overview_icons_area, &pt)) {
            result = true;
            break;
        }
        eb_opsimulator_mouse_lclick(game->hwnd, overview_title_pt);
        eb_engine_idle(500);
    }
    if (!result)
        return false;

    pt.x += 20;
    pt.y += 5;
    eb_opsimulator_mouse_lclick(game->hwnd, pt);
    eb_engine_idle(500);

    eb_opsimulator_press_key(game->hwnd, 'D', false, false, false, 700);
    eb_engine_idle(1000);

    return true;
}

bool eb_game_accept_distribution_mission(eb_game_instance instance, int* mission_type)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);
    assert(mission_type != NULL);

    // open conversation dialog
    RECT conversation_area = { 0 };
    eb_image* agent_portrait = eb_resmanager_get_image(EB_RII_AGENT_PORTRAIT_DISTRIBUTION);
    assert(agent_portrait != NULL);
    if (!eb_game_open_agent_conversation(game, agent_portrait, &conversation_area))
        return false;

    // is open?
    POINT pt = { 0 };
    RECT agent_conversation_btns = eb_game_get_rect(conversation_area.left, conversation_area.top, EB_RLI_MISSION_CONVERSATION_BTNS);
    eb_image* view_mission_btn = eb_resmanager_get_image(EB_RII_MISSION_VIEW_BTN);
    assert(view_mission_btn != NULL);
    eb_image* request_mission_btn = eb_resmanager_get_image(EB_RII_MISSION_REQUEST_BTN);
    assert(request_mission_btn != NULL);
    eb_image* loyalty_rewards = eb_resmanager_get_image(EB_RII_MISSION_LOYALTY_REWARDS);
    assert(loyalty_rewards != NULL);
    bool result = false;
    for (int i = 0; i < s_retry_count * 2; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;

        if (eb_game_multi_search(game->screenshot, view_mission_btn, agent_conversation_btns, &pt)) {
            pt.x += 10;
            pt.y += 5;
            eb_opsimulator_mouse_lclick(game->hwnd, pt);
            eb_engine_idle(5000);
        }
        else if (eb_game_multi_search(game->screenshot, request_mission_btn, agent_conversation_btns, &pt)) {
            pt.x += 10;
            pt.y += 5;
            eb_opsimulator_mouse_lclick(game->hwnd, pt);
            eb_engine_idle(5000);
        }

        if (eb_game_multi_search(game->screenshot, loyalty_rewards, conversation_area, &pt)) {
            result = true;
            break;
        }
        eb_engine_idle(1000);
    }
    if (!result)
        return false;

    eb_engine_idle(1000);

    // is a transport mission?
    RECT agent_conversation_objective = eb_game_get_rect(conversation_area.left, conversation_area.top, EB_RLI_MISSION_CONVERSATION_OBJECTIVE);
    if (!eb_game_is_a_transport_mission(game, agent_conversation_objective)) {
        *mission_type = -1;
        return true;
    }

    // is a low-sec mission?
    if (eb_game_is_a_lowsec_mission(game, agent_conversation_objective)) {
        *mission_type = -1;
        return true;
    }

    // click accept button
    eb_image* mission_accept_btn = eb_resmanager_get_image(EB_RII_MISSION_ACCEPT_BTN);
    assert(mission_accept_btn != NULL);
    eb_image* mission_complete_btn = eb_resmanager_get_image(EB_RII_MISSION_COMPLETE_BTN);
    assert(mission_complete_btn != NULL);
    RECT conversation_btns_area = eb_game_get_rect(conversation_area.left, conversation_area.top, EB_RLI_MISSION_CONVERSATION_BTNS);
    result = false;
    for (int i = 0; i < s_retry_count * 2; i++) {
        pt.x = conversation_area.left + 10;
        pt.y = conversation_area.top + 5;
        eb_opsimulator_mouse_lclick(game->hwnd, pt);
        eb_engine_idle(500);
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            break;
        if (eb_game_multi_search(game->screenshot, mission_accept_btn, conversation_btns_area, &pt)) {
            pt.x += 10;
            pt.y += 5;
            eb_opsimulator_mouse_lclick(game->hwnd, pt);
            pt.y -= 100;
        }

        eb_engine_idle(1000);

        // look for complete button
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            break;
        if (eb_game_multi_search(game->screenshot, mission_complete_btn, conversation_btns_area, &pt)) {
            result = true;
            break;
        }
    }
    if (!result)
        return false;

    if (!eb_image_capture_window(game->screenshot, game->hwnd))
        return false;
    RECT pickup_area, dropoff_area;
    eb_image* pickup_location_text = eb_resmanager_get_image(EB_RII_MISSION_PICKUP_TEXT);
    assert(pickup_location_text != NULL);
    if (!eb_game_multi_search(game->screenshot, pickup_location_text, agent_conversation_objective, &pt))
        return false;
    SetRect(&pickup_area, pt.x - 80, pt.y - 2, pt.x + 300, pt.y + 21);
    eb_image* dropoff_text = eb_resmanager_get_image(EB_RII_MISSION_DROPOFF_TEXT);
    assert(dropoff_text != NULL);
    if (!eb_game_multi_search(game->screenshot, dropoff_text, agent_conversation_objective, &pt))
        return false;
    SetRect(&dropoff_area, pt.x - 80, pt.y - 2, pt.x + 300, pt.y + 21);

    // is the cargo here?
    eb_image* check_mark = eb_resmanager_get_image(EB_RII_MISSION_CHECK_MARK);
    assert(check_mark != NULL);
    if (eb_game_multi_search(game->screenshot, check_mark, pickup_area, &pt))
        *mission_type = 1;
    else
        *mission_type = 2;

    // set destination
    RECT location_area;
    if (*mission_type == 1)
        location_area = dropoff_area;
    else if (*mission_type == 2)
        location_area = pickup_area;
    else
        assert(false);
    if (eb_image_search_line(game->screenshot, RGB(247, 148, 29), 30, location_area, &pt, 5)) {
        pt.x += 20;
    }
    else {
        pt.x = location_area.left + 230;
        pt.y = location_area.top + 5;
    }

    eb_image* menu_set_destination = eb_resmanager_get_image(EB_RII_MENU_SET_DESTINATION);
    assert(menu_set_destination != NULL);
    result = false;
    for (int i = 0; i < s_retry_count; i++) {
        if (eb_game_select_context_menu_item(game, pt, EB_RLI_MENU_GENERAL, menu_set_destination)) {
            result = true;
            break;
        }
    }
    if (!result)
        return false;

    // check
    if (!eb_game_have_set_waypoint(instance))
        return false;

    return true;
}

bool eb_game_take_all_items(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    if (!eb_image_capture_window(game->screenshot, game->hwnd))
        return false;

    POINT pt = { 0 };
    eb_image* inventory_title = eb_resmanager_get_image(EB_RII_INVENTORY_TITLE);
    assert(inventory_title != NULL);
    RECT inventory_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_INVENTORY);
    if (!eb_game_multi_search(game->screenshot, inventory_title, inventory_area, &pt))
        return false;

    RECT inventory_left_area = eb_game_get_rect(pt.x, pt.y, EB_RLI_INVENTORY_LEFT);

    // item hangar
    POINT item_hangar_pt = { 0 };
    eb_image* inventory_item_hangar = eb_resmanager_get_image(EB_RII_INVENTORY_ITEM_HANGAR);
    assert(inventory_item_hangar != NULL);
    bool result = false;
    for (int i = 0; i < 3; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            break;
        if (eb_game_multi_search(game->screenshot, inventory_item_hangar, inventory_left_area, &item_hangar_pt)) {
            result = true;
            break;
        }
        eb_engine_idle(500);
    }
    if (!result)
        return false;

    // current hangar
    POINT current_ship_pt = { 0 };
    eb_image* inventory_current_ship = eb_resmanager_get_image(EB_RII_INVENTORY_CURRENT_SHIP);
    assert(inventory_current_ship != NULL);
    result = false;
    for (int i = 0; i < 3; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            break;
        if (eb_game_multi_search(game->screenshot, inventory_current_ship, inventory_left_area, &current_ship_pt)) {
            result = true;
            break;
        }
        eb_engine_idle(500);
    }
    if (!result)
        return false;

    item_hangar_pt.x += 5;
    item_hangar_pt.y += 5;
    eb_opsimulator_mouse_lclick(game->hwnd, item_hangar_pt);
    eb_engine_idle(500);

    eb_image* menu_select_all = eb_resmanager_get_image(EB_RII_MENU_SELECT_ALL);
    assert(menu_select_all != NULL);
    pt.x += 200;
    pt.y += 100;
    result = false;
    for (int i = 0; i < s_retry_count; i++) {
        if (eb_game_select_context_menu_item(game, pt, EB_RLI_MENU_GENERAL, menu_select_all)) {
            result = true;
            break;
        }
        eb_engine_idle(500);
    }
    if (!result)
        return false;

    current_ship_pt.x += 5;
    current_ship_pt.y += 5;
    eb_opsimulator_mouse_drag_and_drop(game->hwnd, pt, current_ship_pt);
    eb_engine_idle(1000);

    return true;
}

bool eb_game_set_return_waypoints(eb_game_instance instance, int mission_type)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    // open conversation dialog
    RECT conversation_area = { 0 };
    eb_image* agent_portrait = eb_resmanager_get_image(EB_RII_AGENT_PORTRAIT_DISTRIBUTION);
    assert(agent_portrait != NULL);
    if (!eb_game_open_agent_conversation(game, agent_portrait, &conversation_area))
        return false;

    eb_image* location_image = NULL;
    if (mission_type == 1)
        location_image = eb_resmanager_get_image(EB_RII_MISSION_PICKUP_TEXT);
    else if (mission_type == 2)
        location_image = eb_resmanager_get_image(EB_RII_MISSION_DROPOFF_TEXT);
    if (location_image == NULL)
        return false;

    POINT location_text_pt = { 0 };
    RECT agent_conversation_objective = eb_game_get_rect(conversation_area.left, conversation_area.top, EB_RLI_MISSION_CONVERSATION_OBJECTIVE);
    bool result = false;
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            break;
        if (eb_game_multi_search(game->screenshot, location_image, agent_conversation_objective, &location_text_pt)) {
            result = true;
            break;
        }
        eb_engine_idle(500);
    }
    if (!result)
        return false;

    POINT pt = { 0 };
    RECT location_area = { location_text_pt.x + 80, location_text_pt.y - 2, location_text_pt.x + 300, location_text_pt.y + 21 };
    if (eb_image_search_line(game->screenshot, RGB(247, 148, 29), 30, location_area, &pt, 5)) {
        pt.x += 20;
    }
    else {
        pt.x = location_text_pt.x + 150;
        pt.y = location_text_pt.y;
    }

    eb_image* menu_set_destination = eb_resmanager_get_image(EB_RII_MENU_SET_DESTINATION);
    assert(menu_set_destination != NULL);
    result = false;
    for (int i = 0; i < s_retry_count; i++) {
        if (eb_game_select_context_menu_item(game, pt, i == s_retry_count - 1 ? NULL : EB_RLI_MENU_GENERAL, menu_set_destination)) {
            result = true;
            break;
        }

        eb_engine_idle(500);
    }
    return result;
}

bool eb_game_complete_distribution_mission(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    // open conversation dialog
    RECT conversation_area = { 0 };
    eb_image* agent_portrait = eb_resmanager_get_image(EB_RII_AGENT_PORTRAIT_DISTRIBUTION);
    assert(agent_portrait != NULL);
    if (!eb_game_open_agent_conversation(game, agent_portrait, &conversation_area))
        return false;

    if (!eb_image_capture_window(game->screenshot, game->hwnd))
        return false;

    // click complete mission button and check
    POINT pt = { 0 };
    eb_image* mission_complete_btn = eb_resmanager_get_image(EB_RII_MISSION_COMPLETE_BTN);
    assert(mission_complete_btn != NULL);
    eb_image* mission_request_btn = eb_resmanager_get_image(EB_RII_MISSION_REQUEST_BTN);
    assert(mission_request_btn != NULL);
    RECT conversation_btns_area = eb_game_get_rect(conversation_area.left, conversation_area.top, EB_RLI_MISSION_CONVERSATION_BTNS);
    for (int i = 0; i < s_retry_count * 2; i++) {
        if (eb_game_multi_search(game->screenshot, mission_complete_btn, conversation_btns_area, &pt)) {
            pt.x += 10;
            pt.y += 5;
            eb_opsimulator_mouse_lclick(game->hwnd, pt);
            eb_engine_idle(500);
        }

        pt.x = conversation_area.left + 10;
        pt.y = conversation_area.top + 5;
        eb_opsimulator_mouse_lclick(game->hwnd, pt);
        eb_engine_idle(500);

        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            break;

        if (eb_game_multi_search(game->screenshot, mission_request_btn, conversation_btns_area, &pt)) {
            eb_image* mission_close_btn = eb_resmanager_get_image(EB_RII_MISSION_CLOSE_BTN);
            assert(mission_close_btn != NULL);
            if (eb_game_multi_search(game->screenshot, mission_close_btn, conversation_btns_area, &pt)) {
                pt.x += 10;
                pt.y += 5;
                eb_opsimulator_mouse_lclick(game->hwnd, pt);
                eb_engine_idle(1000);
            }

            return true;
        }

        eb_engine_idle(500);
    }

    return false;
}

bool eb_game_decline_distribution_mission(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    // open conversation dialog
    RECT conversation_area = { 0 };
    eb_image* agent_portrait = eb_resmanager_get_image(EB_RII_AGENT_PORTRAIT_DISTRIBUTION);
    assert(agent_portrait != NULL);
    if (!eb_game_open_agent_conversation(game, agent_portrait, &conversation_area))
        return false;

    POINT pt = { 0 };
    eb_image* request_mission_btn = eb_resmanager_get_image(EB_RII_MISSION_REQUEST_BTN);
    assert(request_mission_btn != NULL);
    eb_image* decline_btn = eb_resmanager_get_image(EB_RII_MISSION_DECLINE_BTN);
    assert(decline_btn != NULL);
    RECT conversation_btns_area = eb_game_get_rect(conversation_area.left, conversation_area.top, EB_RLI_MISSION_CONVERSATION_BTNS);
    for (int i = 0; i < s_retry_count * 2; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, request_mission_btn, conversation_btns_area, &pt))
            return true;
        if (eb_game_multi_search(game->screenshot, decline_btn, conversation_btns_area, &pt)) {
            pt.x += 10;
            pt.y += 5;
            eb_opsimulator_mouse_lclick(game->hwnd, pt);
            eb_engine_idle(5000);
        }

        eb_engine_idle(500);
    };

    return false;
}

bool eb_game_dscan_watch(eb_game_instance instance, int* alarm)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    // press 'V'
    eb_opsimulator_press_key(game->hwnd, 'V', false, false, false, 100);
    eb_engine_idle(500);

    POINT pt = { 0 };
    eb_image* dscan_title = eb_resmanager_get_image(EB_RII_DSCANNER_TITLE);
    assert(dscan_title != NULL);

    if (!eb_image_capture_window(game->screenshot, game->hwnd))
        return false;

    if (!eb_game_multi_search(game->screenshot, dscan_title, game->wndrect, &pt))
        return false;

    RECT dscan_result_area = eb_game_get_rect(pt.x, pt.y, EB_RLI_DSCAN_RESULT);
    eb_image* icon_frigate = eb_resmanager_get_image(EB_RII_ICON_FRIGATE);
    assert(icon_frigate != NULL);
    eb_image* icon_destroyer = eb_resmanager_get_image(EB_RII_ICON_DESTROYER);
    assert(icon_destroyer != NULL);

    bool frigate_found = false;
    bool destroyer_found = false;
    for (int i = 0; i < s_retry_count; i++) {
        if (eb_game_multi_search(game->screenshot, icon_frigate, dscan_result_area, &pt)) {
            frigate_found = true;
            break;
        }
        if (eb_game_multi_search(game->screenshot, icon_destroyer, dscan_result_area, &pt)) {
            destroyer_found = true;
            break;
        }

        eb_engine_idle(100);
    };

    *alarm = (frigate_found || destroyer_found);

    return true;
}

void eb_game_alarm_sound()
{
    wchar_t* filepath = eb_utils_str_u8tow(eb_resmanager_get_alarm_sound_path(), -1);
    PlaySoundW(filepath, NULL, SND_FILENAME | SND_ASYNC);
    eb_utils_str_free(filepath);
}

void eb_game_move_camera_slightly(eb_game_instance instance)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL);

    POINT src = { game->wndrect.right / 2 - 10, 1 };
    POINT dest = { game->wndrect.right / 2 + 10, 1 };
    eb_opsimulator_mouse_drag_and_drop(game->hwnd, src, dest);
}

bool eb_game_save_screenshot(eb_game_instance instance, const char* filepath)
{
    eb_game_context* game = (eb_game_context*)instance;
    assert(game != NULL && game->screenshot != NULL);

    if (game->screenshot->data == NULL || game->screenshot->width < 0 || game->screenshot->height < 0)
        return false;

    return eb_image_save_to_file(game->screenshot, filepath);
}

static RECT eb_game_get_rect(int x, int y, char* layout_id)
{
    RECT rc = eb_resmanager_get_layout(layout_id);
    OffsetRect(&rc, x, y);
    return rc;
}

static bool eb_game_multi_search(const eb_image* src, const eb_image* target, RECT rc, POINT* pt)
{
    int tru_count = 4;
    int deviation = 8;

    bool result = false;
    for (int i = 0; i < tru_count; i++) {
        if (eb_image_search(src, target, rc, pt, deviation)) {
            result = true;
            break;
        }
        else {
            deviation *= 2; // 8->16->32->64
        }
    }
    return result;
}

static bool eb_game_open_agent_conversation(eb_game_context* game, eb_image* agent_portrait, RECT* conversation_area)
{
    assert(game != NULL && agent_portrait != NULL && conversation_area != NULL);

    POINT pt = { 0 };
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;

        eb_image* agent_conversation_title = eb_resmanager_get_image(EB_RII_AGENT_CONVERSATION_TITLE);
        assert(agent_conversation_title != NULL);
        if (eb_game_multi_search(game->screenshot, agent_conversation_title, game->wndrect, &pt)) {
            *conversation_area = eb_game_get_rect(pt.x, pt.y, EB_RLI_MISSION_CONVERSATION);

            pt.x += 20;
            pt.y += 5;
            eb_opsimulator_mouse_lclick(game->hwnd, pt);
            eb_engine_idle(500);

            if (!eb_image_capture_window(game->screenshot, game->hwnd))
                return false;

            eb_image* division_distribution = eb_resmanager_get_image(EB_RII_MISSION_DIVISION_TEXT);
            assert(division_distribution != NULL);
            RECT mission_conversation_agent = eb_game_get_rect(conversation_area->left, conversation_area->top, EB_RLI_MISSION_CONVERSATION_AGENT);
            if (eb_game_multi_search(game->screenshot, division_distribution, mission_conversation_agent, &pt))
                return true;
        }

        RECT station_agents_area = eb_game_get_rect(game->wndrect.left, game->wndrect.top, EB_RLI_STATION_PANEL_AGENTS);
        if (eb_game_multi_search(game->screenshot, agent_portrait, station_agents_area, &pt)) {
            pt.x += 198;
            pt.y += 40;
            eb_opsimulator_mouse_dblclick(game->hwnd, pt);
            eb_opsimulator_mouse_dblclick(game->hwnd, pt);
            eb_engine_idle(1000);
        }
        else {
            eb_engine_idle(500);
        }
    }

    return false;
}

static bool eb_game_select_context_menu_item(eb_game_context* game, POINT pt, char* menu_layout_id, eb_image* item_image)
{
    assert(game != NULL && item_image != NULL);

    eb_opsimulator_mouse_rclick(game->hwnd, pt);
    eb_engine_idle(1000);

    if (!eb_image_capture_window(game->screenshot, game->hwnd))
        return false;

    POINT item_pt = { 0 };
    RECT menu_area = menu_layout_id == NULL ? game->wndrect : eb_game_get_rect(pt.x, pt.y, menu_layout_id); // -1: workaround
    if (eb_game_multi_search(game->screenshot, item_image, menu_area, &item_pt)) {
        item_pt.x += (item_image->width / 2);
        item_pt.y += (item_image->height / 2);
        eb_opsimulator_mouse_lclick(game->hwnd, item_pt);
        eb_engine_idle(500);
        return true;
    }

    return false;
}

static bool eb_game_is_a_transport_mission(eb_game_context* game, RECT conversation_objective_area)
{
    assert(game != NULL);

    POINT pt = { 0 };
    eb_image* transport_goods = eb_resmanager_get_image(EB_RII_MISSION_TRANSPORT_GOODS);
    assert(transport_goods != NULL);
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, transport_goods, conversation_objective_area, &pt)) {
            eb_image* pickup_location_text = eb_resmanager_get_image(EB_RII_MISSION_PICKUP_TEXT);
            assert(pickup_location_text != NULL);
            if (eb_game_multi_search(game->screenshot, pickup_location_text, conversation_objective_area, &pt))
                return true;
        }

        eb_engine_idle(500);
    }

    return false;
}

static bool eb_game_is_a_lowsec_mission(eb_game_context* game, RECT conversation_objective_area)
{
    assert(game != NULL);

    POINT pt = { 0 };
    eb_image* low_sec_text = eb_resmanager_get_image(EB_RII_MISSION_LOWSEC_WARNING);
    assert(low_sec_text != NULL);
    for (int i = 0; i < s_retry_count; i++) {
        if (!eb_image_capture_window(game->screenshot, game->hwnd))
            return false;
        if (eb_game_multi_search(game->screenshot, low_sec_text, conversation_objective_area, &pt))
            return true;

        eb_engine_idle(500);
    }

    return false;
}
