#pragma once

#include <cstdint>

#include <memory>
#include <vector>
#include <string_view>

/// Wrapper for OpenGL Vertex Array Buffers
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

    /// General factory function only accepts uint16_t indices
    static std::unique_ptr<IndexedMesh> create(const CreateInfo& info);
    /// Factory function for generating a full screen quad with positions
    /// encoded in the screen space coordinates
    static std::unique_ptr<IndexedMesh>
    createFullscreenQuad(const std::string_view& debug_name);
    /// Factory function for creating a tri-color unit axis centered at 0
    /// with each arm extending at 1 in every axis.
    static std::unique_ptr<IndexedMesh>
    createAxis(const std::string_view& debug_name);
    /// Factory function for creating a 1 unit cube with one vertex at the origin,
    /// every vertex with positive values in each axis and side length of 1.
    static std::unique_ptr<IndexedMesh>
    createCube(const std::string_view& debug_name);

    virtual ~IndexedMesh();
    /// Draw the indexed mesh using opengl
    void draw() const;
    /// Bind the buffers for drawing
    void bind() const;

    /// Map vertex staging memory for copy before upload to driver and then GPU.
    /// Once pointer goes out of scope, the memory is offloaded to the driver.
    std::unique_ptr<uint8_t, MemoryUnmapper>
    mapVertexBuffer(MemoryMapAccess access);
    /// Map index staging memory for copy before upload to driver and then GPU.
    /// Once pointer goes out of scope, the memory is offloaded to the driver.
    std::unique_ptr<uint8_t, MemoryUnmapper>
    mapIndexBuffer(MemoryMapAccess access);

  private:
    /// Private unique constructor forcing the use of factory function which
    /// can return null unlike constructor.
    IndexedMesh(uint32_t vertex_buffer, uint32_t index_buffer, uint32_t vao,
                uint16_t element_count, Topology topology);
};
