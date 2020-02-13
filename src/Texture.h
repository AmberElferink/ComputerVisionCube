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
    static std::unique_ptr<Texture> create();

    void upload(const cv::Mat& mat);
    void bind();

private:
    explicit Texture(uint32_t handle);
    virtual ~Texture();

    uint32_t handle_;
};
