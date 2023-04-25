
#ifdef __cplusplus
extern "C" {
#endif

void hello(const char *str);

void aike_wrapper_set_window(struct ANativeWindow *window);

void aike_wrapper_create();
void aike_wrapper_destroy();
void aike_wrapper_start();
void aike_wrapper_stop();
void aike_wrapper_pause();
void aike_wrapper_resume();

#ifdef __cplusplus
}
#endif
