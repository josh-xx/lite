#include <stdio.h>
#include <SDL2/SDL.h>
#include "api/api.h"
#include "renderer.h"

#ifdef _WIN32
  #include <windows.h>
#elif __linux__
  #include <unistd.h>
#elif __APPLE__
  #include <mach-o/dyld.h>
#endif


SDL_Window *window;


// win 下面，这里先取 dpi，然后除以 96
// 96 是常见的 dpi
// 为什么 win 要动态获取，其他平台返回 1，这个暂时不清楚
static double get_scale(void) {
  float dpi;
  // https://wiki.libsdl.org/SDL_GetDisplayDPI
  SDL_GetDisplayDPI(0, NULL, &dpi, NULL);
#if _WIN32
  return dpi / 96.0;
#else
  return 1.0;
#endif
}


// buf 是需要传给外面的字符串
// sz 是外面声明的字符串长度
// 获取当前的编辑器可执行文件的路径
static void get_exe_filename(char *buf, int sz) {
#if _WIN32
  int len = GetModuleFileName(NULL, buf, sz - 1);
  buf[len] = '\0';
#elif __linux__
  char path[512];
  sprintf(path, "/proc/%d/exe", getpid());
  printf("path %s\n", path);
  int len = readlink(path, buf, sz - 1);
  buf[len] = '\0';
#elif __APPLE__
  unsigned size = sz;
  _NSGetExecutablePath(buf, &size);
#else
  strcpy(buf, "./lite");
#endif
}


// 看起来是设置程序的图标用的，这个好像只在 windows 下执行
static void init_window_icon(void) {
#ifndef _WIN32
  #include "../icon.inl"
  (void) icon_rgba_len; /* unused */
  SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(
    icon_rgba, 64, 64,
    32, 64 * 4,
    0x000000ff,
    0x0000ff00,
    0x00ff0000,
    0xff000000);
  SDL_SetWindowIcon(window, surf);
  SDL_FreeSurface(surf);
#endif
}


int main(int argc, char **argv) {
#ifdef _WIN32
  // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiaware
  HINSTANCE lib = LoadLibrary("user32.dll");
  int (*SetProcessDPIAware)() = (void*) GetProcAddress(lib, "SetProcessDPIAware");
  SetProcessDPIAware();
#endif

  // 初始化
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  // 和屏保有关，不清楚
  SDL_EnableScreenSaver();
  // 文件拖放打开事件
  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
  // 调用 exit(status) 的时候调用 SDL_Quit
  atexit(SDL_Quit);

// 不清楚，和逻辑无关，先不管
#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR /* Available since 2.0.8 */
  SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode(0, &dm);

  // dm.w 屏幕宽度
  // dm.h 屏幕高度
  window = SDL_CreateWindow(
    "", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w * 0.8, dm.h * 0.8,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
  init_window_icon();

  // 初始化渲染
  ren_init(window);


  // 初始化 lua
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  // 注入 lua 方法
  api_load_libs(L);

  // ARGS = argv
  lua_newtable(L);
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    // table
    // argv[i]

    // table[i + 1] = argv[i] # lua 的下标从 1 开始
    lua_rawseti(L, -2, i + 1);
  }
  lua_setglobal(L, "ARGS");

  // VERSION = 1.11
  lua_pushstring(L, "1.11");
  lua_setglobal(L, "VERSION");

  // PLATFORM =
  lua_pushstring(L, SDL_GetPlatform());
  lua_setglobal(L, "PLATFORM");

  // SCALE = get_scale()
  lua_pushnumber(L, get_scale());
  lua_setglobal(L, "SCALE");

  // EXEFILE = get_exe_filename()
  char exename[2048];
  get_exe_filename(exename, sizeof(exename));
  lua_pushstring(L, exename);
  lua_setglobal(L, "EXEFILE");


  // 执行 lua 脚本
  // local core
  // xpcall(
  //   function()
  //     SCALE = tonumber(os.getenv("LITE_SCALE")) or SCALE
  //     PATHSEP = package.config:sub(1, 1)
  //     EXEDIR = EXEFILE:match("^(.+)[/\\\\].*$")
  //     package.path = EXEDIR .. '/data/?.lua;' .. package.path
  //     package.path = EXEDIR .. '/data/?/init.lua;' .. package.path
  //     ; 加载 data/core
  //     core = require('core')
  //     core.init()
  //     core.run()
  //   end, 
  //   function(err)
  //     print('Error: ' .. tostring(err))
  //     print(debug.traceback(nil, 2))
  //     if core and core.on_error then
  //         pcall(core.on_error, err)
  //     end
  //     os.exit(1)
  //   end
  // )
  (void) luaL_dostring(L,
    "local core\n"
    "xpcall(function()\n"
    "  SCALE = tonumber(os.getenv(\"LITE_SCALE\")) or SCALE\n"
    "  PATHSEP = package.config:sub(1, 1)\n"
    "  EXEDIR = EXEFILE:match(\"^(.+)[/\\\\].*$\")\n"
    "  package.path = EXEDIR .. '/data/?.lua;' .. package.path\n"
    "  package.path = EXEDIR .. '/data/?/init.lua;' .. package.path\n"
    "  core = require('core')\n"
    "  core.init()\n"
    "  core.run()\n"
    "end, function(err)\n"
    "  print('Error: ' .. tostring(err))\n"
    "  print(debug.traceback(nil, 2))\n"
    "  if core and core.on_error then\n"
    "    pcall(core.on_error, err)\n"
    "  end\n"
    "  os.exit(1)\n"
    "end)");


  lua_close(L);
  SDL_DestroyWindow(window);

  return EXIT_SUCCESS;
}
