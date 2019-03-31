#include "AppBase.h"

int WinMain(HINSTANCE hinst, HINSTANCE /*hprev*/, LPSTR /*cmdline*/, int /*show*/)
{
    AppBase app(hinst);
    return app.run();
}