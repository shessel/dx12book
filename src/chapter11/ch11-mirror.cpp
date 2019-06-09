#include "Mirror.h"

#include "DebugUtil.h"

int WinMain(HINSTANCE hInst, HINSTANCE /*prev*/, LPSTR /*argv*/, int /*show*/)
{
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    try
    {
        Mirror demo(hInst);
        return demo.run();
    }
    catch (DxDebugException e)
    {
        OutputDebugStringDxDebugException(e);
    }
    return 0;
}