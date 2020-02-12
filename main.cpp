#include <SDL2/SDL.h>
#include <cstdlib>
#include <glad/glad.h>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core/opengl.hpp>

#include <opencv2/videoio.hpp>
#include <string_view>

#include <opencv2/highgui.hpp> // saving images
#include <opencv2/imgproc.hpp> //for drawing lines
#include <Ui.h>

#include "Calibration.h"
#include "IndexedMesh.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Renderer.h"

constexpr float oneSquareMm = 2.3f;
const cv::Size patternSize = cv::Size(6, 9);

// just copy a glsl file in here with the vertex shader
constexpr std::string_view vertexShaderSource =
    "#version 450 core\n"
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
    "#version 450 core\n"
    "layout (location = 0) in vec2 textureCoordinate;\n"
    "layout (location = 0) out vec4 color;\n"
    "uniform sampler2D ourTexture;\n"
    "void main()\n"
    "{\n"
    "    color = texture(ourTexture, textureCoordinate).bgra;\n"
    "}\n";

constexpr std::string_view axisVertexShaderSource =
    "#version 450 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "layout (location = 0) out vec4 out_color;\n"
    "uniform mat4 rotTransMat;\n"
    "uniform mat4 cameraMat;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = cameraMat * rotTransMat * vec4(position, 1.0);\n"
    "    out_color = vec4(color, 1.0);\n"
    "}\n";

constexpr std::string_view axisFragmentShaderSource =
    "#version 450 core\n"
    "layout (location = 0) in vec4 in_color;\n"
    "layout (location = 0) out vec4 out_color;\n"
    "void main()\n"
    "{\n"
    "    out_color = in_color;\n"
    "}\n";

