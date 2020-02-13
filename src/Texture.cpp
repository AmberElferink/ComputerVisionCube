#include "Texture.h"

#include <glad/glad.h>
#include <opencv2/core/mat.hpp>

std::unique_ptr<Texture> Texture::create() {
    uint32_t handle = 0;
    glGenTextures(1, &handle);
    if (handle == 0)
    {
        return nullptr;
    }
    glBindTexture(GL_TEXTURE_2D, handle);

    return std::unique_ptr<Texture>(new Texture(handle));
}

Texture::Texture(uint32_t handle)
    : handle_(handle)
{
}

Texture::~Texture()
{
    glDeleteTextures(1, &handle_);
}

void Texture::upload(const cv::Mat &mat)
{
    glBindTexture(GL_TEXTURE_2D, handle_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mat.size[1], mat.size[0], 0,
                 GL_BGR, GL_UNSIGNED_BYTE, mat.data);
}

void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, handle_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
