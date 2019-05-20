#pragma once

#include <cinttypes>

namespace GeometryUtil
{
    enum class VertexAttributeType
    {
        POSITION,
        UV,
        NORMAL,
        UNINITIALIZED, // ignored during geometry generation, use to leave space for custom attribs
    };

    struct VertexAttributeDesc
    {
        VertexAttributeType attributeType;
        size_t attributeByteOffset;
    };

    struct VertexDesc
    {
        size_t attributeCount;
        const VertexAttributeDesc* pAttributeDescs;
        size_t stride;
    };

    constexpr VertexAttributeDesc defaultAttributeCollection[] =
    {
        {VertexAttributeType::POSITION, 0u},
        {VertexAttributeType::UV, 12u},
        {VertexAttributeType::NORMAL, 20u},
    };

    constexpr VertexDesc defaultVertexDesc = {
        sizeof(defaultAttributeCollection) / sizeof(VertexAttributeDesc), defaultAttributeCollection, 32u
    };

    void calculateVertexIndexCountsGeoSphere(uint8_t suddivisions, size_t& vertexCount, size_t& indexCount);
    void createGeoSphere(const float radius, const uint8_t subdivisions, void* const vertices, void* const indices, const VertexDesc& = defaultVertexDesc);
    void calculateVertexIndexCountsSquare(uint16_t vertexCountPerSide, size_t& vertexCount, size_t& indexCount);
    void createSquare(const float width, const uint16_t vertexCountPerSide, void* const vertices, void* const indices, const VertexDesc& = defaultVertexDesc);
}