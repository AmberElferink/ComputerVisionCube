#include "IndexedMesh.h"

#include <cassert>
#include <cstring>

#include <glad/glad.h>

namespace
{

    constexpr float quad_vertices[] = {
            // ,---------- u
            // |     ,---- v
            // |     |
            0.0f, 0.0f, // 0 top left
            1.00, 0.0f, // 1 top right
            1.0f, 1.0f, // 2 bottom right
            0.0f, 1.0f, // 3 bottom left
    };
    constexpr uint16_t quad_indices[] = {0, 1, 2, 2, 3, 0};

    constexpr float axis_vertices[] = {
            // ,---------------------------------- x
            // |     ,---------------------------- y
            // |     |     ,---------------------- z
            // |     |     |
            // |     |     |     ,---------------- R
            // |     |     |     |     ,---------- G
            // |     |     |     |     |     ,---- B
            // |     |     |     |     |     |
            0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // 0 center
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // 1 x axis
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 2 center
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 3 y axis
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 4 center
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // 5 z axis
    };
    constexpr uint16_t axis_indices[] = {0, 1, 2, 3, 4, 5};

//cube vertices from: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-4-a-colored-cube/
// Our vertices. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
    constexpr float cube_vertices[] = {
            // Left
            0.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
            // Right
            1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f,

            // Top
            1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
            // Bottom
            1.0f, 0.0f, 1.0f,  0.0f, -1.0f, 0.0f,
            0.0f, 0.0f, 0.0f,  0.0f, -1.0f, 0.0f,
            1.0f, 0.0f, 0.0f,  0.0f, -1.0f, 0.0f,
            1.0f, 0.0f, 1.0f,  0.0f, -1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,  0.0f, -1.0f, 0.0f,
            0.0f, 0.0f, 0.0f,  0.0f, -1.0f, 0.0f,

            // Front
            0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
            // Back
            1.0f, 1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
            0.0f, 1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
            1.0f, 1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
            1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
    };

    constexpr uint16_t cube_indices[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                         22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
} // namespace

std::unique_ptr<IndexedMesh> IndexedMesh::create(const CreateInfo &info)
{
    uint32_t buffers[2];
    uint32_t vao;
    glCreateBuffers(2, buffers); // create buffer pointer on gpu
    glCreateVertexArrays(1, &vao);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    // binds buffers to the slot in the vao, and this makes no sense but is
    // needed somehow
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);

    uint32_t totalStride = 0;
    for (uint32_t i = 0; i < info.AttributeCount; ++i)
    {
        auto size = info.Attributes[i].Count;
        switch (info.Attributes[i].Type)
        {
            case GL_FLOAT:
                size *= sizeof(float);
                break;
            default:
                // printf("unsupported type\n");
                assert(false);
                return nullptr;
        }
        totalStride += size;
    }

    glVertexArrayVertexBuffer(vao, 0, buffers[0], 0, info.VertexBufferSize);
    glNamedBufferData(buffers[0], info.VertexBufferSize, nullptr,
            GL_STATIC_DRAW);
    glNamedBufferData(buffers[1], info.IndexBufferSize, nullptr,
            GL_STATIC_DRAW);

    uintptr_t attr_offset = 0;
    for (uint32_t i = 0; i < info.AttributeCount; ++i)
    {
        auto size = info.Attributes[i].Count;
        switch (info.Attributes[i].Type)
        {
            case GL_FLOAT:
                size *= sizeof(float);
                break;
            default:
                // printf("unsupported type\n");
                assert(false);
                return nullptr;
        }
        // tell the gpu (and RenderDoc) you use data of a specific type for the
        // vertices at a specific position in the shader
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, info.Attributes[i].Count,
                info.Attributes[i].Type, GL_FALSE, totalStride,
                reinterpret_cast<const void *>(attr_offset));
        attr_offset += size;
    }

    if (info.DebugName.data())
    {
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
            new IndexedMesh(buffers[0], buffers[1], vao, info.IndexBufferSize / sizeof(uint16_t), info.Topology));
}

