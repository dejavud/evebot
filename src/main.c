#include "eb_common.h"
#include "eb_utils.h"
#include "eb_log.h"
#include "eb_config.h"
#include "eb_resmanager.h"
#include "eb_engine.h"

static bool do_init(int argc, char** argv);
static void do_final();
static void set_working_dir();
static BOOL WINAPI ctrl_handler(DWORD type);

static eb_config s_config;

int main(int argc, char* argv[])
{
    if (do_init(argc, argv)) {
        if (eb_engine_init(&s_config))
            eb_engine_launch();
        eb_engine_shutdown();
    }
    do_final();
    
    return 0;
}

static bool do_init(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    set_working_dir();

    eb_log_set_level(EB_LOG_LEVEL_DEBUG);

    SetConsoleCtrlHandler(ctrl_handler, TRUE);

    // load config
    memset(&s_config, 0x00, sizeof(s_config));
    if (!eb_config_load(argc, argv, &s_config))
        return false;

    // load resources
    if (!eb_resmanager_load(s_config.images_conf_path, s_config.layout_conf_path))
        return false;

    return true;
}

static void do_final()
{
    eb_resmanager_clean();

    if (s_config.shutdown_system) {
        for (int i = 30; i > 0; i--) {
            LOGI("\rShutdown the system, are you sure? (CTRL-C to abort) (%d)", i);
            Sleep(1000);
        }
        LOGI("\n");
        system("shutdown /s /t 60");
    }
}

static void set_working_dir()
{
    const char* dir = eb_utils_get_app_dir();

    wchar_t* wdir = eb_utils_str_u8tow(dir, -1);
    BOOL r = SetCurrentDirectoryW(wdir);
    assert(r);
    eb_utils_str_free(wdir);
}

static BOOL WINAPI ctrl_handler(DWORD type)
{
    BOOL handled = FALSE;

    switch (type) {
    case CTRL_C_EVENT:

        break;
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        break;
    default:
        break;
    }

    return handled;
}
