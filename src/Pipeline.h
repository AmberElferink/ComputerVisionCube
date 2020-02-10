#pragma once

#include <cstdint>

#include <memory>
#include <string_view>

// mat4 is equivalent to float[16]
typedef float mat4[16];

class Pipeline {
  public:
    struct CreateInfo {
        uint32_t ViewportWidth;
        uint32_t ViewportHeight;
        std::string_view VertexShaderSource;
        std::string_view FragmentShaderSource;
        std::string_view DebugName;
    };

    virtual ~Pipeline();

    void bind();
    template <typename T>
    bool setUniform(const std::string_view& uniform_name, const T& uniform);

    /// A factory function in the impl class allows for an error to return null
    static std::unique_ptr<Pipeline> create(const CreateInfo& info);

  private:
    explicit Pipeline(uint32_t program, uint32_t viewportWidth,
                      uint32_t viewportHeight);

    const uint32_t program_;
    uint32_t viewportWidth_;
    uint32_t viewportHeight_;
};
