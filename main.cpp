#include <cstdlib>
#include <SDL.h>
#include <string_view>

#include "IndexedMesh.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Renderer.h"

// just copy a glsl file in here with the vertex shader
constexpr std::string_view vertexShaderSource =
    "#version 460 core\n"
    "layout (location = 0) in vec2 screenCoordinate;\n"
    "layout (location = 0) out vec2 textureCoordinate;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(mix(vec2(-1.0, -1.0), vec2(1.0, 1.0), "
    "screenCoordinate),  0.0, 1.0); //transform screen coordinate system to gl "
    "coordinate system\n"
    "    textureCoordinate = screenCoordinate;\n"
    "}\n";

// fragment shader chooses color for each pixel in the frameBuffer
constexpr std::string_view fragmentShaderSource =
    "#version 460 core\n"
    "layout (location = 0) in vec2 textureCoordinate;\n"
    "layout (location = 0) out vec4 color;\n"
    "void main()\n"
    "{\n"
    "    color = vec4(textureCoordinate.x, textureCoordinate.y, 0, 1);\n"
    "}\n";

int main() {
    uint32_t screenWidth = 800;
    uint32_t screenHeight = 600;

    auto renderer = Renderer::create("Calibration", screenWidth, screenHeight);
    if (!renderer) {
      return EXIT_FAILURE;
    }

    auto fullscreenQuad = IndexedMesh::createFullscreenQuad("fullscreen quad");

    //pipeline
    Pipeline::CreateInfo pipelineInfo;
    pipelineInfo.ViewportWidth = screenWidth;
    pipelineInfo.ViewportHeight = screenHeight;
    pipelineInfo.VertexShaderSource = vertexShaderSource;
    pipelineInfo.FragmentShaderSource = fragmentShaderSource;
    pipelineInfo.DebugName = "fullscreen blit";
    auto fullscreenPipeline = Pipeline::create(pipelineInfo);

    RenderPass::CreateInfo passInfo;
    passInfo.ClearColor[0] = 0.0f;
    passInfo.ClearColor[1] = 0.0f;
    passInfo.ClearColor[2] = 0.0f;
    passInfo.ClearColor[3] = 1.0f;
    passInfo.DebugName = "full screen quad";
    auto fullscreenPass = RenderPass::create(passInfo);

    bool running = true;
    SDL_Event event;

    while(running)
    {
        //input
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT: //cross
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            running = false;
                            break;
                    }
            }
        }

        fullscreenPass->bind();
        fullscreenPipeline->bind();

        // tell it you want to draw 2 triangles (2 vertices)
        fullscreenQuad->draw();

        renderer->swapBuffers();
    }
    return EXIT_SUCCESS;
}