int main(int argc, char* argv[]) {
    // Select video source from command line argument 1 (an int)
    int videoSourceIndex = 0;
    if (argc > 1) {
        videoSourceIndex = std::stoi(argv[1]);
    }

    cv::ocl::setUseOpenCL(true);
    if (!cv::ocl::haveOpenCL()) {
        std::printf("OpenCL is not available...\n");
        return EXIT_FAILURE;
    }

    cv::ocl::Context context;
    if (!context.create(cv::ocl::Device::TYPE_GPU)) {
        std::printf("Failed creating the context...\n");
        return EXIT_FAILURE;
    }

    std::printf("%zu GPU devices are detected.\n", context.ndevices());
    for (int i = 0; i < context.ndevices(); i++) {
        const cv::ocl::Device& device = context.device(i);
        std::printf("name: %s\n"
                    "available: %s\n"
                    "imageSupport: %s\n"
                    "OpenCL_C_Version: %s\n",
                    device.name().c_str(),
                    device.available() ? "true" : "false",
                    device.imageSupport() ? "true" : "false",
                    device.OpenCL_C_Version().c_str());
    }

    cv::VideoCapture videoSource;
    if (!videoSource.open(videoSourceIndex)) {
        std::fprintf(stderr, "Could not open video source on index %d\n",
                     videoSourceIndex);
        return EXIT_FAILURE;
    }
    uint32_t screenWidth = videoSource.get(cv::CAP_PROP_FRAME_WIDTH);
    uint32_t screenHeight = videoSource.get(cv::CAP_PROP_FRAME_HEIGHT);
    cv::Size screenSize = cv::Size(screenWidth, screenHeight);

    auto renderer = Renderer::create("Calibration", screenWidth, screenHeight);
    if (!renderer) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return EXIT_FAILURE;
    }
    auto ui = Ui::create(renderer->getNativeWindowHandle());
    if (!renderer) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return EXIT_FAILURE;
    }

    auto fullscreenQuad = IndexedMesh::createFullscreenQuad("fullscreen quad");
    auto axis = IndexedMesh::createAxis("axis");

    // pipelines
    Pipeline::CreateInfo fullScreenPipelineInfo;
    fullScreenPipelineInfo.ViewportWidth = screenWidth;
    fullScreenPipelineInfo.ViewportHeight = screenHeight;
    fullScreenPipelineInfo.VertexShaderSource = vertexShaderSource;
    fullScreenPipelineInfo.FragmentShaderSource = fragmentShaderSource;
    fullScreenPipelineInfo.DebugName = "fullscreen blit";
    auto fullscreenPipeline = Pipeline::create(fullScreenPipelineInfo);
    if (!fullscreenPipeline) {
        std::fprintf(stderr, "Failed to create fullscreen pipeline\n");
        return EXIT_FAILURE;
    }

    Pipeline::CreateInfo axisPipelineInfo;
    axisPipelineInfo.ViewportWidth = screenWidth;
    axisPipelineInfo.ViewportHeight = screenHeight;
    axisPipelineInfo.VertexShaderSource = axisVertexShaderSource;
    axisPipelineInfo.FragmentShaderSource = axisFragmentShaderSource;
    axisPipelineInfo.DebugName = "axis";
    auto axisPipeline = Pipeline::create(axisPipelineInfo);
    if (!axisPipeline) {
        std::fprintf(stderr, "Failed to create axis pipeline\n");
        return EXIT_FAILURE;
    }

    RenderPass::CreateInfo passInfo;
    passInfo.ClearColor[0] = 0.0f;
    passInfo.ClearColor[1] = 0.0f;
    passInfo.ClearColor[2] = 0.0f;
    passInfo.ClearColor[3] = 1.0f;
    passInfo.DebugName = "full screen quad";
    auto fullscreenPass = RenderPass::create(passInfo);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    cv::Mat frame;

    bool running = true;
    bool calibrateFrame = false;
    SDL_Event event;

    Calibration calibration(6, 9, 23);
    int calibFileCounter = 0;

    bool saveNextImage = false;
    std::string calibFileName;

    bool cameraMatKnown = false;
    //clang-format off
    mat4 cameraMat{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};

    mat4 rotTransMat{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};

    //clang-format on
    bool firstFrame = true;

    while (running) {
        calibrateFrame = false;
        // input
        while (SDL_PollEvent(&event)) {
            ui->processEvent(event);
            switch (event.type) {
            case SDL_QUIT: // cross
                running = false;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_c:
                    calibrateFrame = true;
                    break;
                case SDLK_r:
                    if (ui->CalibrationDirectoryPath != "")
                        calibration.LoadFromSaved(ui->CalibrationDirectoryPath);
                    calibration.CalcCameraMat(screenSize);
                    calibration.PrintResults();
                    break;
                case SDLK_s:
                    calibFileName = std::string(ui->CalibrationDirectoryPath) + "calib" + std::to_string(calibFileCounter) + ".png";
                    calibFileCounter++;
                    saveNextImage = true;
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
            return EXIT_FAILURE;
        }

        if (saveNextImage) {
            if (!cv::imwrite(calibFileName, frame)) {
                std::cout << "failed to save file\n";
            }
            saveNextImage = false;
            std::cout << "image saved\n";
        }

        calibration.DetectPattern(frame, calibrateFrame,
                                  true); // write calibration colors to image

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, frame.data);

        fullscreenPass->bind();
        glBindTexture(GL_TEXTURE_2D, texture);
        fullscreenPipeline->bind();

        // tell it you want to draw 2 triangles (2 vertices)
        fullscreenQuad->draw();

        axisPipeline->bind();

        if (calibration.cameraMatKnown) {
            if (calibration.UpdateRotTransMat(screenSize, rotTransMat, !firstFrame)) {

                firstFrame = false;

                axisPipeline->setUniform("rotTransMat", rotTransMat);
                axisPipeline->setUniform("cameraMat", calibration.cameraMat);

                axis->draw();
            }
        }

        ui->draw(renderer->getNativeWindowHandle(), calibration, screenWidth, screenHeight, rotTransMat);
        renderer->swapBuffers();
    }
    return EXIT_SUCCESS;
}
