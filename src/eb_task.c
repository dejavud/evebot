#include "eb_task.h"
#include "eb_utils.h"
#include "eb_log.h"
#include "eb_engine.h"

#define DEFAULT_SCREENSHOTS_DIR "screenshots"

static void eb_task_routine_save_screenshot(eb_game_instance instance);
static bool eb_task_routine_autopilot(eb_game_instance instance);
static bool eb_task_routine_automission_distribution(eb_game_instance instance);
static bool eb_task_routine_dscan_watch(eb_game_instance instance);

void eb_task_test(eb_game_instance instance)
{
    EB_PERFORMANCE_TEST_BEGIN;
    eb_task_routine_save_screenshot(instance);
    EB_PERFORMANCE_TEST_END("test");
}

void eb_task_autopilot(eb_game_instance instance)
{
    EB_PERFORMANCE_TEST_BEGIN;

    if (eb_task_routine_autopilot(instance)) {
        LOGI("Exit.\n");
    }
    else {
        eb_task_routine_save_screenshot(instance);
        LOGI("Abort.\n");
    }

    EB_PERFORMANCE_TEST_END("autopilot");
}

void eb_task_automission_distribution(eb_game_instance instance)
{
    EB_PERFORMANCE_TEST_BEGIN;

    if (eb_task_routine_automission_distribution(instance)) {
        LOGI("Exit.\n");
    }
    else {
        eb_task_routine_save_screenshot(instance);
        LOGI("Abort.\n");
    }

    EB_PERFORMANCE_TEST_END(_T("automission-distribution"));
}

void eb_task_dscan_watch(eb_game_instance instance)
{
    EB_PERFORMANCE_TEST_BEGIN;

    if (eb_task_routine_dscan_watch(instance)) {
        LOGI("Exit.\n");
    }
    else {
        eb_task_routine_save_screenshot(instance);
        LOGI("Abort.\n");
    }
        
    EB_PERFORMANCE_TEST_END(_T("dscan_watch"));
}

static void eb_task_routine_save_screenshot(eb_game_instance instance)
{
    char dir[MAX_PATH] = { 0 };
    snprintf(dir, MAX_PATH, "%s\\%s", eb_utils_get_app_dir(), DEFAULT_SCREENSHOTS_DIR);
    wchar_t* dirw = eb_utils_str_u8tow(dir, -1);
    CreateDirectoryW(dirw, NULL);  // create dir if not exists
    eb_utils_str_free(dirw);

    char filepath[MAX_PATH] = { 0 };
    snprintf(filepath, MAX_PATH, "%s\\%llu.bmp", dir, time(NULL));
    if (eb_game_save_screenshot(instance, filepath))
        LOGI("Save screenshot to file %s\n", filepath);
    else
        LOGI("Save screenshot failed.\n");
}

static bool eb_task_routine_autopilot(eb_game_instance instance)
{
    // check if we have some waypoints
    if (!eb_game_have_set_waypoint(instance)) {
        LOGI("There is no waypoint.\n");
        return false;
    }

    // undock if we are in station
    if (eb_game_is_in_station(instance)) {
        LOGI("[In Station]\n");

        bool undock_result = false;
        for (int i = 0; i < 5; i++ ) { // try 5 times
            if (eb_game_undock(instance)) {
                undock_result = true;
                LOGI("Undocking...\n");
                break;
            }
        }
        if (!undock_result) {
            LOGI("Undock failed.\n");
            //return false;
        }
    }

    eb_engine_idle(5000);

    // check we are in space
    bool is_in_space = false;
    for (int i = 0; i < 10; i++) {
        if (eb_game_is_in_space(instance)) {
            is_in_space = true;
            break;
        }
        eb_engine_idle(1000);
    }
    if (!is_in_space) {
        LOGI("We are not in space.\n");
        return false;
    }
    LOGI("[In Space]\n");

    // jump to the next waypoint
    int jump_count = 0;
    int jump_failure_count = 0;
    bool is_jumping = false;
    while (!eb_engine_check_exit()) {
        if (eb_game_is_warping(instance)) {
            eb_engine_idle(1000);
            continue;
        }

        if (jump_count > 1 && is_jumping && eb_game_is_in_space(instance) && !eb_game_is_motionless(instance)) {
            eb_engine_idle(1000);
            continue;
        }

        if (eb_game_jump_to_next_waypoint(instance)) {
            LOGI("Jumping...\n");
            is_jumping = true;
            ++jump_count;
            jump_failure_count = 0; // reset
            eb_engine_idle(5000);
        }
        else {
            ++jump_failure_count;
            if (jump_failure_count > 3) {
                if (!eb_game_have_set_waypoint(instance))
                    break;
                LOGI("Jump failed.\n");
                return false;
            }
            //eb_game_move_camera_slightly(instance);
            eb_engine_idle(1000);
            continue;
        }
    }

    LOGI("All waypoints readched.\n");
    return true;
}

