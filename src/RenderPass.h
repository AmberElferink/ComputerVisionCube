#pragma once

#include <memory>
#include <string_view>

class RenderPass {
  public:
    struct CreateInfo {
        bool Clear;
        float ClearColor[4];
        bool WriteDepth;
        std::string_view DebugName;
    };

    virtual ~RenderPass();

    void bind();

    /// A factory function in the impl class allows for an error to return null
    static std::unique_ptr<RenderPass> create(const CreateInfo& info);

  private:
    RenderPass(bool clear, const float clearColor[4], bool writeDepth);

    const bool clear_;
    const float clearColor_[4];
    const bool writeDepth_;

};
