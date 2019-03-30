#include <iostream>
#include <vector>

#include "dxgi.h"

std::vector<IDXGIAdapter1*> getAdapters()
{
    IDXGIFactory1* pFactory = nullptr;
    CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory));

    UINT i = 0;
    IDXGIAdapter1* pAdapter = nullptr;
    std::vector<IDXGIAdapter1*> adapters;
    while (SUCCEEDED(pFactory->EnumAdapters1(i++, &pAdapter)))
    {
        adapters.push_back(pAdapter);
    }

    return adapters;
}

void printAdapterInfo(const std::vector<IDXGIAdapter1*>& adapters)
{
    for (IDXGIAdapter1* pAdapter : adapters)
    {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter->GetDesc1(&desc);
        std::wcout << desc.Description << L"\n";
    }
}

int main()
{
    auto adapters = getAdapters();
    printAdapterInfo(adapters);

    return 0;
}