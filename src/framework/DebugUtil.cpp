#include "DebugUtil.h"

#include <cwchar>
#include "comdef.h"

DxDebugException::DxDebugException(const HRESULT hr, const wchar_t *const expression, const wchar_t *const filename, const int line)
    : m_hr(hr), m_expression(expression), m_filename(filename), m_line(line)
{
    _com_error err(hr);
    wcscpy_s(m_hrMsg, err.ErrorMessage());
};

void OutputDebugStringDxDebugException(const DxDebugException& exception)
{
    OutputDebugStringW(L"DX12 Error: ");
    OutputDebugStringW(exception.m_hrMsg);
    OutputDebugStringW(L"\n  Expression: ");
    OutputDebugStringW(exception.m_expression);
    OutputDebugStringW(L"\n  In ");
    OutputDebugStringW(exception.m_filename);
    OutputDebugStringW(L" Line ");
    wchar_t lineBuffer[16];
    _itow_s(exception.m_line, lineBuffer, 10);
    OutputDebugStringW(lineBuffer);
    OutputDebugStringW(L"\n");
}