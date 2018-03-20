#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#ifdef _DEBUG
#include "debug.h"
#include "debug.c"
#endif

#define TT_HELPER_IMPL
#include "libs/tt_helpers.h"
#include "libs/tt_types.h"

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
