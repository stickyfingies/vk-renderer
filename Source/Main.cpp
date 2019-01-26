#include <GraphicsDevice.h>

#include <iostream>

int main()
{
	GraphicsDevice device{1024, 768};

	try
	{
		device.Run();
	}
	catch (const std::runtime_error & err)
	{
		std::cerr << err.what() << std::endl;

		int c;

		std::cin >> c;

		return 1;
	}

	return 0;
}