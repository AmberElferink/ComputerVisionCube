#include "Ui.h"

#include <imgui.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>
#include <ImGuiFileBrowser.h>

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
{
}

Ui::~Ui() = default;

void Ui::processEvent(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void Ui::draw(SDL_Window* window) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Calibration Files Path")) {
                show_save_dialog_ = true;
            }
            ImGui::EndMenu();
        }


        if (show_save_dialog_) {
            ImGui::OpenPopup("Save Calibration Files");
            show_save_dialog_ = false;
        }

        if (folderDialog_->showSaveFileDialog("Save Calibration Files", ImVec2(600, 300), ".png"))
        {
            std::printf("%s\n", folderDialog_->selected_fn.c_str());
        }

        ImGui::EndMainMenuBar();
    }


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
