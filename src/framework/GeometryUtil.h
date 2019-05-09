#pragma once

#include <cinttypes>

namespace GeometryUtil
{
    enum VertexAttributeType
    {
        POSITION,
        UV,
        NORMAL,
        TANGENT_U,
        TANGENT_V,
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

    constexpr VertexAttributeDesc defaultAlltributeCollection[] =
    {
        {POSITION, 0u},
        {NORMAL, 20u},
        {UV, 12u},
    };

    constexpr VertexDesc defaultVertexDesc = {
        sizeof(defaultAlltributeCollection) / sizeof(VertexAttributeDesc), defaultAlltributeCollection, 32u
    };

    void calculateVertexIndexCountsGeoSphere(uint8_t suddivisions, size_t& vertexCount, size_t& indexCount);
    void createGeoSphere(const float radius, const uint8_t subdivisions, void* const vertices, void* const indices);
    void calculateVertexIndexCountsSquare(uint16_t vertexCountPerSide, size_t& vertexCount, size_t& indexCount);
    void createSquare(const float width, const uint16_t vertexCountPerSide, void* const vertices, void* const indices);
}