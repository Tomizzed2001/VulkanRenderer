#include "utility.hpp"

namespace utility {

	BufferSet::BufferSet() = default;

	BufferSet::~BufferSet() {
		vmaDestroyBuffer(allocator, buffer, allocation);
	}

	BufferSet createBuffer(
		VmaAllocator allocator, 
		unsigned long long sizeOfData, 
		VkBufferUsageFlags usageFlags,
		VmaMemoryUsage memoryUsageFlags,
		VmaAllocationCreateFlags memoryFlags
	) {
		// Set up the details about the VMA allocator
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = memoryUsageFlags;
		allocationInfo.flags = memoryFlags;		
		
		// Set up the buffer info data structure
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeOfData;
		bufferInfo.usage = usageFlags;

		// Create the buffer
		BufferSet bufferSet;
		if (vmaCreateBuffer(allocator, &bufferInfo, &allocationInfo, &bufferSet.buffer, &bufferSet.allocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create buffer.");
		}

		bufferSet.allocator = allocator;

		return bufferSet;
	}
}