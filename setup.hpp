#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <string>
#include <optional>


namespace app
{
	class AppContext {
	public:
        // Window Related
		GLFWwindow* window;
		VkInstance instance;
		VkSurfaceKHR surface;

        // Device Related
        VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;

		// Queues
		std::uint32_t graphicsFamilyIndex = 0;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		std::uint32_t presentFamilyIndex = 0;
		VkQueue presentQueue = VK_NULL_HANDLE;


        // Debug Related
		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

        /// <summary>
        /// Cleans up and destroys all used memory structures before terminating the application
        /// </summary>
		void cleanup();
	};

	AppContext setup();

}