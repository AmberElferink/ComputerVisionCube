#include "Ui.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>
#include <ImGuiFileBrowser.h>

#include "Calibration.h"
#include "Texture.h"

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
    , CalibrationDirectoryPath{"C:/Users/eempi/CLionProjects/INFOMCV_calibration/calibImages/"}
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
          std::memcpy(ui->CalibrationDirectoryPath, directoryPath, sizeof(ui->CalibrationDirectoryPath));
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

void Ui::draw(SDL_Window *window, Calibration &calibration, float *objectMatrix, float *lightPos, float &squareSideLengthM, bool &saveNextImage)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    ImGui::SetNextWindowBgAlpha(0.4f);
    if (ImGui::Begin("Configuration", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTabBar("Tab bar")) {
            if (ImGui::BeginTabItem("Offline")) {
                if (ImGui::CollapsingHeader("Load saved calibration", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::InputText("##Calibration files", CalibrationDirectoryPath, sizeof(CalibrationDirectoryPath));
                    ImGui::SameLine();
                    show_save_dialog_ = ImGui::Button("...##SelectDirectory");
                    if (ImGui::Button("Take Capture (S key)")) {
                        if (strcmp(CalibrationDirectoryPath, "") != 0)
                            saveNextImage = true;
                    }
                    if (ImGui::Button("Calibrate Cameras (R key)")) {
                        if (strcmp(CalibrationDirectoryPath, "") != 0)
                            calibration.LoadFromDirectory(CalibrationDirectoryPath);
                    }
                }

                if (ImGui::CollapsingHeader("Intrinsic Matrix")) {
                    ImGui::InputScalarN("##intrinsic_matrix_0", ImGuiDataType_Double, calibration.CameraMatrix.data + 0 * sizeof(double), 3, nullptr, nullptr, "%0.3f");
                    ImGui::InputScalarN("##intrinsic_matrix_1", ImGuiDataType_Double, calibration.CameraMatrix.data + 3 * sizeof(double), 3, nullptr, nullptr, "%0.3f");
                    ImGui::InputScalarN("##intrinsic_matrix_2", ImGuiDataType_Double, calibration.CameraMatrix.data + 6 * sizeof(double), 3, nullptr, nullptr, "%0.3f");
                }

                if (ImGui::CollapsingHeader("Projection Matrix")) {
                    ImGui::InputFloat4("##projection_matrix_0", calibration.ProjMat + 0);
                    ImGui::InputFloat4("##projection_matrix_1", calibration.ProjMat + 4);
                    ImGui::InputFloat4("##projection_matrix_2", calibration.ProjMat + 8);
                    ImGui::InputFloat4("##projection_matrix_3", calibration.ProjMat + 12);
                }

                if (ImGui::CollapsingHeader("Distortion Coefficients")) {
                    for (uint32_t i = 0; i < calibration.DistortionCoefficients.rows; ++i) {
                        ImGui::InputDouble(("##distortion" + std::to_string(i)).c_str(), &calibration.DistortionCoefficients.at<double>(i, 0));
                    }
                }

                uint32_t numFiles = std::min(
                    static_cast<uint32_t>(std::min(calibration.CalibImages.size(), calibration.CalibImageNames.size())),
                    static_cast<uint32_t>(std::min(calibration.InitialRotationVectors.size[0], calibration.InitialTranslationVectors.size[0]))
                );
                if (numFiles > 0 && ImGui::CollapsingHeader(("Calibration Files (" + std::to_string(numFiles) + ")").c_str())) {
                    for (uint32_t i = 0; i < numFiles; ++i)
                    {
                        if (ImGui::CollapsingHeader(calibration.CalibImageNames[i].c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                            if (calibration.CalibImages[i])
                            {
                                ImGui::Image(reinterpret_cast<ImTextureID>(calibration.CalibImages[i]->getNativeHandle()), ImVec2(256, 256 / calibration.CalibImages[i]->getAspect()));
                            }
                            ImGui::InputScalarN(("rvec##rvec" + std::to_string(i)).c_str(), ImGuiDataType_Double, calibration.InitialRotationVectors.row(i).data, 3, nullptr, nullptr, "%.5f", ImGuiInputTextFlags_ReadOnly);
                            ImGui::InputScalarN(("tvec##tvec" + std::to_string(i)).c_str(), ImGuiDataType_Double, calibration.InitialTranslationVectors.row(i).data, 3, nullptr, nullptr, "%.5f", ImGuiInputTextFlags_ReadOnly);
                        }
                    }
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Online")) {
                ImGui::InputFloat( "Square Side Length (M)", &squareSideLengthM);
                if (ImGui::CollapsingHeader("Object Matrix", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::InputFloat4("##object_matrix_0", objectMatrix + 0);
                    ImGui::InputFloat4("##object_matrix_1", objectMatrix + 4);
                    ImGui::InputFloat4("##object_matrix_2", objectMatrix + 8);
                    ImGui::InputFloat4("##object_matrix_3", objectMatrix + 12);
                }
                ImGui::InputFloat3( "Light Position", lightPos);

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
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
