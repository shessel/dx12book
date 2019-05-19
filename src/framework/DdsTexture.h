#pragma once

#include <cinttypes>
#include <memory>
#include <vector>

#include "d3d12.h"
#include "wrl.h"

struct DdsTexture
{
    std::unique_ptr<uint8_t[]> m_pDdsData;
    std::vector<D3D12_SUBRESOURCE_DATA> m_subresources;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadResource;

    void createFromFileAndUpload(ID3D12GraphicsCommandList* const commandList, const wchar_t* const filename);
};