static bool eb_task_routine_automission_distribution(eb_game_instance instance)
{
    if (!eb_game_is_in_station(instance)) {
        LOGI("We are not in station.\n");
        return false;
    }
    LOGI("[In Station]\n");

    int count = 0;
    int mission_type = 0;
    DWORD time_start, time_end;
    int retry_count = 0;
    int step = 0;
    while (!eb_engine_check_exit()) {
        bool move_camera = false;
        switch (step) {
        case 0:
            step = 0;
            if (!eb_game_accept_distribution_mission(instance, &mission_type)) {
                LOGI("[Step 0] Accept mission failed.\n");
                move_camera = true;
                break;
            }
        case 1:
            step = 1;
            if (mission_type == -1) {
                LOGI("[Step 1] This mission is not incompatible. Decline it.\n");

                if (!eb_game_decline_distribution_mission(instance))
                    LOGI("[Step 1] Decline mission failed.\n");
                else
                    step = 0; // back 
                break;
            }
            else {
                LOGI("[Step 1] Mission accepted. ");
                time_start = GetTickCount();
                if (mission_type == 1)
                    LOGI("This is a delivery mission.\n");
                else if (mission_type == 2)
                    LOGI("This is a retrieve mission.\n");
            }
        case 2:
            step = 2;
            if (mission_type == 1) {
                if (eb_game_take_all_items(instance)) {
                    LOGI("[Step 2] Take the cargos.\n");
                    eb_engine_idle(1000);
                }
                else {
                    LOGI("[Step 2] Take item failed.\n");
                    break;
                }
            }
            else if (mission_type == 2) {
                LOGI("[Step 2] Start autopilot.\n");
                if (!eb_task_routine_autopilot(instance)) {
                    LOGI("[Step 2] Autopilot failed.\n");
                    move_camera = true;
                    break;
                }
            }
        case 3:
            step = 3;
            if (mission_type == 1) {
                LOGI("[Step 3] Start autopilot.\n");
                if (!eb_task_routine_autopilot(instance)) {
                    LOGI("[Step 3] Autopilot failed.\n");
                    move_camera = true;
                    break;
                }
            }
            else if (mission_type == 2) {
                if (eb_game_set_return_waypoints(instance, mission_type)) {
                    LOGI("[Step 3] Set return waypoints.\n");
                }
                else {
                    LOGI("[Step 3] Set return waypoints failed.\n");
                    break;
                }
            }
        case 4:
            step = 4;
            if (mission_type == 1) {
                if (eb_game_set_return_waypoints(instance, mission_type)) {
                    LOGI("[Step 4] Set return waypoints.\n");
                }
                else {
                    LOGI("[Step 4] Set return waypoints failed.\n");
                    break;
                }
            }
            else if (mission_type == 2) {
                if (eb_game_take_all_items(instance)) {
                    LOGI("[Step 4] Take the cargos.\n");
                }
                else {
                    LOGI("[Step 4] Take item failed.\n");
                    break;
                }
            }
        case 5:
            step = 5;
            if (mission_type == 1) {
                if (eb_game_complete_distribution_mission(instance)) {
                    LOGI("[Step 5] Mission completed.\n");
                }
                else {
                    LOGI("[Step 5] Mission complete failed.\n");
                    break;
                }
            }
            else if (mission_type == 2) {
                LOGI("[Step 5] Start autopilot.\n");
                if (!eb_task_routine_autopilot(instance)) {
                    
                }
            }
        case 6:
            step = 6;
            if (mission_type == 1) {
                LOGI("[Step 6] Start autopilot.\n");
                if (!eb_task_routine_autopilot(instance)) {
                    LOGI("[Step 6] Autopilot failed.\n");
                    move_camera = true;
                    break;
                }
            }
            else if (mission_type == 2) {
                if (eb_game_complete_distribution_mission(instance)) {
                    LOGI("[Step 6] Mission completed.\n");
                }
                else {
                    LOGI("[Step 6] Mission complete failed.\n");
                    break;
                }
            }
        case 7:
            count++;
            time_end = GetTickCount();
            LOGI("======== Mission %02d, Elapsed Time: %.2f ========\n", count, (time_end - time_start) / 1000.0f);
            mission_type = 0;
            step = 0;
            retry_count = 0;
            break;
        }

        if (retry_count++ == 10)
            return false;

        if (count == 32)
            break;

        if (move_camera) {
            eb_game_move_camera_slightly(instance);
            eb_engine_idle(500);
        }
    }

    LOGI("Finished %d distribution missions.\n", count);
    return true;
}

static bool eb_task_routine_dscan_watch(eb_game_instance instance)
{
    // check we are in space
    bool is_in_space = false;
    for (int i = 0; i < 10; i++) {
        if (eb_game_is_in_space(instance)) {
            is_in_space = true;
            break;
        }
        eb_engine_idle(1000);
    }
    if (!is_in_space) {
        LOGI("We are not in space.\n");
        return false;
    }
    LOGI("[In Space]\n");

    // d-scan
    while (!eb_engine_check_exit()) {
        POINT dscan_pt = { 0 };
        int alarm = 0;
        if (!eb_game_dscan_watch(instance, &alarm)) {
            LOGI("D-Scan Failed\n");
        }
        else if (alarm) {
            LOGI("Threat Detected!\n");
            eb_game_alarm_sound();
        }
        eb_engine_idle(5000);
    }

    return true;
}
