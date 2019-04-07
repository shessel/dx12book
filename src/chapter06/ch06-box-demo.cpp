#include "ch06-box-demo.h"
#include "DebugUtil.h"

int WinMain(HINSTANCE hinst, HINSTANCE /*hprev*/, LPSTR /*cmdline*/, int /*show*/)
{
    try
    {
        BoxDemo demo(hinst);
        return demo.run();
    }
    catch (DxDebugException e)
    {
        OutputDebugStringDxDebugException(e);
        return 1;
    }
}