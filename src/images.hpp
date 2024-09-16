#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include "setup.hpp"

namespace utility {
	/// <summary>
	/// A class to represent an image and image view combination for usage with buffers
	/// </summary>
	class ImageSet
	{
	public:
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
	};

	/// <summary>
	/// Creates an image, image view and memory allocation
	/// </summary>
	/// <param name="app">Context of the application</param>
	/// <param name="allocator">Memory allocator</param>
	/// <returns>Class containing image, image view and allocation</returns>
	ImageSet createBuffer(app::AppContext& app, VmaAllocator allocator);

}
