cmake_minimum_required (VERSION 3.12)

project (VulkanToy LANGUAGES CXX)

add_executable (VulkanToy)

target_sources (VulkanToy
PRIVATE
	Source/Main.cpp
	Source/GraphicsDevice.cpp
)

set(ENV{VULKAN_SDK} "/VulkanSDK")

find_package(Vulkan REQUIRED)

target_include_directories (VulkanToy
PUBLIC
	Include
	${Vulkan_INCLUDE_DIRS}
)

set (UnixLibs
	dl
	pthread
	X11
	${CMAKE_CURRENT_SOURCE_DIR}/Bin/libglfw3.a
)

set (Win32Libs
	${CMAKE_CURRENT_SOURCE_DIR}/Bin/libglfw3dll.lib
)

if (WIN32)
	set (Libs ${Win32Libs})
else ()
	set (Libs ${UnixLibs})
endif (WIN32)

target_link_libraries (VulkanToy
PRIVATE
	${Libs}
	${Vulkan_LIBRARIES}
)

set_target_properties  (VulkanToy PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bin CXX_STANDARD 17 CXX_EXTENSIONS OFF)