std::unique_ptr<IndexedMesh>
IndexedMesh::createFullscreenQuad(const std::string_view &debug_name)
{
    const std::vector<IndexedMesh::MeshAttributes> attributes{
            MeshAttributes{GL_FLOAT, 2}};
    CreateInfo info;
    info.Attributes = attributes.data();
    info.AttributeCount = attributes.size();
    info.VertexBufferSize = sizeof(quad_vertices);
    info.IndexBufferSize = sizeof(quad_indices);
    info.Topology = Topology::Triangles;
    info.DebugName = debug_name;
    auto fullscreen_quad = IndexedMesh::create(info);

    std::memcpy(fullscreen_quad->mapVertexBuffer(MemoryMapAccess::Write).get(),
            quad_vertices, sizeof(quad_vertices));
    std::memcpy(fullscreen_quad->mapIndexBuffer(MemoryMapAccess::Write).get(),
            quad_indices, sizeof(quad_indices));

    return fullscreen_quad;
}

std::unique_ptr<IndexedMesh> IndexedMesh::createAxis(const std::string_view &debug_name)
{
    const std::vector<IndexedMesh::MeshAttributes> attributes{
            MeshAttributes{GL_FLOAT, 3}, // position
            MeshAttributes{GL_FLOAT, 3}, // color
    };
    CreateInfo info;
    info.Attributes = attributes.data();
    info.AttributeCount = attributes.size();
    info.VertexBufferSize = sizeof(axis_vertices);
    info.IndexBufferSize = sizeof(axis_indices);
    info.Topology = Topology::Lines;
    info.DebugName = debug_name;
    auto axis = IndexedMesh::create(info);

    std::memcpy(axis->mapVertexBuffer(MemoryMapAccess::Write).get(),
            axis_vertices, sizeof(axis_vertices));
    std::memcpy(axis->mapIndexBuffer(MemoryMapAccess::Write).get(),
            axis_indices, sizeof(axis_indices));

    return axis;
}

std::unique_ptr<IndexedMesh> IndexedMesh::createCube(const std::string_view &debug_name)
{
    const std::vector<IndexedMesh::MeshAttributes> attributes{
            MeshAttributes{GL_FLOAT, 3}, // position
            MeshAttributes{GL_FLOAT, 3}, //normals
    };
    CreateInfo info;
    info.Attributes = attributes.data();
    info.AttributeCount = attributes.size();
    info.VertexBufferSize = sizeof(cube_vertices);
    info.IndexBufferSize = sizeof(cube_indices);
    info.Topology = Topology::Triangles;
    info.DebugName = debug_name;
    auto cube = IndexedMesh::create(info);

    std::memcpy(cube->mapVertexBuffer(MemoryMapAccess::Write).get(),
            cube_vertices, sizeof(cube_vertices));
    std::memcpy(cube->mapIndexBuffer(MemoryMapAccess::Write).get(),
            cube_indices, sizeof(cube_indices));

    return cube;
}

IndexedMesh::IndexedMesh(uint32_t vertex_buffer, uint32_t index_buffer,
                         uint32_t vao, uint16_t element_count, Topology topology)

        : vertexBuffer_(vertex_buffer), indexBuffer_(index_buffer), vao_(vao), element_count(element_count),
          topology(topology)
{
}

IndexedMesh::~IndexedMesh()
{
    glDeleteBuffers(2, reinterpret_cast<uint32_t *>(this));
    glDeleteVertexArrays(1, &vao_);
}

void IndexedMesh::draw() const
{
    bind();
    glDrawElements(topology, element_count, GL_UNSIGNED_SHORT, nullptr);
}

void IndexedMesh::bind() const
{
    glBindVertexArray(vao_);
}

std::unique_ptr<uint8_t, IndexedMesh::MemoryUnmapper>
IndexedMesh::mapVertexBuffer(IndexedMesh::MemoryMapAccess access)
{
    if (access <= 0 || access > 3)
    {
        return nullptr;
    }
    void *mapped = glMapNamedBuffer(vertexBuffer_, GL_READ_ONLY - 1 + access);
    return std::unique_ptr<uint8_t, MemoryUnmapper>(
            reinterpret_cast<uint8_t *>(mapped), {vertexBuffer_});
}

std::unique_ptr<uint8_t, IndexedMesh::MemoryUnmapper>
IndexedMesh::mapIndexBuffer(IndexedMesh::MemoryMapAccess access)
{
    if (access <= 0 || access > 3)
    {
        return nullptr;
    }
    void *mapped = glMapNamedBuffer(indexBuffer_, GL_READ_ONLY - 1 + access);
    return std::unique_ptr<uint8_t, MemoryUnmapper>(
            reinterpret_cast<uint8_t *>(mapped), {indexBuffer_});
}


void IndexedMesh::MemoryUnmapper::operator()(const uint8_t *mapped)
{
    if (mapped)
    {
        glUnmapNamedBuffer(id_);
    }
}
