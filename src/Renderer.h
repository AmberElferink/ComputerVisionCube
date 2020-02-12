#pragma once

#include <memory>
#include <string_view>

typedef void *SDL_GLContext;
struct SDL_Window;

struct SDLDestroyer {
  void operator()(SDL_GLContext context) const;
  void operator()(SDL_Window *window) const;
};

class Renderer {
public:
  /// Factory function. Returns null if there was an error
  static std::unique_ptr<Renderer> create(const std::string_view &title,
                                          uint32_t width, uint32_t height);

  virtual ~Renderer();

  /// Swap the backbuffer to screen so draw to the screen is done on new
  /// backbuffer
  void swapBuffers();

  SDL_Window* getNativeWindowHandle() const;

private:
  Renderer(SDL_Window *window, SDL_GLContext context);

  /// keeps track of SDL_Window and destroys it when it ceases to exist
  const std::unique_ptr<SDL_Window, SDLDestroyer> window_;
  const std::unique_ptr<void, SDLDestroyer> context_;
};
