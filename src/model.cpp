#include "model.hpp"

namespace model {
	Mesh createMesh(app::AppContext app, VmaAllocator& allocator, VkCommandPool commandPool,
		std::vector<glm::vec3>& vPositions,
		std::vector<glm::vec2>& vTextureCoords,
		std::vector<std::uint32_t>& vMaterials,
		std::vector<std::uint32_t>& indices,
		std::uint32_t materialID
	){
		// Size of the input data in bytes (use long long since the number can be very large)
		unsigned long long sizeOfPositions = vPositions.size() * sizeof(glm::vec3);
		unsigned long long sizeOfUVs = vTextureCoords.size() * sizeof(glm::vec2);
		unsigned long long sizeOfMatIDs = vMaterials.size() * sizeof(std::uint32_t);
		unsigned long long sizeOfIndices = indices.size() * sizeof(std::uint32_t);
		
		// Set up the vertex positions
		utility::BufferSet positionBuffer = setupMemoryBuffer(
			app, 
			allocator, 
			commandPool, 
			sizeOfPositions, 
			vPositions.data(), 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		);

		// Set up the vertex texture coordinates
		utility::BufferSet UVBuffer = setupMemoryBuffer(
			app,
			allocator,
			commandPool,
			sizeOfUVs,
			vTextureCoords.data(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		);

		// Set up the vertex material ids
		utility::BufferSet MatBuffer = setupMemoryBuffer(
			app,
			allocator,
			commandPool,
			sizeOfMatIDs,
			vMaterials.data(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		);

		// Set up the indices
		utility::BufferSet indexBuffer = setupMemoryBuffer(
			app,
			allocator,
			commandPool,
			sizeOfIndices,
			indices.data(),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		);

		Mesh outputMesh;

		outputMesh.vertexPositions = std::move(positionBuffer);
		outputMesh.vertexUVs = std::move(UVBuffer);
		outputMesh.vertexMaterials = std::move(MatBuffer);
		outputMesh.indices = std::move(indexBuffer);
		outputMesh.numberOfVertices = uint32_t(vPositions.size());
		outputMesh.numberOfIndices = uint32_t(indices.size());
		outputMesh.materialID = materialID;

		return outputMesh;

	}

	utility::BufferSet setupMemoryBuffer(app::AppContext app, VmaAllocator& allocator, VkCommandPool commandPool, VkDeviceSize sizeOfData, const void* data, VkBufferUsageFlags usageFlags) {
		// Set up the on GPU buffer
		utility::BufferSet buffer = utility::createBuffer(
			allocator,
			sizeOfData,
			usageFlags,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		);

		// Set up the staging buffer
		utility::BufferSet staging = utility::createBuffer(
			allocator,
			sizeOfData,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		);

		// Map the memory
		void* memory = nullptr;
		vmaMapMemory(allocator, staging.allocation, &memory);
		std::memcpy(memory, data, sizeOfData);
		vmaUnmapMemory(allocator, staging.allocation);

		// Create a command buffer to record the data into
		VkCommandBuffer commandBuffer = utility::createCommandBuffer(app, commandPool);

		// Begin recording into the command buffer
		VkCommandBufferBeginInfo recordInfo{};
		recordInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(commandBuffer, &recordInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to start command buffer recording.");
		}

		// Copy the data from staging to GPU
		VkBufferCopy copy{};
		copy.size = sizeOfData;
		vkCmdCopyBuffer(commandBuffer, staging.buffer, buffer.buffer, 1, &copy);
		utility::createBufferBarrier(buffer.buffer, VK_WHOLE_SIZE,
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

		vkFreeCommandBuffers(app.logicalDevice, commandPool, 1, &commandBuffer);
		vkDestroyFence(app.logicalDevice, submitComplete, nullptr);

		return buffer;

	}


}