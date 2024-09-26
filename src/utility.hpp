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
		BufferSet(), ~BufferSet();

		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocator allocator;

	};
	
	/// <summary>
	/// Destructor of the buffer set class
	/// </summary>
	/*
	BufferSet::~BufferSet() {
		vmaDestroyBuffer(allocator, buffer, allocation);
	}
	*/

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
		VmaAllocator allocator,
		unsigned long long sizeOfData,
		VkBufferUsageFlags usageFlags,
		VmaMemoryUsage memoryUsageFlags,
		VmaAllocationCreateFlags memoryFlags = 0
	);
}