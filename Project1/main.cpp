
#include "Framework.h"

int main(int argc, char* argv[])
{
	RenderFrame demo(1600, 1024, "Demo");
	demo.onLoad();
	demo.run();
	return 0;
}