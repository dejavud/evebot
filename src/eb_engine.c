#include "eb_engine.h"
#include "eb_utils.h"
#include "eb_log.h"
#include "eb_resmanager.h"
#include "eb_game.h"
#include "eb_task.h"

#define EVE_WND_TITLE_PREFIX    L"EVE - "
#define EVE_WND_CLASS_NAME      L"triuiScreen"

#define GAME_INSTANCE_MAX 256

typedef struct eb_engine_t
{
    HANDLE exit_event;
    eb_config* config;
    eb_game_instance game_instances[GAME_INSTANCE_MAX];
    int n_games;
    eb_game_instance target_game;
} eb_engine;

static eb_engine* s_engine = NULL;

static void eb_engine_detect_game();
static BOOL CALLBACK eb_enum_window_callback(HWND hwnd, LPARAM lparam);
static bool choose_target_game();

bool eb_engine_init(eb_config* config)
{
    assert(config != NULL);

    s_engine = (eb_engine*)malloc(sizeof(eb_engine));
    if (s_engine == NULL)
        return false;
    memset(s_engine, 0x00, sizeof(eb_engine));
    s_engine->config = config;

    bool result = false;
    do {
        s_engine->exit_event = CreateEvent(NULL, TRUE, FALSE, NULL); // manual-reset 
        if (s_engine->exit_event == NULL)
            break;

        eb_engine_detect_game();
        if (!choose_target_game())
            break;

        result = true;
    } while (0);

    return result;
}

void eb_engine_shutdown()
{
    SetEvent(s_engine->exit_event);
    CloseHandle(s_engine->exit_event);
    s_engine->exit_event = NULL;

    for (int i = 0; i < s_engine->n_games; i++) {
        eb_game_destroy_instance(s_engine->game_instances[i]);
        s_engine->game_instances[i] = NULL;
    }

    free(s_engine);
    s_engine = NULL;
}

bool eb_engine_launch()
{
    assert(s_engine != NULL && s_engine->target_game != NULL);
    assert(s_engine->config != NULL);

    switch (s_engine->config->mode) {
    case EB_MODE_TEST:
        eb_task_test(s_engine->target_game);
        break;
    case EB_MODE_AUTOPILOT:
        LOGI("Current Task: [Autopilot]\n");
        eb_task_autopilot(s_engine->target_game);
        break;
    case EB_MODE_MISSION:
        LOGI("Current Task: [Automission]\n");
        eb_task_automission_distribution(s_engine->target_game);
        break;
    case EB_MODE_DSCAN:
        LOGI("Current Task: [D-Scan]\n");
        eb_task_dscan_watch(s_engine->target_game);
        break;
    default:
        LOGI("No Task, Quit...\n");
        break;
    }

    return true;
}

bool eb_engine_idle(int milliseconds)
{
    assert(s_engine != NULL && s_engine->exit_event != NULL);

    return WaitForSingleObject(s_engine->exit_event, milliseconds) != WAIT_OBJECT_0;
}

bool eb_engine_check_exit()
{
    return !eb_engine_idle(0);
}

static void eb_engine_detect_game()
{
    assert(s_engine != NULL);

    EnumWindows(eb_enum_window_callback, (LPARAM)s_engine);
}

static BOOL CALLBACK eb_enum_window_callback(HWND hwnd, LPARAM lparam)
{
    eb_engine* s_engine = (eb_engine*)lparam;
    assert(s_engine != NULL);

    if (eb_engine_check_exit())
        return FALSE;

    wchar_t wnd_title[256];
    GetWindowTextW(hwnd, wnd_title, 256);
    wchar_t class_name[256];
    GetClassNameW(hwnd, class_name, 256);

    if (wcsncmp(wnd_title, EVE_WND_TITLE_PREFIX, wcslen(EVE_WND_TITLE_PREFIX)) == 0 &&
        wcsncmp(class_name, EVE_WND_CLASS_NAME, wcslen(EVE_WND_CLASS_NAME)) == 0) {
        const char* charname = eb_utils_str_wtou8(wnd_title + wcslen(EVE_WND_TITLE_PREFIX), -1);
        assert(charname != NULL);

        s_engine->game_instances[s_engine->n_games++] = eb_game_create_instance(hwnd, charname);
    }

    return TRUE;
}

static bool choose_target_game()
{
    assert(s_engine != NULL);
    assert(s_engine->config != NULL);

    if (s_engine->n_games == 0) {
        LOGE("Game window not found.\n");
        return false;
    }
    
    const char* target_charname = s_engine->config->charname;
    eb_game_instance target_game = NULL;
    if (target_charname != NULL && target_charname[0] != '\0') {
        for (int i = 0; i < s_engine->n_games; i++) {
            eb_game_instance game = s_engine->game_instances[i];
            assert(game != NULL);
            if (strcmp(target_charname, eb_game_charname(game)) == 0) {
                target_game = game;
            }
        }
        if (target_game == NULL) {  // not found
            LOGE("(%s) Game window not found.\n", target_charname);
            return false;
        }
    }
    else {
        if (s_engine->n_games == 1) { // only one instance, auto-select
            target_game = s_engine->game_instances[0];
            target_charname = eb_game_charname(target_game);
        }
        else {
            for (int i = 0; i < s_engine->n_games; i++) {
                eb_game_instance game = s_engine->game_instances[i];
                assert(game != NULL);
                LOGI("%d) %s\n", i + 1, eb_game_charname(game));
            }
            LOGI("Choose Character: ");
            int choose_index = 0;
            int r = scanf("%d", &choose_index);
            if (r == 1 && choose_index >= 1 && choose_index < s_engine->n_games + 1) {
                target_game = s_engine->game_instances[choose_index - 1];
                target_charname = eb_game_charname(target_game);
            }
            else {
                LOGE("Input Error.\n");
                return false;
            }
        }
    }
    s_engine->target_game = target_game;
    LOGI("Target Character: %s\n", target_charname);

    return true;
}
