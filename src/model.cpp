#include "model.hpp"

namespace model {
	Mesh createMesh(app::AppContext app, VmaAllocator& allocator,
		std::vector<glm::vec3>& vPositions,
		std::vector<glm::vec2>& vTextureCoords,
		std::vector<std::uint32_t>& indices,
		std::uint32_t materialID
	){
		// Size of the input data in bytes (use long long since the number can be very large)
		unsigned long long sizeOfPositions = vPositions.size() * sizeof(glm::vec3);
		unsigned long long sizeOfUVs = vTextureCoords.size() * sizeof(glm::vec2);
		unsigned long long sizeOfIndices = indices.size() * sizeof(std::uint32_t);
		
		// Create the buffers on the GPU to hold the data
		utility::BufferSet positionBuffer = utility::createBuffer(
			allocator, 
			sizeOfPositions, 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		);
		utility::BufferSet UVBuffer = utility::createBuffer(
			allocator,
			sizeOfUVs,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		);
		utility::BufferSet indexBuffer = utility::createBuffer(
			allocator,
			sizeOfIndices,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		);
		// Create the staging buffers
		utility::BufferSet positionStaging = utility::createBuffer(
			allocator,
			sizeOfPositions,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		);
		utility::BufferSet UVStaging = utility::createBuffer(
			allocator,
			sizeOfUVs,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		);	
		utility::BufferSet indexStaging = utility::createBuffer(
			allocator,
			sizeOfIndices,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		);

		// Map the memory
		void* positionMemory = nullptr;
		vmaMapMemory(allocator, positionStaging.allocation, &positionMemory);
		std::memcpy(positionMemory, vPositions.data(), sizeOfPositions);
		vmaUnmapMemory(allocator, positionStaging.allocation);

		void* UVMemory = nullptr;
		vmaMapMemory(allocator, UVStaging.allocation, &UVMemory);
		std::memcpy(UVMemory, vTextureCoords.data(), sizeOfUVs);
		vmaUnmapMemory(allocator, UVStaging.allocation);

		void* indexMemory = nullptr;
		vmaMapMemory(allocator, indexStaging.allocation, &indexMemory);
		std::memcpy(indexMemory, indices.data(), sizeOfIndices);
		vmaUnmapMemory(allocator, indexStaging.allocation);

		// Create a command buffer to record the data into
		VkCommandPool commandPool = utility::createCommandPool(app, 0);
		VkCommandBuffer commandBuffer = utility::createCommandBuffer(app, commandPool);

		// Begin recording into the command buffer
		VkCommandBufferBeginInfo recordInfo{};
		recordInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(commandBuffer, &recordInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to start command buffer recording.");
		}

		// Copy the data from staging to GPU
		VkBufferCopy positionCopy{};
		positionCopy.size = sizeOfPositions;
		vkCmdCopyBuffer(commandBuffer, positionStaging.buffer, positionBuffer.buffer, 1, &positionCopy);
		utility::createBufferBarrier(positionBuffer.buffer, VK_WHOLE_SIZE,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
		);

		VkBufferCopy UVCopy{};
		UVCopy.size = sizeOfUVs;
		vkCmdCopyBuffer(commandBuffer, UVStaging.buffer, UVBuffer.buffer, 1, &UVCopy);
		utility::createBufferBarrier(UVBuffer.buffer, VK_WHOLE_SIZE,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
		);

		VkBufferCopy indexCopy{};
		indexCopy.size = sizeOfIndices;
		vkCmdCopyBuffer(commandBuffer, indexStaging.buffer, indexBuffer.buffer, 1, &indexCopy);
		utility::createBufferBarrier(indexBuffer.buffer, VK_WHOLE_SIZE,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
		);

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

		Mesh outputMesh;

		outputMesh.vertexPositions = std::move(positionBuffer);
		outputMesh.vertexUVs = std::move(UVBuffer);
		outputMesh.indices = std::move(indexBuffer);
		outputMesh.numberOfVertices = uint32_t(vPositions.size());
		outputMesh.numberOfIndices = uint32_t(indices.size());
		outputMesh.materialID = materialID;
		
		// Clean up before returning
		vkFreeCommandBuffers(app.logicalDevice, commandPool, 1, &commandBuffer);
		vkDestroyCommandPool(app.logicalDevice, commandPool, nullptr);
		vkDestroyFence(app.logicalDevice, submitComplete, nullptr);

		return outputMesh;

	}
}