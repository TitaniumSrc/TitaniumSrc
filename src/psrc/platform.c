#include "platform.h"
#include <stddef.h>

const char* platname[PLAT__COUNT] = {
    "Unknown",
    "Unix",
    "Linux",
    "Windows",
    "MacOS",
    "Emscripten",
    "FreeBSD",
    "NetBSD",
    "OpenBSD",
    "Microsoft GDK",
    "Android"
};
const char* platid[PLAT__COUNT] = {
    "unknown",
    "unix",
    "linux",
    "win32",
    "macos",
    "emscr",
    "freebsd",
    "netbsd",
    "openbsd",
    "gdk",
    "android"
};
const char* const* platdir[PLAT__COUNT] = {
    (const char* const[]){NULL},
    (const char* const[]){"unix", NULL},
    (const char* const[]){"linux", "unix", NULL},
    (const char* const[]){"windows", "win32", NULL},
    (const char* const[]){"macos", "unix", NULL},
    (const char* const[]){"emscripten", "emscr", NULL},
    (const char* const[]){"freebsd", "bsd", "unix", NULL},
    (const char* const[]){"netbsd", "bsd", "unix", NULL},
    (const char* const[]){"openbsd", "bsd", "unix", NULL},
    (const char* const[]){"gdk", "win32", "windows", NULL},
    (const char* const[]){"android", "unix", "linux", NULL},
};
