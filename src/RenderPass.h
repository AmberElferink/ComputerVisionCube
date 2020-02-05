#pragma once

#include <memory>

class RenderPass {
  public:
    struct CreateInfo {
        float ClearColor[4];
        std::string_view DebugName;
    };

    virtual ~RenderPass();

    void bind();

    /// A factory function in the impl class allows for an error to return null
    static std::unique_ptr<RenderPass> create(const CreateInfo& info);

  private:
    explicit RenderPass(const float clearColor[4]);

    const float clearColor_[4];
};
