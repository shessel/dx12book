#include "ShapesDemo.h"

#include "DebugUtil.h"

void ShapesDemo::initialize()
{
    for (size_t i = 0; i < m_frameResourcesCount; ++i)
    {
        m_frameResources.emplace_back(m_pDevice.Get(), 1, 1);
    }
}

void ShapesDemo::update(float /*dt*/)
{
    m_curFrameResourcesIndex = (m_curFrameResourcesIndex + 1u) % m_frameResourcesCount;
    FrameResources& curFrameResource = m_frameResources[m_curFrameResourcesIndex];
    if (curFrameResource.m_fenceValue != 0 && curFrameResource.m_fenceValue > m_pFence->GetCompletedValue())
    {
        HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_pFence->SetEventOnCompletion(curFrameResource.m_fenceValue, hEvent));
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }
}
void ShapesDemo::render()
{}
void ShapesDemo::onMouseDown(int16_t /*xPos*/, int16_t /*yPos*/, uint8_t /*buttons*/)
{}
void ShapesDemo::onMouseUp(int16_t /*xPos*/, int16_t /*yPos*/, uint8_t /*buttons*/)
{}
void ShapesDemo::onMouseMove(int16_t /*xPos*/, int16_t /*yPos*/, uint8_t /*buttons*/)
{}