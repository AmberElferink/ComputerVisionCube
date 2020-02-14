#include "RenderPass.h"

#include <glad/glad.h>

std::unique_ptr<RenderPass>
RenderPass::create(const RenderPass::CreateInfo& info) {
    return std::unique_ptr<RenderPass>(new RenderPass(info));
}

RenderPass::RenderPass(const CreateInfo& info)
    : info_(info)
{
}

RenderPass::~RenderPass() = default;

void RenderPass::bind() {
    if (info_.Clear) //only clear if you want to draw over the screen (so for the cube) the quad wants to discard previous info.
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(true);
        glClearColor(info_.ClearColor[0], info_.ClearColor[1], info_.ClearColor[2], info_.ClearColor[3]);
        glClearDepthf(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    glDepthMask(info_.DepthWrite);   //disable writing to the depth buffer for the screen quad, and enable it for the renderpass with the cube
                                     //can also do glColorMask to only write to specific channel. Now were just enabling/disabling depth

    if (info_.DepthTest)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}
