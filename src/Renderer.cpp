#include "Renderer.h"
#include "../thirdparty/imgui-filebrowser/ImGuiFileBrowser.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h> //basic opengl
#include <glad/glad.h>
#include <imgui.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>

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
            (type==GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
            message);
    std::fflush(stderr);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
    ImGui_ImplOpenGL3_Init();

    return std::unique_ptr<Renderer>(new Renderer(window, context));
}

Renderer::Renderer(SDL_Window *window, SDL_GLContext context)
    : window_(window), context_(context) {}

Renderer::~Renderer() {
    ImGui::DestroyContext();
    SDL_Quit();
}

void Renderer::ProcessEventsUi(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void Renderer::DrawUi() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_.get());
    ImGui::NewFrame();

    static bool show_save_dialog = false;
    static imgui_addons::ImGuiFileBrowser fileDialog;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Calibration Files Path")) {
                show_save_dialog = true;
            }
            ImGui::EndMenu();
        }


        if (show_save_dialog) {
            ImGui::OpenPopup("Save Calibration Files");
            show_save_dialog = false;
        }

        if (fileDialog.showSaveFileDialog("Save Calibration Files", ImVec2(600, 300), ".png"))
        {
            printf("%s\n", fileDialog.selected_fn.c_str());
        }

        ImGui::EndMainMenuBar();
    }


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::swapBuffers() {
    glFinish();
    SDL_GL_SwapWindow(window_.get());
}
