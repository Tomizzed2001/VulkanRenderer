#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include "setup.hpp"
#include "utility.hpp"

namespace utility {
	/// <summary>
	/// A class to represent an image and image view combination for usage with buffers
	/// </summary>
	class ImageSet
	{
	public:
		VkImage image = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
	};

	/// <summary>
	/// Creates an image, image view and memory allocation
	/// </summary>
	/// <param name="app">Context of the application</param>
	/// <param name="allocator">Memory allocator</param>
	/// <param name="format">Image format</param>
	/// <param name="usageFlags">Usage flags for the image</param>
	/// <param name="aspectFlagBits">Image aspect flags</param>
	/// <returns>Class containing image, image view and allocation</returns>
	ImageSet createImageSet(app::AppContext& app, VmaAllocator& allocator, VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlagBits aspectFlagBits);

	/// <summary>
	/// Creates an image texture set given a compressed dds file
	/// </summary>
	/// <param name="app">Application context</param>
	/// <param name="filePath">Path to the .dds file</param>
	/// <param name="allocator">Memory allocator</param>
	/// <param name="commandPool">Command pool</param>
	/// <returns>An image set containing the VkImage and VkImageView</returns>
	ImageSet createDDSTextureImageSet(app::AppContext& app, char const* filePath, 
		VmaAllocator& allocator, VkCommandPool commandPool);

	/// <summary>
	/// Creates a texture array for all the given dds images assuming that each image
	/// is of the same size and format.
	/// </summary>
	/// <param name="app">Application context</param>
	/// <param name="filePaths">Paths to the .dds files</param>
	/// <param name="allocator">Memory allocator</param>
	/// <param name="commandPool">Command pool</param>
	/// <returns>An image set containing the VkImage and VkImageView</returns>
	ImageSet createDDSTextureArray(app::AppContext& app, std::vector<std::string> filePaths,
		VmaAllocator& allocator, VkCommandPool commandPool);
}
