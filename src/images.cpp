#include "images.hpp"


#define TINYDDSLOADER_IMPLEMENTATION
#include "tinyddsloader.h"

namespace utility {
	ImageSet createImageSet(app::AppContext& app, VmaAllocator& allocator, VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlagBits aspectFlagBits) {
		// Create the image and image view
		ImageSet imageSet;

		// Provide information about the image to set up
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent.width = app.swapchainExtent.width;
		imageInfo.extent.height = app.swapchainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = usageFlags; //VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// Information about the memory to be allocated
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		// Define and create the image and memory allocation
		if (vmaCreateImage(allocator, &imageInfo, &allocationInfo, &imageSet.image, &imageSet.allocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create an image.");
		}

		// Information about the image view for the above created image
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = imageSet.image;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = format;
		imageViewInfo.components = VkComponentMapping{};
		imageViewInfo.subresourceRange = VkImageSubresourceRange{ unsigned int(aspectFlagBits), 0, 1, 0, 1 };

		if (vkCreateImageView(app.logicalDevice, &imageViewInfo, nullptr, &imageSet.imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image view.");
		}

		return imageSet;

	}

	ImageSet createDDSTextureImageSet(app::AppContext& app, char const* filePath, VmaAllocator& allocator,
		VkCommandPool commandPool) {
		
        // Load in the dds file using the tinyddsloader header library
		tinyddsloader::DDSFile file;
		if (file.Load(filePath)) {
			throw std::runtime_error("Failed to load dds file.");
		}

        // Flip the texture
		file.Flip();

		// TODO: Get the exact format and adjust
		VkFormat format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		//std::uint32_t blockSize = 8;
		
		VkExtent3D extent(file.GetWidth(), file.GetHeight(), file.GetDepth());
        
		// Get the image data size (including all mip maps)
        std::uint32_t totalDataSize = 0;
        for (std::uint32_t i = 0; i < file.GetMipCount(); i++) {
		    tinyddsloader::DDSFile::ImageData const* data = file.GetImageData(i,0);
            //totalDataSize += (data->m_width / 4) * (data->m_height / 4) * blockSize;
            totalDataSize += data->m_memSlicePitch;
        }

		// Create the staging buffer
		utility::BufferSet stagingBuffer = utility::createBuffer(
			allocator,
            totalDataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		);

		// Map the memory
		void* dataMemory = nullptr;
		vmaMapMemory(allocator, stagingBuffer.allocation, &dataMemory);
        
        // Set the offset of the mip level data
        std::uint32_t dataOffset = 0;
        // Copy each mip level into the buffer
        for (std::uint32_t i = 0; i < file.GetMipCount(); i++) {
            // Get the mip level image data
            tinyddsloader::DDSFile::ImageData const* data = file.GetImageData(i, 0);
            // Get the data size
            //std::uint32_t dataSize = (data->m_width / 4) * (data->m_height / 4) * blockSize;
		    std::memcpy(static_cast<std::uint8_t*>(dataMemory) + dataOffset, data->m_mem, data->m_memSlicePitch);
            dataOffset += data->m_memSlicePitch;
        }
        
        vmaUnmapMemory(allocator, stagingBuffer.allocation);

		// Create the image
		ImageSet imageSet;

		// Provide information about the image to set up
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent = extent;
		imageInfo.mipLevels = file.GetMipCount();
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		// Provide information about the memory allocation for the image
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.flags = 0;
		allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		// Create the image
		if (vmaCreateImage(allocator, &imageInfo, &allocationInfo, &imageSet.image, &imageSet.allocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create VkImage for texture.");
		}

		// Create a command buffer to upload the image
		VkCommandBuffer commandBuffer = utility::createCommandBuffer(app, commandPool);

		// Begin recording into the command buffer
		VkCommandBufferBeginInfo recordInfo{};
		recordInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(commandBuffer, &recordInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to start command buffer recording.");
		}

		// Transition the buffer so it can be copied
		createImageBarrier(imageSet.image,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
            file.GetMipCount(),
			commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Do the copy for each mip level
        dataOffset = 0;
        for (std::uint32_t i = 0; i < file.GetMipCount(); i++) {
            // Get the mip level image data
            tinyddsloader::DDSFile::ImageData const* data = file.GetImageData(i, 0);
            // Get the data size
            VkExtent3D mipExtent(data->m_width, data->m_height, data->m_depth);
            //std::uint32_t dataSize = (mipExtent.width / 4) * (mipExtent.height / 4) * blockSize;
            
            // Set up the copy details
            VkBufferImageCopy copyBuffer;
            copyBuffer.bufferOffset = dataOffset;
            copyBuffer.bufferRowLength = 0;
            copyBuffer.bufferRowLength = 0;
            copyBuffer.imageSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 };
            copyBuffer.imageOffset = { 0,0,0 };
            copyBuffer.imageExtent = mipExtent;

            // Do the copy
            vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, imageSet.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyBuffer);

            // Add onto the data offset
            dataOffset += data->m_memSlicePitch;
        }

		// Transition image to be shader readable
		createImageBarrier(imageSet.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            file.GetMipCount(),
			commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		// End the recording
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to end command buffer recording.");
		}

		// Use a fence to ensure that transfers are complete before moving on
		VkFence submitComplete = utility::createFence(app);

		// Submit the recorded commands for execution
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		if (vkQueueSubmit(app.graphicsQueue, 1, &submitInfo, submitComplete)) {
			throw std::runtime_error("Failed to submit recorded commands.");
		}

		// Wait for the fence before clean up
		if (vkWaitForFences(app.logicalDevice, 1, &submitComplete, VK_TRUE, std::numeric_limits<std::uint64_t>::max())) {
			throw std::runtime_error("Fence failed to return as complete.");
		}

		// Clean up of image creation
		vkFreeCommandBuffers(app.logicalDevice, commandPool, 1, &commandBuffer);
		vkDestroyFence(app.logicalDevice, submitComplete, nullptr);

		// Create the image view
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = imageSet.image;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = format;
		imageViewInfo.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, file.GetMipCount(), 0, 1};

		if (vkCreateImageView(app.logicalDevice, &imageViewInfo, nullptr, &imageSet.imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create textured image view.");
		}

		return imageSet;
	}


}