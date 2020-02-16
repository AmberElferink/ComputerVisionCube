#pragma once

#include <memory>

struct SDL_Window;
struct ImGuiContext;
union SDL_Event;
class Calibration;

namespace imgui_addons
{
class ImGuiFileBrowser;
}

/// RAII implementation of custom destructor of ImGuiContext so
/// it cleans itself up correctly when it goes out of scope.
struct ImGuiDestroyer {
  void operator()(ImGuiContext* context) const;
};

/// Wrapper around ImGui context
class Ui
{
public:
  /// Factory function to create and manage ImGui context
  /// Requires a native handle to the window.
  static std::unique_ptr<Ui> create(SDL_Window* window);
  virtual ~Ui();

  /// Process keyboard, mouse and window events for inputs
  void processEvent(const SDL_Event& event);
  /// Draw UI and update variables in immediate mode.
  /// Takes in the calibration object and other variables to display and edit their public variables.
  void draw(SDL_Window *window, Calibration &calibration, float *objectMatrix, float *lightPos, float &squareSideLengthM, bool &saveNextImage);

private:
  /// Private unique constructor forcing the use of factory function which
  /// can return null unlike constructor.
  explicit Ui(std::unique_ptr<ImGuiContext, ImGuiDestroyer>&& context);

  std::unique_ptr<ImGuiContext, ImGuiDestroyer> context_;
  bool show_save_dialog_;
  std::unique_ptr<imgui_addons::ImGuiFileBrowser> folderDialog_;

public:
  char CalibrationDirectoryPath[0x400];
};
