#include "images.hpp"

namespace utility {
	ImageSet createImageSet(app::AppContext& app, VmaAllocator allocator) {
		// Create the image and image view
		ImageSet imageSet;

		// Provide information about the image to set up
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		imageInfo.extent.width = app.swapchainExtent.width;
		imageInfo.extent.height = app.swapchainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// Information about the memory to be allocated
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		// Define and create the image and memory allocation
		if (vmaCreateImage(allocator, &imageInfo, &allocationInfo, &imageSet.image, &imageSet.allocation, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create allocate an image.");
		}

		// Information about the image view for the above created image
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = imageSet.image;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		imageViewInfo.components = VkComponentMapping{};
		imageViewInfo.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		if (vkCreateImageView(app.logicalDevice, &imageViewInfo, nullptr, &imageSet.imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image view.");
		}

		return imageSet;

	}
}