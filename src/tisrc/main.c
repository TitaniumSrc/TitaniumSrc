#include "version.h"
#include "platform.h"
#include "debug.h"
#include "common.h"
#include "logging.h"
#include "string.h"
#include "time.h"

#include "common/config.h"

#ifndef TISRC_MODULE_SERVER
    #include "incsdl.h"
    #if defined(TISRC_USESDL1) && defined(main)
        #undef main
    #endif
    #if PLATFORM == PLAT_GDK
        #include <SDL_main.h>
    #endif
#endif

#if (PLATFLAGS & PLATFLAG_UNIXLIKE) || PLATFORM == PLAT_WIN32
    #include <signal.h>
#endif
#include <string.h>
#if !(PLATFLAGS & PLATFLAG_WINDOWSLIKE)
    #include <unistd.h>
#endif
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#if PLATFORM == PLAT_ANDROID
    #include <android/log.h>
#elif (PLATFLAGS & PLATFLAG_WINDOWSLIKE)
    #include <windows.h>
#elif PLATFORM == PLAT_EMSCR
    #include <emscripten.h>
#endif

#include "glue.h"

#if defined(TISRC_MODULE_ENGINE)
    #include "engine/engine.h"
#elif defined(TISRC_MODULE_SERVER)
    #include "server/server.h"
#elif defined(TISRC_MODULE_EDITOR)
    #include "editor/editor.h"
#endif

#if (PLATFLAGS & PLATFLAG_UNIXLIKE) || PLATFORM == PLAT_WIN32
static void sigh_log(int l, char* name, char* msg) {
    if (msg) {
        plog(l, "Received signal: %s; %s", name, msg);
    } else {
        plog(l, "Received signal: %s", name);
    }
}
static void sigh(int sig) {
    signal(sig, sigh);
    #if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L
    char* signame = strsignal(sig);
    #else
    char signame[12];
    snprintf(signame, sizeof(signame), "%d", sig);
    #endif
    switch (sig) {
        case SIGINT:
            if (quitreq) {
                sigh_log(LL_WARN, signame, "Graceful exit already requested; Forcing exit...");
                exit(1);
            } else {
                sigh_log(LL_INFO, signame, "Requesting graceful exit...");
                ++quitreq;
            }
            break;
        case SIGTERM:
        #ifdef SIGQUIT
        case SIGQUIT:
        #endif
            sigh_log(LL_WARN, signame, "Forcing exit...");
            exit(1);
            break;
        case SIGSEGV:
        #ifdef SIGABRT
        case SIGABRT:
        #endif
        #ifdef SIGBUS
        case SIGBUS:
        #endif
        #ifdef SIGFPE
        case SIGFPE:
        #endif
        #ifdef SIGILL
        case SIGILL:
        #endif
            plog(LL_CRIT, "Received signal: %s", signame);
            exit(1);
            break;
        default:
            sigh_log(LL_WARN, signame, NULL);
            break;
    }
}
#endif

#if PLATFORM == PLAT_EMSCR
volatile bool issyncfsok = false;
void __attribute__((used)) syncfsok(void) {issyncfsok = true;}
static void emscrexit(void* arg) {
    exit((int)(uintptr_t)arg);
}
static void emscrmain(void) {
    static bool doloop = false;
    static bool syncmsg = false;
    if (doloop) {
        if (!quitreq) {
            loop();
        } else {
            static bool doexit = false;
            if (doexit) {
                if (!syncmsg) {
                    puts("Waiting on RAM to disk FS.syncfs...");
                    syncmsg = true;
                }
                if (issyncfsok) {
                    emscripten_cancel_main_loop();
                    emscripten_async_call(emscrexit, (void*)(uintptr_t)0, -1);
                    puts("Done");
                }
                return;
            }
            unstrap();
            EM_ASM(
                FS.syncfs(false, function (e) {
                    if (e) console.error("FS.syncfs:", e);
                    ccall("syncfsok");
                });
            );
            doexit = true;
        }
    } else {
        if (!syncmsg) {
            puts("Waiting on disk to RAM FS.syncfs...");
            syncmsg = true;
        }
        if (!issyncfsok) return;
        issyncfsok = false;
        syncmsg = false;
        static int ret;
        ret = bootstrap();
        if (!ret) {
            doloop = true;
            return;
        }
        emscripten_cancel_main_loop();
        emscripten_async_call(emscrexit, (void*)(uintptr_t)1, -1);
    }
}
#elif PLATFORM == PLAT_GDK
#define main SDL_main
#endif

