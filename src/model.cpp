#include "model.hpp"
#include "utility.hpp"

namespace model {
	Mesh createMesh(app::AppContext app, VmaAllocator allocator, 
		std::vector<glm::vec3> vPositions,
		std::vector<glm::vec3> vColours
	){
		// Size of the input data in bytes (use long long since the number can be very large)
		unsigned long long sizeOfPositions = vPositions.size() * sizeof(glm::vec3);
		unsigned long long sizeOfColours = vColours.size() * sizeof(glm::vec3);

		// Create the buffers on the GPU to hold the data
		utility::BufferSet positionBuffer = utility::createBuffer(
			allocator, 
			sizeOfPositions, 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_AUTO
		);
		utility::BufferSet colourBuffer = utility::createBuffer(
			allocator,
			sizeOfColours,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_AUTO
		);

		// Create the staging buffers
		utility::BufferSet positionStaging = utility::createBuffer(
			allocator,
			sizeOfPositions,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		);
		utility::BufferSet colourStaging = utility::createBuffer(
			allocator,
			sizeOfColours,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		);

		Mesh ret;

		return ret;


	}
}