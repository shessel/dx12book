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

std::vector<IDXGIOutput*> getOutputsForAdapter(IDXGIAdapter1* const pAdapter)
{
    UINT i = 0;
    IDXGIOutput* pOutput = nullptr;
    std::vector<IDXGIOutput*> outputs;
    while (SUCCEEDED(pAdapter->EnumOutputs(i++, &pOutput)))
    {
        outputs.push_back(pOutput);
    }
    return outputs;
}

void printOutputInfo(const std::vector<IDXGIOutput*>& outputs)
{
    for (IDXGIOutput* pOutput : outputs)
    {
        DXGI_OUTPUT_DESC desc;
        pOutput->GetDesc(&desc);
        std::wcout << L"\t" << desc.DeviceName << L"\n";

        UINT displayModeCount = 0;
        pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &displayModeCount, nullptr);
        std::vector<DXGI_MODE_DESC> displayModes(displayModeCount);
        pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &displayModeCount, displayModes.data());

        for (const DXGI_MODE_DESC& modeDesc : displayModes)
        {
            std::wcout << L"\t\t" << modeDesc.Width << L"x" << modeDesc.Height << L"@";
            std::wcout << static_cast<float>(modeDesc.RefreshRate.Numerator) / modeDesc.RefreshRate.Denominator << L"Hz" << L"\n";
        }
    }
}

void printAdapterInfo(const std::vector<IDXGIAdapter1*>& adapters)
{
    bool first = true;
    for (IDXGIAdapter1* pAdapter : adapters)
    {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter->GetDesc1(&desc);
        std::wcout << (first ? L"" : L"\n") << desc.Description << L"\n";

        auto outputs = getOutputsForAdapter(pAdapter);
        printOutputInfo(outputs);
        first = false;
    }
}

int main()
{
    auto adapters = getAdapters();
    printAdapterInfo(adapters);

    return 0;
}