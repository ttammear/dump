#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#ifdef AIKE_X86
#include <xmmintrin.h>
#endif

// TODO: tt_simd?
#define TT_SIMD_32_WIDTH   4

#define TT_HELPERS_IMPLEMENTATION
#include "libs/tt_helpers.h"
#include "libs/tt_types.h"
#include "libs/tt_mt_ring_queue.h"

// the implementations I don't plan to change go into libs_static.c
// and they aren't recompiled to keep iteration times fast

//#define STB_SPRINTF_IMPLEMENTATION
#include "libs/stb_sprintf.h"
//#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"


//#define NK_IMPLEMENTATION
#define NK_INCLUDE_FONT_BAKING 
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_STANDARD_VARARGS
#include "libs/nuklear.h"

#include "libs/klib/khash.h"
KHASH_MAP_INIT_STR(str, void*)
KHASH_MAP_INIT_INT64(64, void*)
KHASH_MAP_INIT_INT(uint32, uint32_t)

#include "aike_platform.h"

#include "debug.h"
#ifdef _DEBUG
#include "debug.c"
#endif

#include "mathsc.h"
#include "test.h"
#include "memory.h"
#include "tess.h"

#include "renderer.h"

#include "memory.c"
#include "renderer.c"
#include "test.c"

#include "resourceformat.c"
#include "render_system.c"
#include "tess.c"
#include "client.c"

#include "menu.c"

#include "editor_server_client_shared.c"
#include "server.c"

#include "asset.c"
#include "world.c"
#include "input.c"
#include "ui.c"
#include "editor.c"

#include "game.c"
#include "fib.c"

#include "opengl_renderer.h"
#include "opengl_renderer.c"

#ifdef _DEBUG
static_assert(__COUNTER__ < PROFILER_MAX_ENTRIES_PER_FRAME, "not enough profiler slots");
#endif
