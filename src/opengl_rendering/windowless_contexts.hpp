#ifndef OPENGL_RENDERING_WINDOWLESS_CONTEXTS_HPP
#define OPENGL_RENDERING_WINDOWLESS_CONTEXTS_HPP

#include <mutex>
#include <vector>

#include <Corrade/PluginManager/Manager.h>

#include <Magnum/Platform/WindowlessEglApplication.h>

#define get_gl_context_with_sleep(name, ms_sleep, should_create, gpu, N_gpus) \
    /* Create/Get GLContext */                                                \
    Corrade::Utility::Debug name##_magnum_silence_output{nullptr};            \
    Magnum::Platform::WindowlessGLContext* name = nullptr;                    \
    if (should_create) {                                                      \
        while (name == nullptr) {                                             \
            name = GlobalGLContexts::instance().gl_context(gpu, N_gpus);      \
            /* Sleep for some ms */                                           \
            usleep(ms_sleep * 1000);                                          \
        }                                                                     \
        while (!name->makeCurrent()) {                                        \
            /* Sleep for some ms */                                           \
            usleep(ms_sleep * 1000);                                          \
        }                                                                     \
    }                                                                         \
                                                                              \
    Magnum::Platform::GLContext name##_magnum_context{Magnum::NoCreate};      \
    if (should_create) {                                                      \
        (name##_magnum_context).create();                                     \
    }

#define get_gl_context(name, should_create, gpu, N_gpus) get_gl_context_with_sleep(name, 0, should_create, gpu, N_gpus)

// the following assume an already created array of contexts (or creates an array with 1 GPU)
#define get_gl_context_select(name, gpu) get_gl_context_with_sleep(name, 0, true, gpu, 1)
#define get_gl_context_select_with_sleep(name, ms_sleep, gpu) get_gl_context_with_sleep(name, ms_sleep, true, gpu, 1)
#define get_gl_context_select_with_sleep_and_creation_check(name, ms_sleep, should_create, gpu) get_gl_context_with_sleep(name, ms_sleep, should_create, gpu, 1)
#define get_gl_context_single(name) get_gl_context_with_sleep(name, 0, true, 0, 1)

#define release_gl_context(name) GlobalGLContexts::instance().free_gl_context(name);

struct GlobalGLContexts {
public:
    static GlobalGLContexts& instance();

    GlobalGLContexts(const GlobalGLContexts&) = delete;
    void operator=(const GlobalGLContexts&) = delete;

    Magnum::Platform::WindowlessGLContext* gl_context(std::size_t gpu, std::size_t N_gpus = 1);
    void free_gl_context(Magnum::Platform::WindowlessGLContext* context);

    /* You should call this before starting to draw or after finished */
    void set_max_contexts(std::size_t N, std::size_t N_gpus = 1);
    void set_max_contexts(const std::vector<std::size_t>& gpus);

private:
    GlobalGLContexts() = default;
    ~GlobalGLContexts() = default;

    void _create_contexts(std::size_t N_gpus = 1);
    void _create_contexts(const std::vector<std::size_t>& gpus);

    std::vector<Magnum::Platform::WindowlessGLContext> _gl_contexts;
    std::vector<bool> _used;
    std::vector<std::size_t> _gpu_id;
    std::mutex _context_mutex;
    std::size_t _max_contexts = 4;
};

#endif
