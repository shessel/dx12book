#include "GeometryUtil.h"

#include <cstring>
#include <cmath>
#include <unordered_map>

#include <algorithm>
#include <memory>

#include "DirectXMath.h"

namespace GeometryUtil
{
    struct Vertex {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 col;
    };

    void calculateVertexIndexCountsGeoSphere(uint8_t subdivisions, size_t& vertexCount, size_t& indexCount)
    {
        /*
        // Base icosahedron has 12 vertices, 20 triangles and 30 edges. Each subdivision replaces one triangle by
        // 4 triangles (think triforce) and splits the edges in two halfs, adding one vertex per edge and 3 edges
        // inside of each triangle.
        vertexCount = 12;
        size_t edgeCount = 30;
        size_t triangleCount = 20;
        for (uint8_t i = 0; i < subdivisions; ++i)
        {
            vertexCount += edgeCount;
            edgeCount = 2*edgeCount + 3*triangleCount;
            triangleCount *= 4;
        }
        // Each vertex is part of 5 triangles
        indexCount = 5*vertexCount;
        */
        // solved recurrence relation, vertex count is 2 + 10*4^(subdivisions)
        // 4^x = 1 << (x*2)
        vertexCount = 2 + 10 * (1 << (subdivisions * 2));
        indexCount = 3 * 20 * (1 << (subdivisions * 2));
    }

