#include "Mesh.h"
#include "DebugUtil.h"
#include "D3D12Util.h"

void Mesh::createVertexBuffer(const void* const data, const size_t vertexCount, const size_t vertexSize, ID3D12GraphicsCommandList* const pCommandList)
{
    m_vertexCount = vertexCount;
    m_vertexSize = vertexSize;

    D3D12Util::createAndUploadBuffer(data, vertexCount * vertexSize, pCommandList, &m_pVertexBuffer, &m_pVertexBufferUpload);
    
    const D3D12_RESOURCE_BARRIER barrier = D3D12Util::TransitionBarrier(m_pVertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    pCommandList->ResourceBarrier(1, &barrier);
}

void Mesh::createIndexBuffer(const void* const data, const size_t indexCount, const size_t indexSize, ID3D12GraphicsCommandList* const pCommandList)
{
    m_indexCount = indexCount;
    m_indexSize = indexSize;

    D3D12Util::createAndUploadBuffer(data, indexCount * indexSize, pCommandList, &m_pIndexBuffer, &m_pIndexBufferUpload);

    const D3D12_RESOURCE_BARRIER barrier = D3D12Util::TransitionBarrier(m_pIndexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    pCommandList->ResourceBarrier(1, &barrier);
}

D3D12_INDEX_BUFFER_VIEW Mesh::getIndexBufferView() const
{
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    indexBufferView.SizeInBytes = static_cast<UINT>(m_indexCount * m_indexSize);
    return indexBufferView;
}

D3D12_VERTEX_BUFFER_VIEW Mesh::getVertexBufferView() const
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = static_cast<UINT>(m_vertexCount * m_vertexSize);
    vertexBufferView.StrideInBytes = static_cast<UINT>(m_vertexSize);
    return vertexBufferView;
}
