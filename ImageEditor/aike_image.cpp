
AikeImage *aike_get_image_slot(Aike *aike)
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

void aike_open_image(Aike *aike, uint32_t width, uint32_t height, uint32_t numcomps, void *memory)
{
    AikeImage *imgSl = aike_get_image_slot(aike);
    imgSl->width = width;
    imgSl->height = height;
    imgSl->numComps = numcomps;
    //u64 size = width * height * numcomps;
    // TODO: should we copy and alloc our own buffer?
    imgSl->rawData = memory;
    imgSl->glTex = opengl_load_texture(width, height, numcomps, memory);
}
