#include "Texture.h"

#include <cassert>
#include <glad/glad.h>
#include <opencv2/core/mat.hpp>

std::unique_ptr<Texture> Texture::create(uint32_t width, uint32_t height) {
    uint32_t handle = 0;
    glGenTextures(1, &handle);
    if (handle == 0)
    {
        return nullptr;
    }
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return std::unique_ptr<Texture>(new Texture(handle, width, height));
}

Texture::Texture(uint32_t handle, uint32_t width, uint32_t height)
    : handle_(handle)
    , width_(width)
    , height_(height)
{
}

Texture::~Texture()
{
    glDeleteTextures(1, &handle_);
}

void Texture::upload(const cv::Mat &mat)
{
    assert(width_ == mat.cols);
    assert(height_ == mat.rows);
    glBindTexture(GL_TEXTURE_2D, handle_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mat.cols, mat.rows, 0,
                 GL_BGR, GL_UNSIGNED_BYTE, mat.data);
}

void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, handle_);
}
