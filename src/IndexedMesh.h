#pragma once

#include <cstdint>

#include <memory>
#include <vector>
#include <string_view>

class IndexedMesh {
  public:
    enum Topology
    {
       Points = 0x0000,
       Lines = 0x0001,
       Line_loop = 0x0002,
       Line_strip = 0x0003,
       Triangles = 0x0004,
       Triangle_strip = 0x0005,
       Triangle_fan = 0x0006,
       Quads = 0x0007,
    };
    struct MeshAttributes {
        uint32_t Type;
        uint32_t Count;
    };
    struct CreateInfo {
        const MeshAttributes* Attributes;
        uint32_t AttributeCount;
        uint32_t VertexBufferSize;
        uint32_t IndexBufferSize;
        Topology Topology;
        std::string_view DebugName;
    };
    enum MemoryMapAccess {
        Read = 0x0001,
        Write = 0x0002,
    };
    struct MemoryUnmapper {
        void operator()(const uint8_t* mapped);
        uint32_t id_;
    };
    const uint32_t vertexBuffer_;
    const uint32_t indexBuffer_;
    const uint32_t vao_;
    const uint16_t element_count;
    const Topology topology;

    /// Only accepts uint16_t indices
    static std::unique_ptr<IndexedMesh> create(const CreateInfo& info);
    static std::unique_ptr<IndexedMesh>
    createFullscreenQuad(const std::string_view& debug_name);

    virtual ~IndexedMesh();
    void draw() const;
    void bind() const;

    std::unique_ptr<uint8_t, MemoryUnmapper>
    mapVertexBuffer(MemoryMapAccess access);
    std::unique_ptr<uint8_t, MemoryUnmapper>
    mapIndexBuffer(MemoryMapAccess access);

  private:
    IndexedMesh(uint32_t vertex_buffer, uint32_t index_buffer, uint32_t vao,
                uint16_t element_count, Topology topology);
};
