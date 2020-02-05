#include "Renderer.h"

#include <SDL.h>
#include <SDL_video.h> //basic opengl
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
  fprintf(stderr,
          "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
          message);
  std::fflush(stderr);
}

std::unique_ptr<Renderer> Renderer::create(const std::string_view &title,
                                           uint32_t width, uint32_t height) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    return nullptr;
  }
  auto window = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, width, height,
                                 SDL_WINDOW_OPENGL);
  if (window == nullptr) {
    return nullptr;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

  auto context = SDL_GL_CreateContext(window);

  if (gladLoadGLLoader(SDL_GL_GetProcAddress) == 0) {
    return nullptr;
  }

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);

  return std::unique_ptr<Renderer>(new Renderer(window, context));
}

Renderer::Renderer(SDL_Window *window, SDL_GLContext context)
    : window_(window), context_(context) {}

Renderer::~Renderer() { SDL_Quit(); }

void Renderer::swapBuffers() {
  glFinish();
  SDL_GL_SwapWindow(window_.get());
}
