
#include "Framework.h"

int main(int argc, char* argv[])
{
    RenderFrame demo(1280, 720, "Demo");
    demo.onLoad();
    demo.run();
    return 0;
}