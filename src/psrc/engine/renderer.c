#include "renderer.h"

#ifdef PSRC_ENGINE_RENDERER_USEGL
    #include "renderer_gl.h"
#endif

#include "../version.h"
#include "../debug.h"
#include "../common.h"

#include "../logging.h"
#include "../string.h"
//#include "../threading.h"

#include "../common/p3m.h"

#include "../../stb/stb_image.h"

#if PLATFORM == PLAT_EMSCR
    #include <emscripten/html5.h>
#endif

#include <string.h>
#include <stddef.h>
#include <math.h>
#include <stdarg.h>

#include "../glue.h"

struct rendstate rendstate;

const char* const* rendapi_names[RENDAPI__COUNT] = {
    #ifdef PSRC_ENGINE_RENDERER_USEGL
    (const char*[]){"gl20", "OpenGL 2.0"},
    #endif
};

#if 0 // silence a warning
static void* r_dummy_takeScreenshot(int* w, int* h, int* sz) {
    (void)w; (void)h; (void)sz;
    return NULL;
}
#endif

static enum rendapi trylist[] = {
    #ifdef PSRC_ENGINE_RENDERER_USEGL
    RENDAPI_GL20,
    #endif
    RENDAPI__INVALID
};

void (*render)(void);
void (*display)(void);
void* (*takeScreenshot)(unsigned* w, unsigned* h, unsigned* ch);
static bool (*beforeCreateWindow)(unsigned*);
static bool (*afterCreateWindow)(void);
static void (*beforeDestroyWindow)(void);
static void (*updateFrame)(void);
static void (*updateVSync)(void);

static void destroyWindow(void) {
    #ifndef PSRC_USESDL1
    if (rendstate.window != NULL) {
        beforeDestroyWindow();
        SDL_Window* w = rendstate.window;
        rendstate.window = NULL;
        SDL_DestroyWindow(w);
    }
    #else
    beforeDestroyWindow();
    #endif
}

