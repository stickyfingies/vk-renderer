cmake_minimum_required (VERSION 3.12)

project (VulkanToy LANGUAGES CXX)

add_executable (VulkanToy)

target_sources (VulkanToy
PRIVATE
	Source/Main.cpp
	Source/GraphicsDevice.cpp
)

target_include_directories (VulkanToy
PUBLIC
	Include
	"C:/VulkanSDK/1.1.92.1/Include"
)

target_link_libraries (VulkanToy
PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Bin/glfw3dll.lib
	"C:/VulkanSDK/1.1.92.1/Lib/vulkan-1.lib"
)

set_target_properties  (VulkanToy PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin CXX_STANDARD 17 CXX_EXTENSIONS OFF)