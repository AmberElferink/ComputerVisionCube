#pragma once

#include <memory>
#include <string_view>

/// Construct which stores state changes and attributes of framebuffer usage
/// Use of render passes allows programming graphics pipelines which don't
/// arbitrarily change state causing reconfiguration of the GPU pipeline
/// and slow downs in the driver.
class RenderPass {
  public:
    struct CreateInfo {
        bool Clear;
        float ClearColor[4];
        bool DepthWrite;
        bool DepthTest;;
        std::string_view DebugName;
    };

    virtual ~RenderPass();

    /// Bind all of the states contained in the pass
    void bind();

    /// A factory function in the impl class allows for an error to return null
    static std::unique_ptr<RenderPass> create(const CreateInfo& info);

  private:
    /// Private unique constructor forcing the use of factory function which
    /// can return null unlike constructor.
    explicit RenderPass(const CreateInfo& info);

    const CreateInfo info_;

};
