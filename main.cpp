#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>



namespace {
    // Variables to be used throughout the application
    GLFWwindow* window;
    VkInstance instance;


    /// <summary>
    /// GLFW window creation settings
    /// </summary>
    namespace windowSettings{
        const uint32_t width = 800;
        const uint32_t height = 600;
        const char* name = "Vulkan Renderer";
    }


    // Declaration of functions to be implemented
    GLFWwindow* createWindow(uint32_t width, uint32_t height, const char* name);
    VkInstance createInstance();
    void cleanAndClose();
}

int main() {
    try {
        // Create a window using GLFW
        GLFWwindow* window = createWindow(windowSettings::width, windowSettings::height, windowSettings::name);

        // Create a vulkan instance
        VkInstance instance = createInstance();

        // Main render loop
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

        // Clean up and close the application
        cleanAndClose();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

namespace {

    /// <summary>
    /// Creates a GLFW resizable window that matches the given parameters
    /// </summary>
    /// <param name="width">Width of the window</param>
    /// <param name="height">Height of the window</param>
    /// <param name="name">Name of the window</param>
    /// <returns>A GLFW window</returns>
    GLFWwindow* createWindow(const uint32_t width, const uint32_t height, const char* name) {
        // Initialise glfw
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(width, height, name, nullptr, nullptr);

        return window;
    }

    /// <summary>
    /// Creates a Vulkan instance that allows the Vulkan library to be used for the application.
    /// This function specifies which extensions and layers can be used with this application. 
    /// </summary>
    /// <returns>Vulkan instance</returns>
    VkInstance createInstance() {
        // Fill out the details for the application info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);

        // Get the extensions required for GLFW
        std::uint32_t extensionCount = 0;
        char const** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

        // Fill out the details for the instance info
        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.enabledExtensionCount = extensionCount;
        instanceInfo.ppEnabledExtensionNames = extensionNames;
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pApplicationInfo = &appInfo;

        VkInstance instance;
        if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Instance creation failed.");
        }
        return instance;

    }

    /// <summary>
    /// Cleans up and destroys all used memory structures before terminating the application
    /// </summary>
    void cleanAndClose() {
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

}