#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <string>
#include <optional>

#include "setup.hpp"

namespace {
    /// <summary>
    /// Creates a memory allocator using the vma library for Vulkan
    /// memory allocation.
    /// </summary>
    /// <param name="app">The context of the application</param>
    /// <returns>Vulkan Memory Allocator</returns>
    VmaAllocator createMemoryAllocator(app::AppContext& app);
}

int main() {
    try {
        // -- The setup -- //
        app::AppContext application = app::setup();

        // Create the memory allocator
        VmaAllocator allocator = createMemoryAllocator(application);

        // Main render loop
        while (!glfwWindowShouldClose(application.window)) {
            glfwPollEvents();
        }

        // Clean up and close the application
        application.cleanup();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

namespace {
    VmaAllocator createMemoryAllocator(app::AppContext& app) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(app.physicalDevice, &properties);

        VmaVulkanFunctions functions{};
        functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocInfo{};
        allocInfo.vulkanApiVersion = properties.apiVersion;
        allocInfo.physicalDevice = app.physicalDevice;
        allocInfo.device = app.logicalDevice;
        allocInfo.instance = app.instance;
        allocInfo.pVulkanFunctions = &functions;

        VmaAllocator allocator = VK_NULL_HANDLE;
        if (vmaCreateAllocator(&allocInfo, &allocator) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create VmaAllocator");
        }

        return allocator;
    }
}