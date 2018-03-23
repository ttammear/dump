#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

// TODO: tt_simd?
#define TT_SIMD_32_WIDTH   4


#include "debug.h"
#ifdef _DEBUG
#include "debug.c"
#endif

#define TT_HELPER_IMPL
#include "libs/tt_helpers.h"
#include "libs/tt_types.h"
#include "libs/tt_mt_ring_queue.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "libs/stb_sprintf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#include "aike_platform.h"

#include "mathsc.h"
#include "test.h"
#include "renderer.h"

#include "renderer.c"
#include "test.c"

#include "fib.c"
#include "opengl_renderer.c"

#ifdef _DEBUG
static_assert(__COUNTER__ < PROFILER_MAX_ENTRIES_PER_FRAME, "not enough profiler slots");
#endif
