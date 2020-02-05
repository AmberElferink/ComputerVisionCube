#include "RenderPass.h"

#include <glad/glad.h>

std::unique_ptr<RenderPass>
RenderPass::create(const RenderPass::CreateInfo& info) {
    return std::unique_ptr<RenderPass>(new RenderPass(info.ClearColor));
}

RenderPass::RenderPass(const float clearColor[4])
    : clearColor_{clearColor[0], clearColor[1], clearColor[2], clearColor[3]} {}

RenderPass::~RenderPass() = default;

void RenderPass::bind() {
    glClearColor(1.f, 1.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
}
