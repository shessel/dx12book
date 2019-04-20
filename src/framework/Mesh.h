#pragma once

#include <vector>
#include "d3d12.h"
#include "wrl.h"

struct Mesh
{
    void createVertexBuffer(const void* const data, const size_t vertexCount, const size_t vertexSize, ID3D12GraphicsCommandList* const pCommandList);
    void createIndexBuffer(const void* const data, const size_t indexCount, const size_t indexSize, ID3D12GraphicsCommandList* const pCommandList);

    D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const;
    D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBufferUpload;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBufferUpload;

    size_t m_vertexSize;
    size_t m_vertexCount;
    size_t m_indexSize;
    size_t m_indexCount;
};
