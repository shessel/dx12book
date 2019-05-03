#pragma once

#include <cinttypes>

namespace GeometryUtil
{
    void calculateVertexIndexCountsGeoSphere(uint8_t suddivisions, size_t& vertexCount, size_t& indexCount);
    void createGeoSphere(const float radius, const uint8_t subdivisions, void* const vertices, void* const indices);
    void calculateVertexIndexCountsSquare(uint16_t vertexCountPerSide, size_t& vertexCount, size_t& indexCount);
    void createSquare(const float width, const uint16_t vertexCountPerSide, void* const vertices, void* const indices);
}