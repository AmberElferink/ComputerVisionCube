#include "RenderPass.h"

#include <glad/glad.h>

std::unique_ptr<RenderPass>
RenderPass::create(const RenderPass::CreateInfo& info) {
    return std::unique_ptr<RenderPass>(new RenderPass(info.Clear, info.ClearColor, info.WriteDepth));
}

RenderPass::RenderPass(bool clear, const float clearColor[4], bool writeDepth)
    : clear_(clear)
    , clearColor_{clearColor[0], clearColor[1], clearColor[2], clearColor[3]}
    , writeDepth_(writeDepth)
{
}

RenderPass::~RenderPass() = default;

void RenderPass::bind() {
    if(clear_) //only clear if you want to draw over the screen (so for the cube) the quad wants to discard previous info.
    {
        glClearColor(clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]);
        glClearDepthf(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    glDepthMask(writeDepth_);   //disable writing to the depth buffer for the screen quad, and enable it for the renderpass with the cube
                                //can also do glColorMask to only write to specific channel. Now were just enabling/disabling depth
    glEnable(GL_DEPTH_TEST);
}
