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
        // Window
		GLFWwindow* window;
		VkInstance instance;
		VkSurfaceKHR surface;

        // Device
        VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;

		// Queues
		std::vector<std::uint32_t> queueFamilyIndices;
		std::uint32_t graphicsFamilyIndex = 0;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		std::uint32_t presentFamilyIndex = 0;
		VkQueue presentQueue = VK_NULL_HANDLE;

		// Swapchain
		VkSwapchainKHR swapchain;
		VkFormat swapchainFormat;
		VkExtent2D swapchainExtent;
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;

        // Debug Related
		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

        /// <summary>
        /// Cleans up and destroys all used memory structures before terminating the application
        /// </summary>
		void cleanup();
	};

	/// <summary>
	/// Sets up the vulkan application
	/// </summary>
	/// <returns>The application context / settings</returns>
	AppContext setup();

	/// <summary>
	/// Creates the swapchain for the application
	/// </summary>
	/// <param name="aApp">The application context</param>
	void swapchainSetup(app::AppContext* aApp);

	/// <summary>
	/// Creates the swapchain images for the application's swapchain
	/// </summary>
	/// <param name="aApp">The application context</param>
	void createSwapchainImages(app::AppContext* aApp);

}