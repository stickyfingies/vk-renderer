#include <VulkanApp.h>

#include <iostream>

int main()
{
	VulkanApp app{1024, 768};

	try
	{
		app.Run();
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
