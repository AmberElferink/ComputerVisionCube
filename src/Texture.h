#pragma once

#include <cstdint>
#include <memory>

namespace cv
{
class Mat;
}

class Texture
{
public:
    static std::unique_ptr<Texture> create(uint32_t width, uint32_t height);
    virtual ~Texture();

    void upload(const cv::Mat& mat);
    void bind();

    inline float getAspect() const { return static_cast<float>(width_) / height_; };
    inline uint32_t getNativeHandle() const { return handle_; };

private:
    Texture(uint32_t handle, uint32_t width, uint32_t height);

    const uint32_t handle_;
    const uint32_t width_;
    const uint32_t height_;
};
