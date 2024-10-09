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
        /*
        std::cout << "Format: ";
        switch (file.GetFormat()) {
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Typeless:
            std::cout << "R32G32B32A32_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Float:
            std::cout << "R32G32B32A32_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_UInt:
            std::cout << "R32G32B32A32_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_SInt:
            std::cout << "R32G32B32A32_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Typeless:
            std::cout << "R32G32B32_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Float:
            std::cout << "R32G32B32_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_UInt:
            std::cout << "R32G32B32_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_SInt:
            std::cout << "R32G32B32_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Typeless:
            std::cout << "R16G16B16A16_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Float:
            std::cout << "R16G16B16A16_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UNorm:
            std::cout << "R16G16B16A16_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UInt:
            std::cout << "R16G16B16A16_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SNorm:
            std::cout << "R16G16B16A16_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SInt:
            std::cout << "R16G16B16A16_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_Typeless:
            std::cout << "R32G32_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_Float:
            std::cout << "R32G32_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_UInt:
            std::cout << "R32G32_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G32_SInt:
            std::cout << "R32G32_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32G8X24_Typeless:
            std::cout << "R32G8X24_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::D32_Float_S8X24_UInt:
            std::cout << "D32_Float_S8X24_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32_Float_X8X24_Typeless:
            std::cout << "R32_Float_X8X24_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::X32_Typeless_G8X24_UInt:
            std::cout << "X32_Typeless_G8X24_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_Typeless:
            std::cout << "R10G10B10A2_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UNorm:
            std::cout << "R10G10B10A2_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UInt:
            std::cout << "R10G10B10A2_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R11G11B10_Float:
            std::cout << "R11G11B10_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_Typeless:
            std::cout << "R8G8B8A8_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm:
            std::cout << "R8G8B8A8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB:
            std::cout << "R8G8B8A8_UNorm_SRGB";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UInt:
            std::cout << "R8G8B8A8_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SNorm:
            std::cout << "R8G8B8A8_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SInt:
            std::cout << "R8G8B8A8_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_Typeless:
            std::cout << "R16G16_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_Float:
            std::cout << "R16G16_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_UNorm:
            std::cout << "R16G16_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_UInt:
            std::cout << "R16G16_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_SNorm:
            std::cout << "R16G16_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16G16_SInt:
            std::cout << "R16G16_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32_Typeless:
            std::cout << "R32_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::D32_Float:
            std::cout << "D32_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32_Float:
            std::cout << "R32_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32_UInt:
            std::cout << "R32_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R32_SInt:
            std::cout << "R32_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R24G8_Typeless:
            std::cout << "R24G8_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::D24_UNorm_S8_UInt:
            std::cout << "D24_UNorm_S8_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R24_UNorm_X8_Typeless:
            std::cout << "R24_UNorm_X8_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::X24_Typeless_G8_UInt:
            std::cout << "X24_Typeless_G8_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_Typeless:
            std::cout << "R8G8_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_UNorm:
            std::cout << "R8G8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_UInt:
            std::cout << "R8G8_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_SNorm:
            std::cout << "R8G8_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_SInt:
            std::cout << "R8G8_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16_Typeless:
            std::cout << "R16_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16_Float:
            std::cout << "R16_Float";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::D16_UNorm:
            std::cout << "D16_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16_UNorm:
            std::cout << "R16_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16_UInt:
            std::cout << "R16_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16_SNorm:
            std::cout << "R16_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R16_SInt:
            std::cout << "R16_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8_Typeless:
            std::cout << "R8_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8_UNorm:
            std::cout << "R8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8_UInt:
            std::cout << "R8_UInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8_SNorm:
            std::cout << "R8_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8_SInt:
            std::cout << "R8_SInt";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::A8_UNorm:
            std::cout << "A8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R1_UNorm:
            std::cout << "R1_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R9G9B9E5_SHAREDEXP:
            std::cout << "R9G9B9E5_SHAREDEXP";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R8G8_B8G8_UNorm:
            std::cout << "R8G8_B8G8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::G8R8_G8B8_UNorm:
            std::cout << "G8R8_G8B8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC1_Typeless:
            std::cout << "BC1_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm:
            std::cout << "BC1_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB:
            std::cout << "BC1_UNorm_SRGB";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC2_Typeless:
            std::cout << "BC2_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm:
            std::cout << "BC2_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm_SRGB:
            std::cout << "BC2_UNorm_SRGB";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC3_Typeless:
            std::cout << "BC3_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm:
            std::cout << "BC3_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm_SRGB:
            std::cout << "BC3_UNorm_SRGB";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC4_Typeless:
            std::cout << "BC4_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm:
            std::cout << "BC4_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC4_SNorm:
            std::cout << "BC4_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC5_Typeless:
            std::cout << "BC5_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm:
            std::cout << "BC5_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC5_SNorm:
            std::cout << "BC5_SNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B5G6R5_UNorm:
            std::cout << "B5G6R5_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B5G5R5A1_UNorm:
            std::cout << "B5G5R5A1_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm:
            std::cout << "B8G8R8A8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B8G8R8X8_UNorm:
            std::cout << "B8G8R8X8_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::R10G10B10_XR_BIAS_A2_UNorm:
            std::cout << "R10G10B10_XR_BIAS_A2_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_Typeless:
            std::cout << "B8G8R8A8_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm_SRGB:
            std::cout << "B8G8R8A8_UNorm_SRGB";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B8G8R8X8_Typeless:
            std::cout << "B8G8R8X8_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B8G8R8X8_UNorm_SRGB:
            std::cout << "B8G8R8X8_UNorm_SRGB";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC6H_Typeless:
            std::cout << "BC6H_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC6H_UF16:
            std::cout << "BC6H_UF16";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC6H_SF16:
            std::cout << "BC6H_SF16";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC7_Typeless:
            std::cout << "BC7_Typeless";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm:
            std::cout << "BC7_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm_SRGB:
            std::cout << "BC7_UNorm_SRGB";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::AYUV:
            std::cout << "AYUV";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::Y410:
            std::cout << "Y410";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::Y416:
            std::cout << "Y416";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::NV12:
            std::cout << "NV12";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::P010:
            std::cout << "P010";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::P016:
            std::cout << "P016";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::YUV420_OPAQUE:
            std::cout << "YUV420_OPAQUE";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::YUY2:
            std::cout << "YUY2";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::Y210:
            std::cout << "Y210";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::Y216:
            std::cout << "Y216";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::NV11:
            std::cout << "NV11";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::AI44:
            std::cout << "AI44";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::IA44:
            std::cout << "IA44";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::P8:
            std::cout << "P8";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::A8P8:
            std::cout << "A8P8";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::B4G4R4A4_UNorm:
            std::cout << "B4G4R4A4_UNorm";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::P208:
            std::cout << "P208";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::V208:
            std::cout << "V208";
            break;
        case tinyddsloader::DDSFile::DXGIFormat::V408:
            std::cout << "V408";
            break;
        }
        std::cout << "\n";
        */
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