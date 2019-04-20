#pragma once

#include <vector>
#include "d3d12.h"
#include "wrl.h"

struct Mesh
{
    void createVertexBuffer(const void* const data, const size_t vertexCount, const size_t vertexSize, ID3D12GraphicsCommandList* const pCommandList, const size_t index = 0);
    void createIndexBuffer(const void* const data, const size_t indexCount, const size_t indexSize, ID3D12GraphicsCommandList* const pCommandList);

    D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(const size_t index = 0) const;
    D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const;

    static constexpr size_t MAX_VERTEX_BUFFERS = 4u;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer[MAX_VERTEX_BUFFERS];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBufferUpload[MAX_VERTEX_BUFFERS];

    Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBufferUpload;

    size_t m_vertexSize[MAX_VERTEX_BUFFERS];
    size_t m_vertexCount;
    size_t m_indexSize;
    size_t m_indexCount;
};