static void updateWindowMode(enum rendmode newmode) {
    switch (newmode) {
        case RENDMODE_WINDOWED: {
            if (rendstate.mode != RENDMODE_WINDOWED) {
                rendstate.res.current = rendstate.res.windowed;
                #if PLATFORM != PLAT_EMSCR
                #ifndef PSRC_USESDL1
                SDL_SetWindowFullscreen(rendstate.window, 0);
                #endif
                #else
                emscripten_exit_fullscreen();
                #endif
                rendstate.mode = RENDMODE_WINDOWED;
            }
            #ifndef PSRC_USESDL1
            SDL_SetWindowSize(rendstate.window, rendstate.res.current.width, rendstate.res.current.height);
            #else
            SDL_SetVideoMode(rendstate.res.current.width, rendstate.res.current.height, rendstate.bpp, rendstate.flags);
            #endif
        } break;
        case RENDMODE_BORDERLESS: {
            #if PLATFORM != PLAT_EMSCR
            if (rendstate.mode != RENDMODE_BORDERLESS) {
                rendstate.res.current = rendstate.res.fullscr;
                rendstate.mode = RENDMODE_BORDERLESS;
            }
            #ifndef PSRC_USESDL1
            SDL_DisplayMode mode;
            SDL_GetCurrentDisplayMode(0, &mode);
            mode.w = rendstate.res.current.width;
            mode.h = rendstate.res.current.height;
            SDL_SetWindowDisplayMode(rendstate.window, &mode);
            SDL_SetWindowFullscreen(rendstate.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            #else
            SDL_SetVideoMode(rendstate.res.current.width, rendstate.res.current.height, rendstate.bpp, rendstate.flags | SDL_NOFRAME);
            #endif
            #else
            if (emscripten_request_fullscreen("#canvas", false) == EMSCRIPTEN_RESULT_SUCCESS) {
                rendstate.res.current = rendstate.res.fullscr;
                SDL_SetWindowSize(rendstate.window, rendstate.res.current.width, rendstate.res.current.height);
                rendstate.mode = RENDMODE_BORDERLESS;
            } else {
                plog(LL_WARN, "Failed to go to fullscreen (canvas has probably not acquired an input lock)");
                rendstate.mode = RENDMODE_WINDOWED;
            }
            #endif
        } break;
        case RENDMODE_FULLSCREEN: {
            #if PLATFORM != PLAT_EMSCR
            if (rendstate.mode != RENDMODE_FULLSCREEN) {
                rendstate.res.current = rendstate.res.fullscr;
                rendstate.mode = RENDMODE_FULLSCREEN;
            }
            #ifndef PSRC_USESDL1
            SDL_DisplayMode mode;
            SDL_GetCurrentDisplayMode(0, &mode);
            mode.w = rendstate.res.current.width;
            mode.h = rendstate.res.current.height;
            SDL_SetWindowDisplayMode(rendstate.window, &mode);
            SDL_SetWindowFullscreen(rendstate.window, SDL_WINDOW_FULLSCREEN);
            #else
            SDL_SetVideoMode(rendstate.res.current.width, rendstate.res.current.height, rendstate.bpp, rendstate.flags | SDL_FULLSCREEN);
            #endif
            #else
            if (emscripten_request_fullscreen("#canvas", false) == EMSCRIPTEN_RESULT_SUCCESS) {
                rendstate.res.current = rendstate.res.fullscr;
                SDL_SetWindowSize(rendstate.window, rendstate.res.current.width, rendstate.res.current.height);
                rendstate.mode = RENDMODE_FULLSCREEN;
            } else {
                plog(LL_WARN, "Failed to go to fullscreen (canvas has probably not acquired an input lock)");
                rendstate.mode = RENDMODE_WINDOWED;
            }
            #endif
        } break;
    }
}

static void updateWindowIcon(void) {
    if (!rendstate.icon) return;
    int w, h, c;
    #if DEBUG(1)
    plog(LL_INFO | LF_DEBUG, "Setting window icon to '%s'...", rendstate.icon);
    #endif
    void* data = stbi_load(rendstate.icon, &w, &h, &c, STBI_rgb_alpha);
    if (data) {
        SDL_Surface* s = SDL_CreateRGBSurfaceFrom(
            data, w, h, 32, w * 4,
            0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
        );
        #ifndef PSRC_USESDL1
        SDL_SetWindowIcon(rendstate.window, s);
        #else
        SDL_WM_SetIcon(s, NULL);
        #endif
        SDL_FreeSurface(s);
        stbi_image_free(data);
    } else {
        plog(LL_WARN, "Failed to set window icon");
    }
}

#ifndef PSRC_USESDL1
#define SDL_SetHint(n, v) if (!SDL_SetHint((n), (v))) plog(LL_WARN, "Failed to set " #n " to %s: %s", (char*)(v), SDL_GetError())
#define SDL_SetHintWithPriority(n, v, p) if (!SDL_SetHintWithPriority((n), (v), (p))) plog(LL_WARN, "Failed to set " #n " to %s using " #p ": %s", (char*)(v), SDL_GetError())
#endif
static bool createWindow(void) {
    if (rendstate.api <= RENDAPI__INVALID || rendstate.api >= RENDAPI__COUNT) {
        plog(LL_CRIT, "Invalid rendering API (%d)", (int)rendstate.api);
        return false;
    }
    #ifndef PSRC_USESDL1
    #ifdef SDL_HINT_NO_SIGNAL_HANDLERS
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    #endif
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    #ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    #endif
    //SDL_SetRelativeMouseMode(SDL_TRUE);
    #endif
    unsigned flags;
    #ifndef PSRC_USESDL1
    flags = SDL_WINDOW_RESIZABLE;
    {
        SDL_DisplayMode dtmode;
        SDL_GetDesktopDisplayMode(0, &dtmode);
        if (!rendstate.res.fullscr.width) rendstate.res.fullscr.width = dtmode.w;
        if (!rendstate.res.fullscr.height) rendstate.res.fullscr.height = dtmode.h;
        if (rendstate.fps < 0) rendstate.fps = dtmode.refresh_rate;
    }
    switch (rendstate.mode) {
        case RENDMODE_WINDOWED:
            rendstate.res.current = rendstate.res.windowed;
            break;
        case RENDMODE_BORDERLESS:
            rendstate.res.current = rendstate.res.fullscr;
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            break;
        case RENDMODE_FULLSCREEN:
            rendstate.res.current = rendstate.res.fullscr;
            flags |= SDL_WINDOW_FULLSCREEN;
            break;
    }
    if (!beforeCreateWindow(&flags)) {
        rendstate.api = RENDAPI__INVALID;
        return false;
    }
    #else
    flags = 0;
    rendstate.flags = SDL_ANYFORMAT;
    switch (rendstate.mode) {
        case RENDMODE_WINDOWED:
            rendstate.res.current = rendstate.res.windowed;
            break;
        case RENDMODE_BORDERLESS:
            rendstate.res.current = rendstate.res.fullscr;
            flags |= SDL_NOFRAME;
            break;
        case RENDMODE_FULLSCREEN:
            rendstate.res.current = rendstate.res.fullscr;
            flags |= SDL_FULLSCREEN;
            break;
    }
    if (!beforeCreateWindow(&rendstate.flags)) {
        rendstate.api = RENDAPI__INVALID;
        return false;
    }
    #endif
    #if DEBUG(1)
    plog(LL_INFO | LF_DEBUG, "Windowed resolution: %dx%d", rendstate.res.windowed.width, rendstate.res.windowed.height);
    plog(LL_INFO | LF_DEBUG, "Fullscreen resolution: %dx%d", rendstate.res.fullscr.width, rendstate.res.fullscr.height);
    #endif
    #ifndef PSRC_USESDL1
    rendstate.window = SDL_CreateWindow(
        titlestr,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        rendstate.res.current.width, rendstate.res.current.height,
        SDL_WINDOW_SHOWN | flags
    );
    if (!rendstate.window) {
        plog(LL_CRIT | LF_FUNCLN, "Failed to create window: %s", SDL_GetError());
        return false;
    }
    SDL_SetWindowMinimumSize(rendstate.window, 320, 240);
    #else
    if (!SDL_SetVideoMode(rendstate.res.current.width, rendstate.res.current.height, rendstate.bpp, rendstate.flags | flags)) {
        plog(LL_CRIT | LF_FUNCLN, "Failed to create window: %s", SDL_GetError());
        return false;
    }
    SDL_WM_SetCaption(titlestr, NULL);
    #endif
    updateWindowIcon();
    if (!afterCreateWindow()) {
        rendstate.api = RENDAPI__INVALID;
        destroyWindow();
        return false;
    }
    return true;
}

static bool startRenderer_internal(void) {
    switch (rendstate.api) {
        #ifdef PSRC_ENGINE_RENDERER_USEGL
        case RENDAPI_GL20:
            if (!r_gl_prepRenderer()) return false;
            render = r_gl_render;
            display = r_gl_display;
            takeScreenshot = r_gl_takeScreenshot;
            beforeCreateWindow = r_gl_beforeCreateWindow;
            afterCreateWindow = r_gl_afterCreateWindow;
            beforeDestroyWindow = r_gl_beforeDestroyWindow;
            updateFrame = r_gl_updateFrame;
            updateVSync = r_gl_updateVSync;
            break;
        #endif
        default:
            return false;
    }
    if (!createWindow()) return false;
    return true;
}

static void stopRenderer_internal(void) {
    destroyWindow();
}

bool reloadRenderer(void) {
    stopRenderer_internal();
    return startRenderer_internal();
}

bool startRenderer(void) {
    if (rendstate.api != RENDAPI__INVALID) {
        if (startRenderer_internal()) return true;
    }
    for (int i = 0; (rendstate.api = trylist[i]) != RENDAPI__INVALID; ++i) {
        if (startRenderer_internal()) return true;
    }
    plog(LL_CRIT, "Could not use any available rendering APIs");
    return false;
}

void stopRenderer(void) {
    stopRenderer_internal();
}

bool updateRendererConfig(enum rendopt opt, ...) {
    va_list args;
    va_start(args, opt);
    while (1) {
        switch (opt) {
            case RENDOPT_END: {
                goto rettrue;
            } break;
            case RENDOPT_ICON: {
                free(rendstate.icon);
                rendstate.icon = strdup(va_arg(args, char*));
                updateWindowIcon();
            } break;
            case RENDOPT_API: {
                enum rendapi oldapi = rendstate.api;
                stopRenderer_internal();
                rendstate.api = va_arg(args, int);
                if (!startRenderer_internal()) {
                    plog(
                        LL_WARN,
                        "Failed to restart renderer after changing API to %s. Reverting to %s...",
                        rendapi_names[rendstate.api][1], rendapi_names[oldapi][1]
                    );
                    rendstate.api = oldapi;
                    if (!startRenderer_internal()) {
                        plog(LL_ERROR, "Failed to restart renderer after reverting API to %s.", rendapi_names[rendstate.api][1]);
                        goto retfalse;
                    }
                }
            } break;
            case RENDOPT_FULLSCREEN: {
                int tmp = va_arg(args, int);
                enum rendmode newmode;
                if (tmp < 0) {
                    newmode = (rendstate.mode == RENDMODE_WINDOWED) ?
                        ((rendstate.borderless) ? RENDMODE_BORDERLESS : RENDMODE_FULLSCREEN) :
                        RENDMODE_WINDOWED;
                } else if (tmp) {
                    newmode = (rendstate.borderless) ? RENDMODE_BORDERLESS : RENDMODE_FULLSCREEN;
                } else {
                    newmode = RENDMODE_WINDOWED;
                }
                updateWindowMode(newmode);
                updateFrame();
            } break;
            case RENDOPT_BORDERLESS: {
                rendstate.borderless = va_arg(args, int);
                enum rendmode newmode = (rendstate.mode == RENDMODE_BORDERLESS || rendstate.mode == RENDMODE_FULLSCREEN) ?
                    ((rendstate.borderless) ? RENDMODE_BORDERLESS : RENDMODE_FULLSCREEN) :
                    RENDMODE_WINDOWED;
                updateWindowMode(newmode);
                updateFrame();
            } break;
            case RENDOPT_VSYNC: {
                rendstate.vsync = va_arg(args, int);
                updateVSync();
            } break;
            case RENDOPT_RES: {
                struct rendres* res = va_arg(args, struct rendres*);
                if (res->width) rendstate.res.current.width = res->width;
                if (res->height) rendstate.res.current.height = res->height;
                switch (rendstate.mode) {
                    case RENDMODE_WINDOWED:
                        if (rendstate.res.current.width != rendstate.res.windowed.width ||
                            rendstate.res.current.height != rendstate.res.windowed.height) {
                            rendstate.res.windowed = rendstate.res.current;
                            updateWindowMode(rendstate.mode);
                            updateFrame();
                        }
                        break;
                    case RENDMODE_BORDERLESS:
                    case RENDMODE_FULLSCREEN:
                        if (rendstate.res.current.width != rendstate.res.fullscr.width ||
                            rendstate.res.current.height != rendstate.res.fullscr.height) {
                            rendstate.res.fullscr = rendstate.res.current;
                            updateWindowMode(rendstate.mode);
                            updateFrame();
                        }
                        break;
                }
            } break;
            case RENDOPT_LIGHTING: {
                rendstate.lighting = va_arg(args, int);
            } break;
            case RENDOPT_TEXTUREQLT: {
                rendstate.texqlt = va_arg(args, int);
            } break;
        }
        opt = va_arg(args, int);
    }
    rettrue:;
    va_end(args);
    return true;
    retfalse:;
    va_end(args);
    return false;
}

bool initRenderer(void) {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        plog(LL_WARN | LF_FUNCLN, "Failed to init video: %s", SDL_GetError());
        #if PLATFORM == PLAT_LINUX
        unsetenv("SDL_VIDEODRIVER");
        if (SDL_Init(SDL_INIT_VIDEO)) {
            plog(LL_CRIT | LF_FUNCLN, "Failed to init video: %s", SDL_GetError());
            return false;
        }
        #else
        return false;
        #endif
    }
    char* tmp = cfg_getvar(&config, "Renderer", "api");
    if (tmp) {
        enum rendapi i = 0;
        while (1) {
            if (i == RENDAPI__COUNT) {
                rendstate.api = RENDAPI__INVALID;
                break;
            }
            if (!strcasecmp(tmp, rendapi_names[i][0])) {
                rendstate.api = i;
                break;
            }
            ++i;
        }
        free(tmp);
    } else {
        rendstate.api = RENDAPI__INVALID;
    }
    tmp = cfg_getvar(&config, "Renderer", "resolution.windowed");
    #if PLATFORM == PLAT_EMSCR
    rendstate.res.windowed = (struct rendres){960, 720};
    #else
    rendstate.res.windowed = (struct rendres){800, 600};
    #endif
    if (tmp) {
        sscanf(
            tmp, "%ux%u",
            &rendstate.res.windowed.width,
            &rendstate.res.windowed.height
        );
        free(tmp);
    }
    tmp = cfg_getvar(&config, "Renderer", "resolution.fullscreen");
    rendstate.res.fullscr = (struct rendres){0, 0};
    if (tmp) {
        sscanf(
            tmp, "%ux%u",
            &rendstate.res.fullscr.width,
            &rendstate.res.fullscr.height
        );
        free(tmp);
    }
    tmp = cfg_getvar(&config, "Renderer", "fps");
    rendstate.fps = -1;
    if (tmp) {
        sscanf(tmp, "%d", &rendstate.fps);
        free(tmp);
    }
    #ifdef PSRC_USESDL1
    {
        const SDL_VideoInfo* vinf = SDL_GetVideoInfo();
        rendstate.bpp = vinf->vfmt->BitsPerPixel;
        if (!rendstate.res.fullscr.width) rendstate.res.fullscr.width = vinf->current_w;
        if (!rendstate.res.fullscr.height) rendstate.res.fullscr.height = vinf->current_h;
        if (rendstate.fps < 0) rendstate.fps = 60; // TODO: get the actual hz somehow?
    }
    #endif
    tmp = cfg_getvar(&config, "Renderer", "borderless");
    if (tmp) {
        rendstate.borderless = strbool(tmp, false);
        free(tmp);
    } else {
        rendstate.borderless = false;
    }
    tmp = cfg_getvar(&config, "Renderer", "fullscreen");
    rendstate.mode = (strbool(tmp, false)) ?
        ((rendstate.borderless) ? RENDMODE_BORDERLESS : RENDMODE_FULLSCREEN) :
        RENDMODE_WINDOWED;
    free(tmp);
    tmp = cfg_getvar(&config, "Renderer", "vsync");
    if (tmp) {
        rendstate.vsync = strbool(tmp, true);
        free(tmp);
    } else {
        rendstate.vsync = true;
    }
    return true;
}

void quitRenderer(void) {
    free(rendstate.icon);
}
