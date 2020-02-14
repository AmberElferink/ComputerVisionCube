#pragma once

#include <memory>
#include <string_view>

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

    void bind();

    /// A factory function in the impl class allows for an error to return null
    static std::unique_ptr<RenderPass> create(const CreateInfo& info);

  private:
    explicit RenderPass(const CreateInfo& info);

    const CreateInfo info_;

};
