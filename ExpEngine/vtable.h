typedef struct TessVtable
{
    void (*none)(void);
    void (*editorObjectId)(struct Renderer*, struct ObjectIDSamplesReady*, void*);
} TessVtable;

void tess_reload_vtable();

#ifdef TESS_VTABLE_IMPLEMENTATION

void objectid_callback(struct Renderer *renderer, struct ObjectIDSamplesReady *tdr, void *userData);

struct TessVtable *g_tessVtbl;
struct TessVtable tessVtbl;

void tess_reload_vtable()
{
    g_tessVtbl = &tessVtbl;
    g_tessVtbl->editorObjectId = objectid_callback;
};

#endif
