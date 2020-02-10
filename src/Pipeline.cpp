#include "Pipeline.h"

#include <glad/glad.h>

std::unique_ptr<Pipeline> Pipeline::create(const Pipeline::CreateInfo& info) {
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (vertexShader < 0) {
        return nullptr;
    }
    {
        auto src = info.VertexShaderSource.data();
        glShaderSource(vertexShader, 1, &src, nullptr);
        glCompileShader(vertexShader);
        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        char infoLog[512];
        if (success == 0) {
            glGetShaderInfoLog(vertexShader, sizeof(infoLog), nullptr, infoLog);
            std::fprintf(stderr, "%s\n", infoLog);
            return nullptr;
        }
    }
    if (!info.DebugName.empty()) {
        glObjectLabel(
            GL_SHADER, vertexShader, -1,
            (info.DebugName.data() + std::string(" vertex shader")).data());
    }

    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (fragmentShader < 0) {
        glDeleteShader(vertexShader);
        return nullptr;
    }
    {
        auto src = info.FragmentShaderSource.data();
        glShaderSource(fragmentShader, 1, &src, nullptr);
        glCompileShader(fragmentShader);
        int success;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        char infoLog[512];
        if (success == 0) {
            glGetShaderInfoLog(fragmentShader, sizeof(infoLog), nullptr,
                               infoLog);
            std::fprintf(stderr, "%s\n", infoLog);
            return nullptr;
        }
    }
    if (!info.DebugName.empty()) {
        glObjectLabel(
            GL_SHADER, fragmentShader, -1,
            (info.DebugName.data() + std::string(" fragment shader")).data());
    }

    // link the different shaders to one program (the program you see in
    // RenderDoc)
    uint32_t program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    {
        char infoLog[512];
        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::fprintf(stderr, "%s\n", infoLog);
            return nullptr;
        }
    }
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    if (!info.DebugName.empty()) {
        glObjectLabel(GL_PROGRAM, program, -1, info.DebugName.data());
    }

    return std::unique_ptr<Pipeline>(
        new Pipeline(program, info.ViewportWidth, info.ViewportHeight));
}

Pipeline::Pipeline(uint32_t program, uint32_t viewportWidth,
                   uint32_t viewportHeight)
    : program_(program), viewportWidth_(viewportWidth),
      viewportHeight_(viewportHeight) {}

Pipeline::~Pipeline() { glDeleteProgram(program_); }

void Pipeline::bind() {
    glViewport(0, 0, viewportWidth_, viewportHeight_);
    glUseProgram(program_);
}

template <>
bool Pipeline::setUniform(const std::string_view& uniform_name, const mat4& uniform)
{
    auto index = glGetUniformLocation(program_, uniform_name.data());
    if (index == GL_INVALID_INDEX) {
        std::fprintf(stderr, "Could not bind uniform %s: name not present\n", uniform_name.data());
        return false;
    }
    glProgramUniformMatrix4fv(program_, index, 1, false, uniform);

    return true;
}