    void createGeoSphere(const float radius, const uint8_t subdivisions, void* const vertices, void* const indices)
    {
        const float phi = 1.6180339887f;
        const float distFromCenter = std::sqrtf(phi * phi + 1.0f);
        const float normalizationFactor = radius / distFromCenter;
        const float phiNorm = phi * normalizationFactor;
        const float oneNorm = normalizationFactor;
        const Vertex baseVertices[] = {
            {{0.0f,  phiNorm,  oneNorm}, DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)},
            {{0.0f,  phiNorm, -oneNorm}, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)},
            {{0.0f, -phiNorm,  oneNorm}, DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)},
            {{0.0f, -phiNorm, -oneNorm}, DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
            {{-oneNorm, 0.0f,  phiNorm}, DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)},
            {{ oneNorm, 0.0f,  phiNorm}, DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {{-oneNorm, 0.0f, -phiNorm}, DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)},
            {{ oneNorm, 0.0f, -phiNorm}, DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)},
            {{-phiNorm,  oneNorm, 0.0f}, DirectX::XMFLOAT3(0.2f, 0.2f, 0.2f)},
            {{ phiNorm,  oneNorm, 0.0f}, DirectX::XMFLOAT3(0.4f, 0.4f, 0.4f)},
            {{-phiNorm, -oneNorm, 0.0f}, DirectX::XMFLOAT3(0.6f, 0.6f, 0.6f)},
            {{ phiNorm, -oneNorm, 0.0f}, DirectX::XMFLOAT3(0.8f, 0.8f, 0.8f)},
        };

        constexpr uint16_t baseIndices[] = {
            0, 5, 4,
            0, 9, 5,
            1, 9, 0,
            1, 7, 9,
            1, 6, 7,
            1, 8, 6,
            1, 0, 8,
            0, 4, 8,
            5, 9, 11,
            7, 11, 9,
            4, 10, 8,
            6, 8, 10,
            2, 4, 5,
            2, 5, 11,
            2, 11, 3,
            3, 11, 7,
            3, 7, 6,
            3, 6, 10,
            2, 3, 10,
            2, 10, 4,
        };

        std::unique_ptr<Vertex[]> tmpVertices;
        std::unique_ptr<uint16_t[]> tmpIndices;

        if (subdivisions > 0u)
        {
            // tmp buffers need enough space for second to last iteration
            size_t tmpVertexBufferVertexCount;
            size_t tmpIndexBufferIndexCount;
            calculateVertexIndexCountsGeoSphere(subdivisions - 1, tmpVertexBufferVertexCount, tmpIndexBufferIndexCount);
            tmpVertices = std::make_unique<Vertex[]>(tmpVertexBufferVertexCount);
            tmpIndices = std::make_unique<uint16_t[]>(tmpIndexBufferIndexCount);
        }

        // last iteration should target final buffer so starting dst/src depends on number of subdivisions
        Vertex* dstVertices = subdivisions % 2 == 1 ? reinterpret_cast<Vertex*>(vertices) : tmpVertices.get();
        uint16_t* dstIndices = subdivisions % 2 == 1 ? reinterpret_cast<uint16_t*>(indices) : tmpIndices.get();
        Vertex* srcVertices = subdivisions % 2 == 1 ? tmpVertices.get() : reinterpret_cast<Vertex*>(vertices);
        uint16_t* srcIndices = subdivisions % 2 == 1 ? tmpIndices.get() : reinterpret_cast<uint16_t*>(indices);

        std::memcpy(srcVertices, baseVertices, sizeof(baseVertices));
        std::memcpy(srcIndices, baseIndices, sizeof(baseIndices));

        if (subdivisions > 0u)
        {
            size_t totalVertexCount;
            size_t totalIndexCount;
            calculateVertexIndexCountsGeoSphere(subdivisions, totalVertexCount, totalIndexCount);

            std::unordered_map<size_t, uint16_t> edgesToNewIndex;
            // each vertex is part of 5 edges, but each edge has 2 end vertices
            edgesToNewIndex.reserve((totalVertexCount * 5) / 2);

            size_t vertexCopyOffset = 0;
            size_t indexCopyOffset = 0;

            for (uint8_t iteration = 0; iteration < subdivisions; ++iteration)
            {
                size_t vertexCountLastIteration;
                size_t indexCountLastIteration;
                calculateVertexIndexCountsGeoSphere(iteration, vertexCountLastIteration, indexCountLastIteration);

                // copy new vertices from last iteration
                std::memcpy(dstVertices + vertexCopyOffset, srcVertices + vertexCopyOffset, (vertexCountLastIteration - vertexCopyOffset) * sizeof(Vertex));
                std::memcpy(dstIndices + indexCopyOffset, srcIndices + indexCopyOffset, (indexCountLastIteration - indexCopyOffset) * sizeof(uint16_t));

                uint16_t nextFreeVertexIndex = static_cast<uint16_t>(vertexCountLastIteration);
                vertexCopyOffset = vertexCountLastIteration;
                indexCopyOffset = indexCountLastIteration;

                size_t dstTriangleIndex = 0;
                for (size_t srcTriangleIndex = 0; srcTriangleIndex < indexCountLastIteration; srcTriangleIndex += 3)
                {
                    const uint16_t *const triangle = &srcIndices[srcTriangleIndex];
                    const Vertex triangleVertices[] = {
                        srcVertices[triangle[0]],
                        srcVertices[triangle[1]],
                        srcVertices[triangle[2]],
                    };

                    uint16_t newVertexIndices[3] = {};

                    for (size_t edgeIndex = 0; edgeIndex < 3u; ++edgeIndex)
                    {
                        uint16_t i0 = triangle[edgeIndex];
                        uint16_t i1 = triangle[(edgeIndex + 1) % 3];

                        if (i0 > i1)
                        {
                            std::swap(i0, i1);
                        }

                        const size_t key = i0 << 16 | i1;
                        uint16_t& newVertexIndex = edgesToNewIndex[key];

                        if (newVertexIndex == 0)
                        {
                            newVertexIndex = nextFreeVertexIndex++;
                            DirectX::XMVECTOR xmvPos0 = DirectX::XMLoadFloat3(&triangleVertices[edgeIndex].pos);
                            DirectX::XMVECTOR xmvPos1 = DirectX::XMLoadFloat3(&triangleVertices[(edgeIndex + 1) % 3].pos);
                            xmvPos0 = DirectX::XMVectorAdd(xmvPos0, xmvPos1);
                            xmvPos0 = DirectX::XMVector3Normalize(xmvPos0);
                            xmvPos0 = DirectX::XMVectorScale(xmvPos0, radius);
                            DirectX::XMStoreFloat3(&dstVertices[newVertexIndex].pos, xmvPos0);

                            dstVertices[newVertexIndex].col = {
                                srcTriangleIndex * 1.0f / indexCountLastIteration,
                                srcTriangleIndex * 1.0f / indexCountLastIteration,
                                srcTriangleIndex * 1.0f / indexCountLastIteration
                            };
                        }
                        newVertexIndices[edgeIndex] = newVertexIndex;
                    }

                    dstIndices[dstTriangleIndex++] = triangle[0];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[0];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[2];
                    
                    dstIndices[dstTriangleIndex++] = triangle[1];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[1];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[0];
                    
                    dstIndices[dstTriangleIndex++] = triangle[2];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[2];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[1];
                    
                    dstIndices[dstTriangleIndex++] = newVertexIndices[0];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[1];
                    dstIndices[dstTriangleIndex++] = newVertexIndices[2];
                }

                std::swap(dstVertices, srcVertices);
                std::swap(dstIndices, srcIndices);
            }
        }
    }

    void calculateVertexIndexCountsSquare(uint16_t vertexCountPerSide, size_t& vertexCount, size_t& indexCount)
    {
        vertexCount = vertexCountPerSide * vertexCountPerSide;
        // square count is (vertexCountPerSide-1)^2 which becomes vertexCount^2 - 2*vertexCountPerSide + 1^2.
        // triangle count is twice the square count, and index count is three times triangle count
        indexCount = 6 * (vertexCount - 2 * vertexCountPerSide + 1);
    }

    void createSquare(const float width, const uint16_t vertexCountPerSide, void* const vertices, void* const indices)
    {
        size_t vertexCount;
        size_t indexCount;
        calculateVertexIndexCountsSquare(vertexCountPerSide, vertexCount, indexCount);

        const float stepSize = width / (vertexCountPerSide - 1);
        const float start = -width / 2.0f;
        float x = start;
        float y = x;

        uint8_t* curVertexByte = reinterpret_cast<uint8_t*>(vertices);
        for (size_t xOffset = 0; xOffset < vertexCountPerSide; ++xOffset)
        {
            x = start;
            for (size_t yOffset = 0; yOffset < vertexCountPerSide; ++yOffset)
            {
                for (size_t i = 0; i < defaultVertexDesc.attributeCount; ++i)
                {
                    const auto& desc = defaultVertexDesc.pAttributeDescs[i];
                    switch (desc.attributeType)
                    {
                    case POSITION:
                    {
                        DirectX::XMFLOAT3* pos = reinterpret_cast<DirectX::XMFLOAT3*>(curVertexByte + desc.attributeByteOffset);
                        *pos = { x, 0.0f, y };
                    }
                    break;
                    case UV:
                    {
                        DirectX::XMFLOAT2 *uv = reinterpret_cast<DirectX::XMFLOAT2*>(curVertexByte + desc.attributeByteOffset);
                        *uv = {
                            static_cast<float>(xOffset) / (vertexCountPerSide - 1),
                            static_cast<float>(yOffset) / (vertexCountPerSide - 1)
                        };
                    }
                    break;
                    case NORMAL:
                    {
                        DirectX::XMFLOAT3* normal = reinterpret_cast<DirectX::XMFLOAT3*>(curVertexByte + desc.attributeByteOffset);
                        *normal = { 0.0f, 1.0f, 0.0f };
                    }
                    break;
                    }
                }
                curVertexByte += defaultVertexDesc.stride;
                x += stepSize;
            }
            y += stepSize;
        }

        uint16_t *curIndex = reinterpret_cast<uint16_t*>(indices);
        for (uint16_t xOffset = 0; xOffset < vertexCountPerSide - 1; ++xOffset)
        {
            for (uint16_t yOffset = 0; yOffset < vertexCountPerSide - 1; ++yOffset)
            {
                *(curIndex++) = yOffset * vertexCountPerSide + xOffset;
                *(curIndex++) = yOffset * vertexCountPerSide + xOffset + 1;
                *(curIndex++) = (yOffset + 1) * vertexCountPerSide + xOffset;

                *(curIndex++) = (yOffset + 1) * vertexCountPerSide + xOffset;
                *(curIndex++) = yOffset * vertexCountPerSide + xOffset + 1;
                *(curIndex++) = (yOffset + 1) * vertexCountPerSide + xOffset + 1;
            }
        }
    }
}