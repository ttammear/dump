
void aike_init_tile_pool(AikeTilePool *pool)
{
    pool->numFree = AIKE_TILE_POOL_INIT_SIZE;
    for(uint32_t i = 0; i < AIKE_TILE_POOL_INIT_SIZE; i++)
        pool->freeList[i] = i;
    pool->glTextureArray = opengl_create_texture_array(AIKE_IMG_CHUNK_SIZE, AIKE_IMG_CHUNK_SIZE, AIKE_TILE_POOL_INIT_SIZE);
}

void aike_destroy_tile_pool(AikeTilePool *pool)
{
    pool->numFree = 0;
    opengl_destroy_texture_array(pool->glTextureArray);
}

// TODO: this should be renamed to specify that it's opengl tile, not the whole deal
ImageTile* aike_alloc_tile(Aike *aike)
{
    AikeTilePool *pool = &aike->tilePool;
    // TODO: CPU side tiles should be pooled like openGL side!
    ImageTile *tile = (ImageTile*)aike_alloc(sizeof(ImageTile));
    tile->glLayer = pool->freeList[--pool->numFree];
    assert(pool->numFree >= 0 && pool->numFree < 0xFFFF);
    return tile;
}

void aike_free_tile(Aike *aike, ImageTile *tile)
{
    AikeTilePool *pool = &aike->tilePool;
    pool->freeList[pool->numFree++] = tile->glLayer;
    assert(pool->numFree <= AIKE_TILE_POOL_INIT_SIZE);
    aike_free(tile);
}

void aike_update_tile(Aike *aike, ImageTile *tile)
{
    AikeTilePool *pool = &aike->tilePool;
    opengl_copy_texture_array_layer(pool->glTextureArray, AIKE_IMG_CHUNK_SIZE, AIKE_IMG_CHUNK_SIZE, tile->glLayer, (void*)tile->data);
}

ImageTile* aike_get_tile(Aike *aike, AikeImage *img, int32_t x, int32_t y)
{
    uint32_t thash = (((int16_t)y)<<16) | (int16_t)x;
    khiter_t key = kh_get(ptr_t, img->tile_hashmap, thash);
    if(key != kh_end(img->tile_hashmap))
    {
        ImageTile *tile = (ImageTile*)kh_value(img->tile_hashmap, key);
        return tile;
    }
    return NULL;
}

AikeImage *aike_alloc_image_slot(Aike *aike)
{
    for(int i = 0; i < ARRAY_COUNT(aike->images.images); i++)
    {
        if(!aike->images.imagePresent[i])
        {
            aike->images.imagePresent[i] = true;
            return &aike->images.images[i];
        }
    }
    // TODO: i don't think hard crash here is a valid response
    return NULL;
}

void aike_free_image_slot(Aike *aike, AikeImage *slot)
{
    uint32_t index = slot - aike->images.images;
    assert(index < ARRAY_COUNT(aike->images.images));
    // mark as available
    aike->images.imagePresent[index] = true;
}

AikeImage *aike_get_first_image(Aike *aike)
{
    for(int i = 0; i < ARRAY_COUNT(aike->images.images); i++)
    {
        if(aike->images.imagePresent[i])
        {
            return &aike->images.images[i];
        }
    }
    return NULL;
}

// TODO: hardcoded value badbadbad
AikeImage *aike_get_dummy_image(Aike *aike)
{
    if(aike->images.imagePresent[1])
    {
        return &aike->images.images[1];
    }
    return NULL;
}

static void aike_load_image_tiles(Aike *aike, AikeImage *img, void *data)
{
    uint32_t xtiles = (int)ceilf((float)img->width/AIKE_IMG_CHUNK_SIZE);
    uint32_t ytiles = (int)ceilf((float)img->height/AIKE_IMG_CHUNK_SIZE);
    uint32_t w = img->width;
    uint32_t h = img->height;
    uint32_t c = img->numComps;
    uint8_t *bdata = (uint8_t*)data;
    assert(c == 3 || c == 4); // TODO?
    for(uint32_t i = 0; i < xtiles; i ++)
    for(uint32_t j = 0; j < ytiles; j ++)
    {
        ImageTile *chunk = aike_alloc_tile(aike);
        for(uint32_t y = 0; y < AIKE_IMG_CHUNK_SIZE; y++)
        for(uint32_t x = 0; x < AIKE_IMG_CHUNK_SIZE; x++)
        {
//             uint32_t row = (xtiles-1-i)*AIKE_IMG_CHUNK_SIZE + y;
            uint32_t row = i*AIKE_IMG_CHUNK_SIZE + y;
            uint32_t col = j*AIKE_IMG_CHUNK_SIZE + x;
            if(row < w && col < h)
            {
                uintptr_t offset = row * w * c + col * c;
                chunk->data[y][x][0] = bdata[offset+0];
                chunk->data[y][x][1] = bdata[offset+1];
                chunk->data[y][x][2] = bdata[offset+2];
                chunk->data[y][x][3] = c == 3 ? 0xFF : bdata[offset+3];
            }
            else
            {
                chunk->data[y][x][0] = 0xFF;
                chunk->data[y][x][1] = 0xFF;
                chunk->data[y][x][2] = 0xFF;
                chunk->data[y][x][3] = 0x00;
            }
        }
        chunk->tileX = i;
        chunk->tileY = j;
        aike_update_tile(aike, chunk);
        uint32_t tileHash = (i << 16) | j;
        int32_t ret;
        khiter_t key = kh_put(ptr_t, img->tile_hashmap, tileHash, &ret);
        kh_value(img->tile_hashmap, key) = (void*)chunk;
    }
}

AikeImage* aike_open_image(Aike *aike, uint32_t width, uint32_t height, uint32_t numcomps, void *memory, bool seamless)
{
    AikeImage *imgSl = aike_alloc_image_slot(aike);
    imgSl->width = width;
    imgSl->height = height;
    imgSl->numComps = numcomps;
    //u64 size = width * height * numcomps;
    // TODO: should we copy and alloc our own buffer?
    imgSl->rawData = memory;
    // TODO: delete ME
    if(!seamless)
        imgSl->glTex = opengl_load_texture(width, height, numcomps, memory);
    else
        imgSl->glTex = opengl_load_seamless_texture(width, height, numcomps, memory);

    imgSl->tile_hashmap = kh_init(ptr_t);
    aike_load_image_tiles(aike, imgSl, memory);
    return imgSl;
}

// TODO: call this!!
void aike_close_image(Aike *aike, AikeImage *img)
{
    khiter_t k;
    khash_t(ptr_t) *h = img->tile_hashmap;
    // return all tiles back to pool and clear hashmap
    for (k = kh_begin(h); k != kh_end(h); ++k)
    {
        if (kh_exist(h, k)) 
        {
            ImageTile *tile = (ImageTile*)kh_value(h, k);
            aike_free_tile(aike, tile);
        }
    }

    kh_destroy(ptr_t, img->tile_hashmap);
    // TODO: delete opengl data
    // TODO: the image might not be loaded with stb_image
    stbi_image_free(img->rawData);
    aike_free_image_slot(aike, img);
}
