#include "images.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

        // Set a default format and check to see if it different
        VkFormat format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;

        switch (file.GetFormat()) {
            case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm:
                format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB:
                format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm:
                format = VK_FORMAT_BC2_UNORM_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm_SRGB:
                format = VK_FORMAT_BC2_SRGB_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm:
                format = VK_FORMAT_BC3_UNORM_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm_SRGB:
                format = VK_FORMAT_BC3_SRGB_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm:
                format = VK_FORMAT_BC4_UNORM_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC4_SNorm:
                format = VK_FORMAT_BC4_SNORM_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm:
                format = VK_FORMAT_BC5_UNORM_BLOCK;
                break;
            case tinyddsloader::DDSFile::DXGIFormat::BC5_SNorm:
                format = VK_FORMAT_BC5_SNORM_BLOCK;
                break;
            default:
                std::cout << "Not a compressed texture";
        }
		
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

		if (format != VK_FORMAT_BC1_RGB_UNORM_BLOCK) {
			imageSet.isAlpha = true;
		}

		return imageSet;
	}

	ImageSet createPNGTextureImageSet(app::AppContext& app, char const* filePath, VmaAllocator& allocator,
		VkCommandPool commandPool) {

		// Flip the texture
		stbi_set_flip_vertically_on_load(1);

		// Load in the png file using the stb image library
		int width, height, channels;
		stbi_uc* imageData = stbi_load(filePath, &width, &height, &channels, 4);

		// Set a default format
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

		VkExtent3D extent(width, height, 1);

		// Get the image data size
		std::uint32_t totalDataSize = width * height * 4;		

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
		// Copy the data into the staging buffer		
		std::memcpy(dataMemory, imageData, totalDataSize);
		vmaUnmapMemory(allocator, stagingBuffer.allocation);
	
		// Free the image data
		stbi_image_free(imageData);

		// Get the number of mip map levels
		// Taken from https://vulkan-tutorial.com/Generating_Mipmaps
		// Max gets the largest dimension
		// Logs2 is how many times it can be divided by 2
		// Floor solves problems that may occur if dimensions are not to a power of 2
		// 1 is added for the base mip level
		std::uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

		// Create the image
		ImageSet imageSet;

		// Provide information about the image to set up
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent = extent;
		imageInfo.mipLevels = mipLevels;
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
			mipLevels,
			commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Set up the copy details
		VkBufferImageCopy copyBuffer;
		copyBuffer.bufferOffset = 0;
		copyBuffer.bufferRowLength = 0;
		copyBuffer.bufferRowLength = 0;
		copyBuffer.imageSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyBuffer.imageOffset = { 0,0,0 };
		copyBuffer.imageExtent = extent;

		// Do the copy
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, imageSet.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyBuffer);

		// Transition image for the current mip map
		createImageBarrier(imageSet.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			1,
			commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Define starting dimensions
		int mipWidth = width;
		int mipHeight = height;

		// Copy the mip level data
		for (std::uint32_t mipLevel = 1; mipLevel < mipLevels; mipLevel++) {
			// Blit the previous mip level down to the current level
			VkImageBlit blit{};
			blit.srcSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, mipLevel -1, 0, 1 };
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.dstSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 0, 1 };
			blit.dstOffsets[0] = { 0, 0, 0 };
			
			// Check for the dimension of the current mip level
			// Due to integer division always set to 1 if less than 1 in case of 0 
			if (mipWidth > 1) {
				mipWidth = mipWidth / 2;
			}
			else {
				mipWidth = 1;
			}
			if (mipHeight > 1) {
				mipHeight = mipHeight / 2;
			}
			else {
				mipHeight = 1;
			}
			blit.dstOffsets[1] = { mipWidth, mipHeight, 1 };

			vkCmdBlitImage(commandBuffer,
				imageSet.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				imageSet.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR
			);

			createImageBarrier(imageSet.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				mipLevel,
				commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		}

		// Transition image to be shader readable
		createImageBarrier(imageSet.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			mipLevels,
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
		imageViewInfo.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1 };

		if (vkCreateImageView(app.logicalDevice, &imageViewInfo, nullptr, &imageSet.imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create textured image view.");
		}

		return imageSet;
	}

}