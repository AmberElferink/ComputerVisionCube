#include "Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h> //basic opengl
#include <glad/glad.h>

void SDLDestroyer::operator()(SDL_GLContext context) const {
    SDL_GL_DeleteContext(context);
}

void SDLDestroyer::operator()(SDL_Window *window) const {
    SDL_DestroyWindow(window);
}

void GLAPIENTRY MessageCallback([[maybe_unused]] GLenum /*source*/, GLenum type,
                                [[maybe_unused]] GLuint /*id*/, GLenum severity,
                                [[maybe_unused]] GLsizei /*length*/,
                                const GLchar *message,
                                [[maybe_unused]] const void * /*userparam*/) {
//    fprintf(stderr,
//            "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
//            (type==GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
//            message);
//    std::fflush(stderr);
}

std::unique_ptr<Renderer> Renderer::create(const std::string_view &title,
                                           uint32_t width, uint32_t height) {
    if (SDL_Init(SDL_INIT_VIDEO)!=0) {
        std::fprintf(stderr, "SDL Init fFailed: %s\n",
                     SDL_GetError());
        return nullptr;
    }
    auto window = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, width, height,
                                   SDL_WINDOW_OPENGL);
    if (window==nullptr) {
        std::fprintf(stderr, "SDL Window Creation Failed: %s\n",
                     SDL_GetError());
        return nullptr;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    auto context = SDL_GL_CreateContext(window);
    if (context==nullptr) {
        std::fprintf(stderr, "SDL Context Creation Failed: %s\n", SDL_GetError());
        return nullptr;
    }

    if (gladLoadGLLoader(SDL_GL_GetProcAddress)==0) {
        return nullptr;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);

    return std::unique_ptr<Renderer>(new Renderer(window, context));
}

Renderer::Renderer(SDL_Window *window, SDL_GLContext context)
    : window_(window), context_(context) {}

Renderer::~Renderer() {
    SDL_Quit();
}

void Renderer::swapBuffers() {
    glFinish();
    SDL_GL_SwapWindow(window_.get());
}

SDL_Window *Renderer::getNativeWindowHandle() const {
    return window_.get();
}
