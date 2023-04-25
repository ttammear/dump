
#include "containers.cpp"

#include "aike_platform.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "libs/stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "libs/stb_image.h"

#include "libs/klib/khash.h"
KHASH_MAP_INIT_INT(ptr_t, void*)

#include "image_editor.h"

#define CONTAINERS_IMPLEMENTATION
#include "containers.cpp"

#include "aike.cpp"
#include "aike_image.cpp"
#include "renderer.cpp"
#include "ui.cpp"
#include "utility.cpp"
#include "input.cpp"
#include "events.cpp"
