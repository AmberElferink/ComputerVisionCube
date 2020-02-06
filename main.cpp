#include <SDL.h>
#include <cstdlib>
#include <opencv2/core/opengl.hpp>
#include <opencv2/videoio.hpp>
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
    "    textureCoordinate = vec2(screenCoordinate.x, 1 - "
    "screenCoordinate.y);\n"
    "}\n";

// fragment shader chooses color for each pixel in the frameBuffer
constexpr std::string_view fragmentShaderSource =
    "#version 460 core\n"
    "layout (location = 0) in vec2 textureCoordinate;\n"
    "layout (location = 0) out vec4 color;\n"
    "uniform sampler2D ourTexture;\n"
    "void main()\n"
    "{\n"
    "    color = texture(ourTexture, textureCoordinate);\n"
    "}\n";

int main(int argc, char* argv[]) {
    // Select video source from command line argument 1 (an int)
    int videoSourceIndex = 0;
    if (argc > 1) {
        videoSourceIndex = std::stoi(argv[1]);
    }

    cv::VideoCapture videoSource;
    if (!videoSource.open(videoSourceIndex)) {
        std::fprintf(stderr, "Could not open video source on index %d\n",
                     videoSourceIndex);
        return EXIT_FAILURE;
    }
    uint32_t screenWidth = videoSource.get(cv::CAP_PROP_FRAME_WIDTH);
    uint32_t screenHeight = videoSource.get(cv::CAP_PROP_FRAME_HEIGHT);

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

    // Make sure opencv has the same context and the renderer
    if (cv::ocl::haveOpenCL()) {
        (void)cv::ogl::ocl::initializeContextFromGL();
    }

    cv::Mat frame;
    cv::ogl::Texture2D interopFrame;

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

        // Get frame from webcam
        videoSource >> frame;
        if (frame.empty()) {
            running = false;
            std::fprintf(stderr,
                         "Camera returned an empty frame... Quitting.\n");
        } else {
            interopFrame.copyFrom(frame);
        }

        fullscreenPass->bind();
        interopFrame.bind();
        fullscreenPipeline->bind();

        // tell it you want to draw 2 triangles (2 vertices)
        fullscreenQuad->draw();

        renderer->swapBuffers();
    }
    return EXIT_SUCCESS;
}
