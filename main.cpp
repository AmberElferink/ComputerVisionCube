#include <cstdlib>
#include <SDL.h>
#include <glad/glad.h>
#include <string_view>

#include "IndexedMesh.h"
#include "Renderer.h"

//just copy a glsl file in here with the vertex shader
constexpr std::string_view vertexShaderSource = "#version 460 core\n"
                                        "layout (location = 0) in vec2 screenCoordinate;\n"
                                        "layout (location = 0) out vec2 textureCoordinate;\n"
                                        "\n"
                                        "void main()\n"
                                        "{\n"
                                        "    gl_Position = vec4(mix(vec2(-1.0, -1.0), vec2(1.0, 1.0), screenCoordinate),  0.0, 1.0); //transform screen coordinate system to gl coordinate system\n"
                                        "    textureCoordinate = screenCoordinate;\n"
                                        "}\n";

//fragment shader chooses color for each pixel in the frameBuffer
constexpr std::string_view fragmentShaderSource = "#version 460 core\n"
                                            "layout (location = 0) in vec2 textureCoordinate;\n"
                                            "layout (location = 0) out vec4 color;\n"
                                            "void main()\n"
                                            "{\n"
                                            "    color = vec4(textureCoordinate.x, textureCoordinate.y, 0, 1);\n"
                                            "}\n";

int main() {
    int screenWidth = 800;
    int screenHeight = 600;

    auto renderer = Renderer::create("Calibration", 800, 600);
    if (!renderer) {
      return EXIT_FAILURE;
    }

    auto fullscreenQuad = IndexedMesh::createFullscreenQuad("fullscreen quad");

    //pipeline
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glObjectLabel(GL_SHADER, fragmentShader, -1, "fullscreenBlit fragment shader");
    glObjectLabel(GL_SHADER, vertexShader, -1, "fullscreenBlit vertex shader");
    {
        auto src = vertexShaderSource.data();
        glShaderSource(vertexShader, 1, &src, nullptr);
        glCompileShader(vertexShader);
        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        char infoLog[512];
        if(success == 0)
        {
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::fprintf(stderr, "%s\n", infoLog);
            return EXIT_FAILURE;
        }
    }

    {
        auto src = fragmentShaderSource.data();
        glShaderSource(fragmentShader, 1, &src, nullptr);
        glCompileShader(fragmentShader);
        int success;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        char infoLog[512];
        if(success == 0)
        {
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::fprintf(stderr, "%s\n", infoLog);
            return EXIT_FAILURE;
        }
    }

    uint32_t fullScreenBlit = glCreateProgram();
    glObjectLabel(GL_PROGRAM, fullScreenBlit, -1, "fullscreen shader");
    glAttachShader(fullScreenBlit, vertexShader); //link the different shaders to one program (the program you see in renderdoc)
    glAttachShader(fullScreenBlit, fragmentShader);
    glLinkProgram(fullScreenBlit);

    {
        char infoLog[512];
        int success;
        glGetProgramiv(fullScreenBlit, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(fullScreenBlit, 512, nullptr, infoLog);
            std::fprintf(stderr, "%s\n", infoLog);
            return EXIT_FAILURE;
        }
    }
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);



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

        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(1.f, 1.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(fullScreenBlit);

        // tell it you want to draw 2 triangles (2 vertices)
        fullscreenQuad->draw();

        renderer->swapBuffers();
    }
    glDeleteProgram(fullScreenBlit);
    return EXIT_SUCCESS;
}
