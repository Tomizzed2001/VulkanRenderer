#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>

#include <vk_mem_alloc.h>
#include "setup.hpp"
#include "utility.hpp"

#include "glm.hpp"
#include "vec3.hpp"

namespace model {
	struct Mesh {
		// Vertex data
		utility::BufferSet vertexPositions;
		utility::BufferSet vertexUVs;
		utility::BufferSet vertexMaterials;
		utility::BufferSet indices;

		// Size data
		std::uint32_t numberOfVertices;
		std::uint32_t numberOfIndices;

		// Material data
		std::uint32_t materialID;
	};

	/// <summary>
	/// Creates a mesh and completes all of the necessary memory steps required for the data to be used.
	/// </summary>
	/// <param name="app">Application context</param>
	/// <param name="allocator">Vulkan memory allocator</param>
	/// <param name="commandPool">Vulkan command pool</param>
	/// <param name="vPositions">Vertex positions</param>
	/// <param name="vTextureCoords">Vertex texture coords</param>
	/// <param name="vMaterials">Vertex material ids</param>
	/// <param name="indices">Vertex indices</param>
	/// <returns>A mesh data structure</returns>
	Mesh createMesh(app::AppContext app, VmaAllocator& allocator, VkCommandPool commandPool,
		std::vector<glm::vec3>& vPositions, 
		std::vector<glm::vec2>& vTextureCoords,
		std::vector<std::uint32_t>& vMaterials,
		std::vector<std::uint32_t>& indices
	);

	/// <summary>
	/// Sets up a memory buffer for a given set of data on the GPU
	/// </summary>
	/// <param name="app">Application context</param>
	/// <param name="allocator">Vulkan memory allocator</param>
	/// <param name="commandPool">Vulkan command pool</param>
	/// <param name="sizeOfData">The size of the input data</param>
	/// <param name="data">A pointer to the input data</param>
	/// <param name="usageFlags">Usage flags for the buffer</param>
	/// <returns>A memory buffer</returns>
	utility::BufferSet setupMemoryBuffer(app::AppContext app, VmaAllocator& allocator, VkCommandPool commandPool,
		VkDeviceSize sizeOfData, 
		const void* data, 
		VkBufferUsageFlags usageFlags
	);
}