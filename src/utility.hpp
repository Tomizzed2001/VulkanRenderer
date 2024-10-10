#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>
#include "setup.hpp"

namespace utility {
	/// <summary>
	/// 
	/// </summary>
	class BufferSet
	{
	public:
		
		/// <summary>
		/// Default constructor
		/// </summary>
		BufferSet();

		/// <summary>
		/// Parameterised constructor
		/// </summary>
		/// <param name="allocator">Memory allocator</param>
		/// <param name="buffer">Memory buffer</param>
		/// <param name="allocation">Memory allocation</param>
		BufferSet(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

		/// <summary>
		/// Destructor
		/// </summary>
		~BufferSet();
		
		// Delete the copy constructors to avoid duplicating the buffer by accident
		BufferSet(BufferSet&) = delete;
		BufferSet& operator= (BufferSet&) = delete;

		/// <summary>
		/// Move constructor (Should not throw exceptions)
		/// </summary>
		/// <param name="other">The original buffer</param>
		BufferSet(BufferSet&& other) noexcept;
		
		/// <summary>
		/// Move assignment operator (Should not throw exceptions)
		/// </summary>
		/// <param name="other">The original buffer</param>
		/// <returns>A buffer set</returns>
		BufferSet& operator = (BufferSet&& other) noexcept;

	public:
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocator allocator = VK_NULL_HANDLE;

	};

	/// <summary>
	/// Creates a buffer for a given data size with the set data flags
	/// </summary>
	/// <param name="allocator">Vulkan Memory Allocator</param>
	/// <param name="sizeOfData">Size of the data used by the buffer</param>
	/// <param name="usageFlags">Any usage flags for the buffer</param>
	/// <param name="memoryUsageFlags">Any usage flags for the memory allocation</param>
	/// <param name="memoryFlags">Any flags for the allocation</param>
	/// <returns>A buffer object which maintains track of its memory</returns>
	BufferSet createBuffer(
		VmaAllocator& allocator,
		VkDeviceSize sizeOfData,
		VkBufferUsageFlags usageFlags,
		VmaMemoryUsage memoryUsageFlags,
		VmaAllocationCreateFlags memoryFlags = 0
	);

	/// <summary>
	/// Creates a buffer barrier in the memory to wait for any GPU data transfers to complete
	/// </summary>
	/// <param name="buffer">The buffer being transferred</param>
	/// <param name="sizeOfBuffer">The size of the buffer being transferred</param>
	/// <param name="srcAccessMask">Current access flag(s)</param>
	/// <param name="dstAccessMask">Future access flag(s)</param>
	/// <param name="srcQueueFamilyIndex">Current queue flag(s)</param>
	/// <param name="dstQueueFamilyIndex">Future queue flag(s)</param>
	/// <param name="commandBuffer">Command buffer to hold</param>
	/// <param name="aSrcStageFlags">Current staging flag(s)</param>
	/// <param name="aDstStageFlags">Future staging flag(s)</param>
	void createBufferBarrier(
		VkBuffer buffer,
		VkDeviceSize sizeOfBuffer,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		std::uint32_t srcQueueFamilyIndex,
		std::uint32_t dstQueueFamilyIndex,
		VkCommandBuffer commandBuffer,
		VkPipelineStageFlags aSrcStageFlags,
		VkPipelineStageFlags aDstStageFlags
	);

	/// <summary>
	/// Creates an image barrier in the memory to wait for any GPU data transfers to complete
	/// </summary>
	/// <param name="buffer">The buffer being transferred</param>
	/// <param name="sizeOfBuffer">The size of the buffer being transferred</param>
	/// <param name="srcAccessMask">Current access flag(s)</param>
	/// <param name="dstAccessMask">Future access flag(s)</param>
	/// <param name="srcQueueFamilyIndex">Current queue flag(s)</param>
	/// <param name="dstQueueFamilyIndex">Future queue flag(s)</param>
	/// <param name="mipmapLevels">Number of mipmap levels</param>
	/// <param name="commandBuffer">Command buffer to hold</param>
	/// <param name="aSrcStageFlags">Current staging flag(s)</param>
	/// <param name="aDstStageFlags">Future staging flag(s)</param>
	/// <param name="numLayers">Number of layers (1 if not an array)</param>
	void createImageBarrier(
		VkImage image,
		VkImageLayout srcLayout,
		VkImageLayout dstLayout,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		std::uint32_t srcQueueFamilyIndex,
		std::uint32_t dstQueueFamilyIndex,
		std::uint32_t mipmapLevels,
		VkCommandBuffer commandBuffer,
		VkPipelineStageFlags aSrcStageFlags,
		VkPipelineStageFlags aDstStageFlags,
		uint32_t numLayers = 1
	);

	/// <summary>
	/// Creates a command pool with the given flags
	/// </summary>
	/// <param name="app">Application context</param>
	/// <param name="flags">The flags to apply to the command pool</param>
	/// <returns>A Vulkan command pool</returns>
	VkCommandPool createCommandPool(app::AppContext& app, VkCommandPoolCreateFlags flags);

	/// <summary>
	/// Creates a command buffer given a command pool
	/// </summary>
	/// <param name="app">Application context</param>
	/// <param name="commandPool">The command pool</param>
	/// <returns>The command buffer</returns>
	VkCommandBuffer createCommandBuffer(app::AppContext& app, VkCommandPool commandPool);

	/// <summary>
	/// Creates a fence given a set of flags
	/// </summary>
	/// <param name="app">The application context</param>
	/// <param name="aFlags">Flag to apply to the fence. Is either signalled or unsignalled by default</param>
	/// <returns>A fence</returns>
	VkFence createFence(app::AppContext& app, VkFenceCreateFlags flag = 0);

	/// <summary>
	/// Creates a semaphore that can be used to wait for completed executions
	/// </summary>
	/// <param name="app">The application context</param>
	/// <param name="flag">Flag to apply to the semaphore. Is is unsignalled by default.</param>
	/// <returns>Vulkan semaphore</returns>
	VkSemaphore createSemaphore(app::AppContext& app, VkSemaphoreCreateFlags flag);
}