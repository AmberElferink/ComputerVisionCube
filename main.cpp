#include <SDL2/SDL.h>
#include <cstdlib>
#include <iostream>
#include <opencv2/core/opengl.hpp>

#include <opencv2/videoio.hpp>
#include <string_view>

#include <opencv2/highgui.hpp> // saving images
#include <Ui.h>
#include <Texture.h>

#include "Calibration.h"
#include "IndexedMesh.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Renderer.h"

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
    "    color = texture(ourTexture, textureCoordinate);\n"
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

//this loops over the vertices only (not any surface in between)
constexpr std::string_view cubeVertexShaderSource =
        "#version 450 core\n"
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 1) in vec3 normal;\n"
        "layout (location = 0) out vec4 world_pos;\n"
        "layout (location = 1) out vec4 world_normal;\n"
        "uniform mat4 rotTransMat;\n"
        "uniform mat4 cameraMat;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    world_pos = rotTransMat * vec4(position, 1.0f);\n"
        "    world_normal = rotTransMat * vec4(normal, 0); //normal is not affected by translations, so 0 \n"
        "    gl_Position = cameraMat * world_pos;\n"
        "}\n";

//this loops over the surfaces in between, after the transform tu unit projection space, so basically the pixels.
constexpr std::string_view cubeFragmentShaderSource =
    "#version 450 core\n"
    "layout (location = 0) in vec4 position;\n"
    "layout (location = 1) in vec4 normal;\n"
    "layout (location = 0) out vec4 out_color;\n"
    "uniform vec3 lightPos;\n"
    "void main()\n"
    "{\n"
    "    vec4 dir = vec4(lightPos, 1.0) - position;\n"
    "    vec3 viewDir = -normalize(position.xyz);\n"
    "    float dist2 = dot(dir, dir);\n"
    "    dir = normalize(dir);\n"
    "    vec3 reflectDir = reflect(-dir.xyz, normal.xyz);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);\n"
    "    float lightIntensity = (clamp(dot(dir.xyz, normal.xyz), 0.0, 0.1) * 0.5 + spec * 0.25) / dist2  + 0.3;\n"
    "    out_color.rgb = lightIntensity * vec3(0.349f, 0.65f, 0.67f);\n"
    "    out_color.a = 1.0f;\n"
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
    cv::Size screenSize = cv::Size(videoSource.get(cv::CAP_PROP_FRAME_WIDTH), videoSource.get(cv::CAP_PROP_FRAME_HEIGHT));

    auto renderer = Renderer::create("Calibration", screenSize.width, screenSize.height);
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
    auto cube = IndexedMesh::createCube("cube");

    // pipelines
    Pipeline::CreateInfo fullScreenPipelineInfo;
    fullScreenPipelineInfo.ViewportWidth = screenSize.width;
    fullScreenPipelineInfo.ViewportHeight = screenSize.height;
    fullScreenPipelineInfo.VertexShaderSource = vertexShaderSource;
    fullScreenPipelineInfo.FragmentShaderSource = fragmentShaderSource;
    fullScreenPipelineInfo.DebugName = "fullscreen blit";
    auto fullscreenPipeline = Pipeline::create(fullScreenPipelineInfo);
    if (!fullscreenPipeline) {
        std::fprintf(stderr, "Failed to create fullscreen pipeline\n");
        return EXIT_FAILURE;
    }

    Pipeline::CreateInfo axisPipelineInfo;
    axisPipelineInfo.ViewportWidth = screenSize.width;
    axisPipelineInfo.ViewportHeight = screenSize.height;
    axisPipelineInfo.VertexShaderSource = axisVertexShaderSource;
    axisPipelineInfo.FragmentShaderSource = axisFragmentShaderSource;
    axisPipelineInfo.LineWidth = 2.0f;
    axisPipelineInfo.DebugName = "axis";
    auto axisPipeline = Pipeline::create(axisPipelineInfo);
    if (!axisPipeline) {
        std::fprintf(stderr, "Failed to create axis pipeline\n");
        return EXIT_FAILURE;
    }

    Pipeline::CreateInfo cubePipelineInfo;
    cubePipelineInfo.ViewportWidth = screenSize.width;
    cubePipelineInfo.ViewportHeight = screenSize.height;
    cubePipelineInfo.VertexShaderSource = cubeVertexShaderSource;
    cubePipelineInfo.FragmentShaderSource = cubeFragmentShaderSource;
    cubePipelineInfo.DebugName = "cube";
    auto cubePipeline = Pipeline::create(cubePipelineInfo);
    if (!cubePipeline) {
        std::fprintf(stderr, "Failed to create cube pipeline\n");
        return EXIT_FAILURE;
    }

    RenderPass::CreateInfo passInfo;
    passInfo.Clear = true;
    passInfo.ClearColor[0] = 0.0f;
    passInfo.ClearColor[1] = 0.0f;
    passInfo.ClearColor[2] = 0.0f;
    passInfo.ClearColor[3] = 1.0f;
    passInfo.DepthWrite = false;
    passInfo.DepthTest = false;
    passInfo.DebugName = "full screen quad";
    auto fullscreenPass = RenderPass::create(passInfo);

    passInfo.Clear = false;
    passInfo.DepthWrite = true;
    passInfo.DepthTest = true;
    auto objectPass = RenderPass::create(passInfo);

    passInfo.Clear = false;
    passInfo.DepthWrite = false;
    passInfo.DepthTest = false;
    auto axisPass = RenderPass::create(passInfo);

    auto texture = Texture::create(screenSize.width, screenSize.height);
    if (!texture) {
        std::fprintf(stderr, "Failed to create camera texture\n");
        return EXIT_FAILURE;
    }

    cv::Mat frame;

    bool running = true;
    bool calibrateFrame = false;
    SDL_Event event;

    float squareSideLengthM = 0.023;
    Calibration calibration(patternSize, screenSize, squareSideLengthM);

    bool saveNextImage = false;
    std::string calibFileName;

    //clang-format off
    mat4 rotTransMat{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};

    float3 lightPos = { 0.0f, 0.0f, 0.0f};

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
                    if (std::strcmp(ui->CalibrationDirectoryPath, "") != 0)
                        calibration.LoadFromDirectory(ui->CalibrationDirectoryPath);
                    break;
                case SDLK_s:
                    saveNextImage = true;
                    break;
                }
            }
        }

        // Get frame from webcam
        videoSource >> frame;
        if (frame.empty()) {
            std::fprintf(stderr,
                         "Camera returned an empty frame... Quitting.\n");
            return EXIT_FAILURE;
        }

        if (saveNextImage) {
            calibration.TakeCapture(ui->CalibrationDirectoryPath, frame);
            saveNextImage = false;
        }

        calibration.DetectPattern(frame, calibrateFrame,
                                  true); // write calibration colors to image

        texture->upload(frame);
        cubePipeline->setUniform( "lightPos", lightPos);
        bool drawObjects = false;
        if (calibration.UpdateRotTransMat(rotTransMat, squareSideLengthM, !firstFrame)) {
            axisPipeline->setUniform("rotTransMat", rotTransMat);
            axisPipeline->setUniform("cameraMat", calibration.CameraProjMat);
            cubePipeline->setUniform( "rotTransMat", rotTransMat);
            cubePipeline->setUniform( "cameraMat", calibration.CameraProjMat);
            drawObjects = true;
        }

        // tell it you want to draw 2 triangles (2 vertices)
        fullscreenPass->bind();
        fullscreenPipeline->bind();
        texture->bind();
        fullscreenQuad->draw();

        if (drawObjects) {
            objectPass->bind();
            cubePipeline->bind();
            cube->draw();

            axisPass->bind();
            axisPipeline->bind();
            axis->draw();
            firstFrame = false;
        }

        ui->draw(renderer->getNativeWindowHandle(), calibration, rotTransMat, lightPos, squareSideLengthM, saveNextImage);
        renderer->swapBuffers();
    }
    return EXIT_SUCCESS;
}
