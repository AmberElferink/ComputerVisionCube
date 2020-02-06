#include "IndexedMesh.h"

#include <cassert>
#include <cstring>

#include <glad/glad.h>

namespace {

constexpr float quad_vertices[] = {
    0.f, 0.f, // 0 top left
    1.0, 0.f, // 1 top right
    1.f, 1.f, // 2 bottom right
    0.f, 1.f, // 3 bottom left
};
constexpr uint16_t quad_indices[] = {0, 1, 2, 2, 3, 0};
} // namespace

std::unique_ptr<IndexedMesh> IndexedMesh::create(const CreateInfo& info) {
    uint32_t buffers[2];
    uint32_t vao;
    glCreateBuffers(2, buffers); // create buffer pointer on gpu
    glCreateVertexArrays(1, &vao);

    glBindVertexArray(vao);
    // binds buffers to the slot in the vao, and this makes no sense but is
    // needed somehow
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);

    uint32_t attr_index = 0;
    uintptr_t attr_offset = 0;
    for (uint32_t i = 0; i < info.AttributeCount; ++i) {
        auto size = info.Attributes[i].Count;
        switch (info.Attributes[i].Type) {
        case GL_FLOAT:
            size *= sizeof(float);
            break;
        default:
            // printf("unsupported type\n");
            assert(false);
            return nullptr;
        }
        glEnableVertexAttribArray(attr_index);
        // tell the gpu (and RenderDoc) you use data of a specific type for the
        // vertices at a specific position in the shader
        glVertexAttribPointer(attr_index, info.Attributes[i].Count,
                              info.Attributes[i].Type, GL_FALSE, size,
                              reinterpret_cast<const void*>(attr_offset));
        attr_index++;
        attr_offset += size;
    }

    glVertexArrayVertexBuffer(vao, 0, buffers[0], 0, attr_offset);

    glNamedBufferData(buffers[0], info.VertexCount * info.VertexSize, nullptr,
                      GL_STATIC_DRAW);
    glNamedBufferData(buffers[1], info.IndexCount * info.IndexSize, nullptr,
                      GL_STATIC_DRAW);

    if (info.DebugName.data()) {
        // to be able to read it in RenderDoc/errors
        glObjectLabel(
            GL_BUFFER, buffers[0], -1,
            (info.DebugName.data() + std::string(" vertex buffer")).c_str());
        // to be able to read it in RenderDoc/errors
        glObjectLabel(
            GL_BUFFER, buffers[1], -1,
            (info.DebugName.data() + std::string(" index buffer")).c_str());
        // to be able to read it in RenderDoc/errors
        glObjectLabel(
            GL_VERTEX_ARRAY, vao, -1,
            (info.DebugName.data() + std::string(" vertex array object"))
                .c_str());
    }

    return std::unique_ptr<IndexedMesh>(
        new IndexedMesh(buffers[0], buffers[1], vao, info.IndexCount));
}

std::unique_ptr<IndexedMesh>
IndexedMesh::createFullscreenQuad(const std::string_view& debug_name) {
    const std::vector<IndexedMesh::MeshAttributes> attributes{
        MeshAttributes{GL_FLOAT, 2}};
    CreateInfo info;
    info.Attributes = attributes.data();
    info.AttributeCount = attributes.size();
    info.VertexSize = sizeof(quad_vertices[0]);
    info.VertexCount = sizeof(quad_vertices) / sizeof(quad_vertices[0]);
    info.IndexSize = sizeof(quad_indices[0]);
    info.IndexCount = sizeof(quad_indices) / sizeof(quad_indices[0]);
    info.DebugName = debug_name;
    auto fullscreen_quad = IndexedMesh::create(info);

    std::memcpy(fullscreen_quad->mapVertexBuffer(MemoryMapAccess::Write).get(),
                quad_vertices, sizeof(quad_vertices));
    std::memcpy(fullscreen_quad->mapIndexBuffer(MemoryMapAccess::Write).get(),
                quad_indices, sizeof(quad_indices));

    return fullscreen_quad;
}

IndexedMesh::IndexedMesh(uint32_t vertex_buffer, uint32_t index_buffer,
                         uint32_t vao, uint16_t element_count)

    : vertexBuffer_(vertex_buffer), indexBuffer_(index_buffer), vao_(vao),
      element_count(element_count) {}

IndexedMesh::~IndexedMesh() {
    glDeleteBuffers(2, reinterpret_cast<uint32_t*>(this));
    glDeleteVertexArrays(1, &vao_);
}

void IndexedMesh::draw() const {
    bind();
    glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_SHORT, nullptr);
}

void IndexedMesh::bind() const { glBindVertexArray(vao_); }

std::unique_ptr<uint8_t, IndexedMesh::MemoryUnmapper>
IndexedMesh::mapVertexBuffer(IndexedMesh::MemoryMapAccess access) {
    if (access <= 0 || access > 3) {
        return nullptr;
    }
    void* mapped = glMapNamedBuffer(vertexBuffer_, GL_READ_ONLY - 1 + access);
    return std::unique_ptr<uint8_t, MemoryUnmapper>(
        reinterpret_cast<uint8_t*>(mapped), {vertexBuffer_});
}

std::unique_ptr<uint8_t, IndexedMesh::MemoryUnmapper>
IndexedMesh::mapIndexBuffer(IndexedMesh::MemoryMapAccess access) {
    if (access <= 0 || access > 3) {
        return nullptr;
    }
    void* mapped = glMapNamedBuffer(indexBuffer_, GL_READ_ONLY - 1 + access);
    return std::unique_ptr<uint8_t, MemoryUnmapper>(
        reinterpret_cast<uint8_t*>(mapped), {indexBuffer_});
}

void IndexedMesh::MemoryUnmapper::operator()(const uint8_t* mapped) {
    if (mapped) {
        glUnmapNamedBuffer(id_);
    }
}
