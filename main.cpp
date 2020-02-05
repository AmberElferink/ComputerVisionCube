#include <cstdlib>
#include <SDL.h>
#include <SDL_video.h> //basic opengl
#include <glad/glad.h>
#include <memory>
#include <string_view>

struct SDLDestroyer
{
    void operator()(SDL_GLContext context) const
    {
        SDL_GL_DeleteContext(context);
    }
    void operator()(SDL_Window* window) const
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

void GLAPIENTRY
MessageCallback([[maybe_unused]] GLenum /*source*/, GLenum type, [[maybe_unused]] GLuint /*id*/, GLenum severity, [[maybe_unused]] GLsizei /*length*/, const GLchar* message, [[maybe_unused]] const void* /*userparam*/)
{
    fprintf(stderr,
            "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type,
            severity,
            message);
    std::fflush(stderr);
}

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

    SDL_Init(SDL_INIT_VIDEO);
    std::unique_ptr<SDL_Window, SDLDestroyer> window; //keeps track of SDL_Window and destroys it when it seizes to exist
    window.reset(SDL_CreateWindow("Calibration", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL));
    if(window == nullptr)
        return EXIT_FAILURE;

    float quad_vertices[] = {
            0.f, 0.f, // 0 top left
            1.0, 0.f, // 1 top right
            1.f, 1.f, // 2 bottom right
            0.f, 1.f, // 3 bottom left
    };
    static const uint16_t quad_indices[6] = {0, 1, 2, 2, 3, 0}; //drawing the quad

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    std::unique_ptr<void, SDLDestroyer> context;
    context.reset(SDL_GL_CreateContext(window.get()));
    if(context == nullptr) {
        return EXIT_FAILURE;
    }

    if( gladLoadGLLoader(SDL_GL_GetProcAddress) == 0) {
        return EXIT_FAILURE;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);

    uint32_t quadVertexBuffer;
    glCreateBuffers(1, &quadVertexBuffer); //create buffer pointer on gpu
    uint32_t indexBuffer;
    glCreateBuffers(1, &indexBuffer);
    //you can also create them at once if you have a bufferarray, then put 2

	glObjectLabel(GL_BUFFER, quadVertexBuffer, -1, "fullscreen quad vertexbuffer"); //to be able to read it in renderdoc/errors
	glObjectLabel(GL_BUFFER, indexBuffer, -1, "fullscreen quad indexbuffer"); //to be able to read it in renderdoc/errors


    uint32_t vao;
    glCreateVertexArrays(1, &vao);
	glObjectLabel(GL_VERTEX_ARRAY, vao, -1, "fullscreen quad vertex array object"); //to be able to read it in renderdoc/errors

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer); //binds buffers to the slot in the vao, and this makes no sense but is needed somehow
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	//tell renderdoc (and the gpu) you use float2 for the vertices at position 0 in the shader
	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float) * 2, nullptr); //define array dimensionality and also say you step per two floats
	glEnableVertexAttribArray(0);

    glNamedBufferData(quadVertexBuffer, sizeof(quad_vertices), nullptr, GL_STATIC_DRAW); // malloc without data, reserve space on gpu
    glNamedBufferData(indexBuffer, sizeof(quad_indices), nullptr, GL_STATIC_DRAW);

    {
        void* mappedMemory = glMapNamedBuffer(quadVertexBuffer, GL_WRITE_ONLY);
        memcpy(mappedMemory, quad_vertices, sizeof(quad_vertices));
        glUnmapNamedBuffer(quadVertexBuffer);
    }
    {
        void *mappedMemory = glMapNamedBuffer(indexBuffer, GL_WRITE_ONLY);
        memcpy(mappedMemory, quad_indices, sizeof(quad_indices));
        glUnmapNamedBuffer(indexBuffer);
    }

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

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr); //tell it you want to draw 2 triangles (2 vertices)


        glFinish();
        SDL_GL_SwapWindow(window.get()); //draw to the screen


    }
    glDeleteProgram(fullScreenBlit);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1, &quadVertexBuffer);
    return EXIT_SUCCESS;
}

