#include "utility.hpp"

namespace utility {

	BufferSet::BufferSet() = default;

	BufferSet::~BufferSet() {
		vmaDestroyBuffer(allocator, buffer, allocation);
	}

	BufferSet createBuffer(
		VmaAllocator& allocator, 
		VkDeviceSize sizeOfData, 
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

	void createBarrier(
		VkBuffer buffer, 
		unsigned long long sizeOfBuffer, 
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		std::uint32_t srcQueueFamilyIndex,
		std::uint32_t dstQueueFamilyIndex,
		VkCommandBuffer commandBuffer,
		VkPipelineStageFlags aSrcStageFlags,
		VkPipelineStageFlags aDstStageFlags
	) {
		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.buffer = buffer;
		bufferBarrier.size = sizeOfBuffer;
		bufferBarrier.srcAccessMask = srcAccessMask;
		bufferBarrier.dstAccessMask = dstAccessMask;
		bufferBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		bufferBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

		vkCmdPipelineBarrier(
			commandBuffer, aSrcStageFlags, aDstStageFlags, 0, // Buffer details
			0, nullptr,				// Memory barriers
			1, &bufferBarrier,		// Buffer barriers
			0, nullptr				// Image  barriers
		);
	}

	VkCommandPool createCommandPool(app::AppContext& app, VkCommandPoolCreateFlags flags) {
		// Set the required information for the command pool
		VkCommandPoolCreateInfo commandPoolInfo{};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.queueFamilyIndex = app.graphicsFamilyIndex;
		commandPoolInfo.flags = flags;

		// Create the command pool
		VkCommandPool commandPool;
		if (vkCreateCommandPool(app.logicalDevice, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create command pool.");
		}

		return commandPool;
	}

	VkCommandBuffer createCommandBuffer(app::AppContext& app, VkCommandPool commandPool) {
		// Set the information required of the command buffer
		VkCommandBufferAllocateInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandPool = commandPool;
		commandBufferInfo.commandBufferCount = 1;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		// Create / allocate the command buffer
		VkCommandBuffer commandBuffer;
		if (vkAllocateCommandBuffers(app.logicalDevice, &commandBufferInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create command buffer.");
		}

		return commandBuffer;
	}

	VkFence createFence(app::AppContext& app, VkFenceCreateFlags flag) {
		// Set the information required to create the fence
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = flag;

		// Create the fence
		VkFence fence;
		if (vkCreateFence(app.logicalDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create fence.");
		}

		return fence;
	}

	VkSemaphore createSemaphore(app::AppContext& app, VkSemaphoreCreateFlags flag) {
		// Set the information required to create the semaphore
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.flags = flag;

		// Create the semaphore
		VkSemaphore semaphore;
		if (vkCreateSemaphore(app.logicalDevice, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphore.");
		}

		return semaphore;
	}
}