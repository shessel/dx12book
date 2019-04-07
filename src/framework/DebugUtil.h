#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

struct DxDebugException
{
    DxDebugException(const HRESULT hr, const wchar_t *const expression, const wchar_t *const filename, const int line);
    
    const HRESULT m_hr;
    static constexpr size_t m_hrMsgLength = 256;
    wchar_t m_hrMsg[m_hrMsgLength];
    const wchar_t *const m_expression;
    const wchar_t *const m_filename;
    const int m_line;
};

void OutputDebugStringDxDebugException(const DxDebugException& exception);

#define ThrowIfFailed(expression)\
{\
    HRESULT __debug_util_hr_for_expression = expression;\
    if (FAILED(__debug_util_hr_for_expression))\
    {\
        const wchar_t *const filename = L"" __FILE__;\
        throw DxDebugException(__debug_util_hr_for_expression, L#expression, filename, __LINE__);\
    }\
}