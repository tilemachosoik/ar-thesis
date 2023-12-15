#include "windowless_contexts.hpp"

#include <Corrade/Utility/Resource.h>

#include <iostream>

GlobalGLContexts& GlobalGLContexts::instance()
{
    static GlobalGLContexts gdata;
    return gdata;
}

Magnum::Platform::WindowlessGLContext* GlobalGLContexts::gl_context(std::size_t gpu, std::size_t N_gpus)
{
    std::lock_guard<std::mutex> lg(_context_mutex);
    if (_gl_contexts.size() == 0) {
        _create_contexts(N_gpus);
    }

    for (std::size_t i = 0; i < _max_contexts; i++) {
        if (!_used[i] && _gpu_id[i] == gpu) {
            _used[i] = true;
            return &_gl_contexts[i];
        }
    }

    return nullptr;
}

void GlobalGLContexts::free_gl_context(Magnum::Platform::WindowlessGLContext* context)
{
    if (context == nullptr)
        return;
    std::lock_guard<std::mutex> lg(_context_mutex);
    for (std::size_t i = 0; i < _gl_contexts.size(); i++) {
        if (&_gl_contexts[i] == context) {
            while (!_gl_contexts[i].release()) {} // release the context
            _used[i] = false;
            break;
        }
    }
}

void GlobalGLContexts::set_max_contexts(std::size_t N, std::size_t N_gpus)
{
    std::lock_guard<std::mutex> lg(_context_mutex);
    _max_contexts = N;
    _create_contexts(N_gpus);
}

void GlobalGLContexts::set_max_contexts(const std::vector<std::size_t>& gpus)
{
    std::lock_guard<std::mutex> lg(_context_mutex);
    _max_contexts = gpus.size();
    _create_contexts(gpus);
}

void GlobalGLContexts::_create_contexts(std::size_t N_gpus)
{
    _used.clear();
    _gpu_id.clear();
    _gl_contexts.clear();
    _gl_contexts.reserve(_max_contexts);
    for (std::size_t i = 0; i < _max_contexts; i++) {
        _used.push_back(false);
        _gpu_id.push_back(i % N_gpus);
        Magnum::Platform::WindowlessGLContext::Configuration config;
        config.setDevice(_gpu_id.back());
        _gl_contexts.emplace_back(Magnum::Platform::WindowlessGLContext{config});
    }
}

void GlobalGLContexts::_create_contexts(const std::vector<std::size_t>& gpus)
{
    _used.clear();
    _gpu_id.clear();
    _gl_contexts.clear();
    _gl_contexts.reserve(_max_contexts);
    for (std::size_t i = 0; i < _max_contexts; i++) {
        _used.push_back(false);
        _gpu_id.push_back(gpus[i]);
        Magnum::Platform::WindowlessGLContext::Configuration config;
        config.setDevice(gpus[i]);
        _gl_contexts.emplace_back(Magnum::Platform::WindowlessGLContext{config});
        std::cout << "Creating glContext for gpu #" + std::to_string(gpus[i]) + "." << std::endl;
        if (!_gl_contexts.back().isCreated()) {
            std::cerr << "Failed to create glContext for gpu #" + std::to_string(gpus[i]) + "." << std::endl;
        }
    }
}
