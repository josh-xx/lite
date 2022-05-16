cmake_minimum_required(VERSION 3.16)
project(lite)

set(CMAKE_C_STANDARD 99)

include_directories(lua/src)
include_directories(src)
link_directories(lua/src)
include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)
add_executable(lite
    src/main.c
    src/renderer.c
    src/rencache.c
    src/lib/stb/stb_truetype.c
    src/api/api.c
    src/api/renderer.c
    src/api/renderer_font.c
    src/api/system.c
)
target_link_libraries(lite SDL2 lua)
