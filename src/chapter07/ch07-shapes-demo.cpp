#include "ShapesDemo.h"

#include "DebugUtil.h"

int WinMain(HINSTANCE hInst, HINSTANCE /*hprev*/, LPSTR /*cmdline*/, int /*show*/)
{
#ifndef NDEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    try
    {
        ShapesDemo demo(hInst);
        return demo.run();
    }
    catch (DxDebugException e)
    {
        OutputDebugStringDxDebugException(e);
        return 1;
    }
}