#pragma once

#include <cstdint>
#include <memory>

namespace cv
{
class Mat;
}

/// Wrapper around OpenGL textures, OpenCV-OpenGL interop and samplers
/// Allows upload of data but not resizing or modifying texture attributes
/// once they are created. These are expensive operations which are not
/// desired.
class Texture
{
public:
    /// Factory function for creating a texture with a specific dimension
    /// only one color format is used which is why it is not an argument.
    static std::unique_ptr<Texture> create(uint32_t width, uint32_t height);
    virtual ~Texture();

    /// Upload CPU copy of OpenCV memory into GPU buffer of OpenGL texture
    void upload(const cv::Mat& mat);
    /// Bind a texture for sampling in a pipeline
    void bind();

    /// Get aspect ratio of texture. Useful for projection matrix
    inline float getAspect() const { return static_cast<float>(width_) / height_; };
    /// Get native handle of texture. Useful for UI preview or drawing in other renderers.
    inline uint32_t getNativeHandle() const { return handle_; };

private:
    /// Private unique constructor forcing the use of factory function which
    /// can return null unlike constructor.
    Texture(uint32_t handle, uint32_t width, uint32_t height);

    const uint32_t handle_;
    const uint32_t width_;
    const uint32_t height_;
};
