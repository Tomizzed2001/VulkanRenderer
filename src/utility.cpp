#include "utility.hpp"
#include <cassert>

namespace utility {

	BufferSet::BufferSet() = default;

	BufferSet::BufferSet(VmaAllocator inAllocator, VkBuffer inBuffer, VmaAllocation inAllocation)
	{
		allocator = inAllocator;
		buffer = inBuffer;
		allocation = inAllocation;
	}

	BufferSet::~BufferSet()
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(allocator, buffer, allocation);
		}
	}

	BufferSet::BufferSet(BufferSet&& other)	noexcept
		: allocator(std::exchange(other.allocator, VK_NULL_HANDLE))
		, buffer(std::exchange(other.buffer, VK_NULL_HANDLE))
		, allocation(std::exchange(other.allocation, VK_NULL_HANDLE))
	{
	}

	BufferSet& BufferSet::operator=(BufferSet&& other) noexcept
	{
		std::swap(allocator, other.allocator);
		std::swap(buffer, other.buffer);
		std::swap(allocation, other.allocation);
		return *this;
	}

	BufferSet createBuffer(
		VmaAllocator& allocator,
		VkDeviceSize sizeOfData, 
		VkBufferUsageFlags usageFlags,
		VmaMemoryUsage memoryUsageFlags,
		VmaAllocationCreateFlags memoryFlags
	) {
		// Set up the buffer info data structure
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeOfData;
		bufferInfo.usage = usageFlags;

		// Set up the details about the VMA allocator
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = memoryUsageFlags;
		allocationInfo.flags = memoryFlags;		

		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;

		// Create the buffer
		if (vmaCreateBuffer(allocator, &bufferInfo, &allocationInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create buffer.");
		}

		return BufferSet(allocator, buffer, allocation);
	}

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
	) {
		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.buffer = buffer;
		bufferBarrier.size = sizeOfBuffer;
		bufferBarrier.offset = 0;
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
		VkPipelineStageFlags aDstStageFlags
	) {
		VkImageMemoryBarrier imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.image = image;
		imageBarrier.oldLayout = srcLayout;
		imageBarrier.newLayout = dstLayout;
		imageBarrier.srcAccessMask = srcAccessMask;
		imageBarrier.dstAccessMask = dstAccessMask;
		imageBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		imageBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		imageBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipmapLevels, 0, 1 };

		vkCmdPipelineBarrier(
			commandBuffer, aSrcStageFlags, aDstStageFlags, 0, // Buffer details
			0, nullptr,				// Memory barriers
			0, nullptr,				// Buffer barriers
			1, &imageBarrier		// Image  barriers
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