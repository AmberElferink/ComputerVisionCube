#include "Ui.h"
#include "Calibration.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>
#include <ImGuiFileBrowser.h>
#include <cstring>

void ImGuiDestroyer::operator()(ImGuiContext* context) const {
    ImGui::DestroyContext(context);
}

std::unique_ptr<Ui> Ui::create(SDL_Window* window) {
    if (!IMGUI_CHECKVERSION()) {
        return nullptr;
    }
    std::unique_ptr<ImGuiContext, ImGuiDestroyer> context;
    {
        auto ctx = ImGui::CreateContext();
        if (ctx == nullptr) {
            return nullptr;
        }
        context.reset(ctx);
    }

    // Setup Platform/Renderer bindings
    if (!ImGui_ImplSDL2_InitForOpenGL(window, nullptr)) {
        return nullptr;
    }
    if (!ImGui_ImplOpenGL3_Init()) {
        return nullptr;
    }

    return std::unique_ptr<Ui>(new Ui(std::move(context)));
}

Ui::Ui(std::unique_ptr<ImGuiContext, ImGuiDestroyer>&& context)
    : context_(std::move(context))
    , show_save_dialog_(false)
    , folderDialog_(std::make_unique<imgui_addons::ImGuiFileBrowser>())
    , CalibrationDirectoryPath{0}
{
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "UserData";
    ini_handler.TypeHash = ImHashStr("UserData");
    ini_handler.ReadOpenFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) -> void* {
      ImGuiWindowSettings* settings = ImGui::FindWindowSettings(ImHashStr(name));
      return (void*)settings;
    };
    ini_handler.ReadLineFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) -> void {
      auto ui = reinterpret_cast<Ui*>(handler->UserData);
      char directoryPath[0x400];
      if (sscanf(line, "DirectoryPath=%1023[^\n]", directoryPath) != EOF)
          std::memcpy(ui->CalibrationDirectoryPath, directoryPath, sizeof(CalibrationDirectoryPath));
    };
    ini_handler.WriteAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) -> void {
      auto ui = reinterpret_cast<Ui*>(handler->UserData);
      ImGuiContext& g = *GImGui;

      buf->appendf("[%s][%s]\n", handler->TypeName, "Configuration");
      buf->appendf("DirectoryPath=%s\n", ui->CalibrationDirectoryPath);
    };
    ini_handler.UserData = this;
    ImGuiContext& g = *GImGui;
    g.SettingsHandlers.push_back(ini_handler);
}

Ui::~Ui() = default;

void Ui::processEvent(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void Ui::draw(SDL_Window* window, Calibration& calibration, int cameraWidth, int cameraHeight) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    if (ImGui::Begin("Configuration ")) {
        ImGui::Text("Offline calibration directory");
        ImGui::SameLine();
        ImGui::InputText("##Offline calibration files", CalibrationDirectoryPath, sizeof(CalibrationDirectoryPath));
        ImGui::SameLine();
        show_save_dialog_ = ImGui::Button("...##SelectDirectory");
        if (ImGui::Button("Calibrate Cameras (R key)")) {
            if (strcmp(CalibrationDirectoryPath, "") != 0)
                calibration.LoadFromSaved(CalibrationDirectoryPath);
            calibration.CalcCameraMat(cv::Size(cameraWidth, cameraHeight));

            calibration.PrintResults();
        }
        if (ImGui::BeginChild("Calibration Matrix", ImVec2(300, -ImGui::GetTextLineHeightWithSpacing()))) {
            ImGui::Text("Calibration Matrix");
            ImGui::InputFloat4("##calibration0", calibration.cameraMat + 0);
            ImGui::InputFloat4("##calibration1", calibration.cameraMat + 4);
            ImGui::InputFloat4("##calibration2", calibration.cameraMat + 8);
            ImGui::InputFloat4("##calibration3", calibration.cameraMat + 12);
        }
        ImGui::EndChild();
    }
    ImGui::End();


    if (show_save_dialog_) {
        ImGui::OpenPopup("Select Calibration Files Path");
        show_save_dialog_ = false;
    }

    if (folderDialog_->showSelectDirectoryDialog("Select Calibration Files Path", ImVec2(600, 300)))
    {
        std::printf("%s\n", folderDialog_->selected_fn.c_str());
        std::strncpy(CalibrationDirectoryPath, folderDialog_->selected_fn.c_str(), std::min(sizeof(CalibrationDirectoryPath), folderDialog_->selected_fn.length()));
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
