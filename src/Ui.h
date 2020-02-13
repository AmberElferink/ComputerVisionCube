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

struct ImGuiDestroyer {
  void operator()(ImGuiContext* context) const;
};

class Ui
{
public:
  static std::unique_ptr<Ui> create(SDL_Window* window);
  virtual ~Ui();

  void processEvent(const SDL_Event& event);
  void draw(SDL_Window *window, Calibration &calibration, int cameraWidth, int cameraHeight, float *objectMatrix,
            float *lightPos, bool& saveNextImage);

private:
  explicit Ui(std::unique_ptr<ImGuiContext, ImGuiDestroyer>&& context);

  std::unique_ptr<ImGuiContext, ImGuiDestroyer> context_;
  bool show_save_dialog_;
  std::unique_ptr<imgui_addons::ImGuiFileBrowser> folderDialog_;

public:
  char CalibrationDirectoryPath[0x400];
};