#if PLATFORM == PLAT_ANDROID
__attribute__((visibility ("default")))
#endif
int main(int argc, char** argv) {
    makeVerStrs();

    int ret;
    if (argc > 1) {
        ret = parseargs(argc - 1, argv + 1);
        if (ret >= 0) return ret;
    }

    puts(verstr);
    puts(platstr);
    #if PLATFORM == PLAT_LINUX
        #ifndef TISRC_MODULE_SERVER
            setenv("SDL_VIDEODRIVER", "wayland", false);
        #endif
    #elif PLATFORM == PLAT_ANDROID
        __android_log_write(ANDROID_LOG_INFO, "TitaniumSrc", verstr);
        __android_log_write(ANDROID_LOG_INFO, "TitaniumSrc", platstr);
    #endif

    if (!initLogging()) {
        fputs("{X}: Failed to init logging\n", stderr);
        return 1;
    }

    #if (PLATFLAGS & PLATFLAG_UNIXLIKE) || PLATFORM == PLAT_WIN32
        signal(SIGINT, sigh);
        signal(SIGTERM, sigh);
        #ifdef SIGQUIT
        signal(SIGQUIT, sigh);
        #endif
        #ifdef SIGUSR1
        signal(SIGUSR1, SIG_IGN);
        #endif
        #ifdef SIGUSR2
        signal(SIGUSR2, SIG_IGN);
        #endif
        #ifdef SIGPIPE
        signal(SIGPIPE, SIG_IGN);
        #endif
        #if !DEBUG(0)
            signal(SIGSEGV, sigh);
            #ifdef SIGABRT
            signal(SIGABRT, sigh);
            #endif
            #ifdef SIGBUS
            signal(SIGBUS, sigh);
            #endif
            #ifdef SIGFPE
            signal(SIGFPE, sigh);
            #endif
            #ifdef SIGILL
            signal(SIGILL, sigh);
            #endif
        #endif
    #endif
    #if PLATFORM == PLAT_WIN32
        TIMECAPS tc;
        UINT tmrres = 1;
        if (timeGetDevCaps(&tc, sizeof(tc)) != TIMERR_NOERROR) {
            if (tmrres < tc.wPeriodMin) tmrres = tc.wPeriodMin;
            else if (tmrres > tc.wPeriodMax) tmrres = tc.wPeriodMax;
        }
        timeBeginPeriod(tmrres);
    #endif
    #if (PLATFLAGS & PLATFLAG_WINDOWSLIKE)
        QueryPerformanceFrequency(&perfctfreq);
        while (!(perfctfreq.QuadPart % 10) && !(perfctmul % 10)) {
            perfctfreq.QuadPart /= 10;
            perfctmul /= 10;
        }
    #endif

    #if PLATFORM != PLAT_EMSCR
        ret = bootstrap();
        if (!ret) {
            while (!quitreq) {
                loop();
            }
            unstrap();
        }
    #else
        EM_ASM(
            FS.mkdir('/data');
            FS.mount(IDBFS, {autoPersist: true}, '/data');
            FS.syncfs(true, function (e) {
                if (e) console.error("FS.syncfs:", e);
                ccall("syncfsok");
            });
        );
        emscripten_set_main_loop(emscrmain, -1, true);
        ret = 0;
    #endif

    #if PLATFORM == PLAT_WIN32
        timeEndPeriod(tmrres);
    #endif

    return ret;
}
