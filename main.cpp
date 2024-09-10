#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <string>



namespace {
    // Variables to be used throughout the application
    GLFWwindow* window;
    VkInstance vulkanInstance;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface;


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
    void createDebugMessenger();
    void deviceSetup();
    VkPhysicalDevice select_device();
    bool findQueueFamily(VkPhysicalDevice aPhysicalDev, VkQueueFlags aQueueFlags, VkSurfaceKHR aSurface);
    void cleanAndClose();
    
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    // Load in the debug messenger extension functions since they are not automatically loaded
    // with Vulkan
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

}

int main() {
    try {
        // -- The setup -- //
        // Create a window using GLFW
        window = createWindow(windowSettings::width, windowSettings::height, windowSettings::name);

        // Create a vulkan instance
        vulkanInstance = createInstance();

        // Set up the debug messenger if in debug mode
#		if !defined(NDEBUG)
        createDebugMessenger();
#		endif

        // Get the surface from the window
        if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get surface from glfw window");
        }

        // Get and set the device to use with vulkan
        deviceSetup();

        // -- The setup -- //


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
    /// This function specifies which extensions and layers can be used with this application and
    /// which debug settings will be used. 
    /// </summary>
    /// <returns>Vulkan instance</returns>
    VkInstance createInstance() {
        // Fill out the details for the application info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
        
        // Check if the extensions are supported by Vulkan
        // Get the number of extensions
        std::uint32_t supportedExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
        // Get the names and versions of the extensions
        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());
        // Put these into an unordered set of strings that can be searched through
        std::unordered_set<std::string> supportedExtensionNames;
        for (auto &extension : supportedExtensions) {
            supportedExtensionNames.insert(extension.extensionName);
        }
        // Get the extensions required for GLFW
        std::uint32_t extensionCount = 0;
        char const** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
        // Check if the extensions are available before enabling them
        std::vector<char const*> extensionsToEnable;
        for (std::uint32_t i = 0; i < extensionCount; i++) {
            if (supportedExtensionNames.count(extensionNames[i]) == 0) {
                throw std::runtime_error("Extension " + std::string(extensionNames[i]) + " is not supported");
            }
            // Add it to the enabled extensions
            extensionsToEnable.emplace_back(extensionNames[i]);
        }

        // Check if validation layers can be enabled
        // Check if the extensions are supported by Vulkan
        // Get the number of extensions
        std::uint32_t supportedLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&supportedLayerCount, nullptr);
        // Get the names and versions of the extensions
        std::vector<VkLayerProperties> supportedLayers(supportedLayerCount);
        vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers.data());
        // Put these into an unordered set of strings that can be searched through
        std::unordered_set<std::string> supportedLayerNames;
        for (auto& layer : supportedLayers) {
            supportedLayerNames.insert(layer.layerName);
        }
        std::vector<char const*> layersToEnable;

        // Fill out the details for the instance info
        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;

        // Check if debug mode is on and should additional layers be enabled
        bool enableDebugUtils = false;
#		if !defined(NDEBUG)
        if (supportedLayerNames.count("VK_LAYER_KHRONOS_validation"))
        {
            layersToEnable.emplace_back("VK_LAYER_KHRONOS_validation");
        }

        if (supportedExtensionNames.count("VK_EXT_debug_utils"))
        {
            enableDebugUtils = true;
            extensionsToEnable.emplace_back("VK_EXT_debug_utils");
        }

        // Prepare the debugger
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = debugCallback;
        debugInfo.pUserData = nullptr;

        debugInfo.pNext = instanceInfo.pNext;
        instanceInfo.pNext = &debugInfo;

#		endif

        // Add more details into the instance info
        instanceInfo.enabledExtensionCount = extensionsToEnable.size();
        instanceInfo.ppEnabledExtensionNames = extensionsToEnable.data();
        instanceInfo.enabledLayerCount = layersToEnable.size();
        instanceInfo.ppEnabledLayerNames = layersToEnable.data();

        VkInstance instance;
        if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Instance creation failed.");
        }

        return instance;

    }

    /// <summary>
    /// Creates the debug messenger
    /// </summary>
    void createDebugMessenger() {
        // Prepare the debug info
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = debugCallback;
        debugInfo.pUserData = nullptr;

        // Before returning create the debug messenger
        if (CreateDebugUtilsMessengerEXT(vulkanInstance, &debugInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Debug messenger creation failed.");
        }
    }

    /// <summary>
    /// Information needed to set up the debug messenger call back
    /// </summary>
    /// <param name="messageSeverity"></param>
    /// <param name="messageType"></param>
    /// <param name="pCallbackData"></param>
    /// <param name="pUserData"></param>
    /// <returns></returns>
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    /// <summary>
    /// General function to set up the physical and logical devices
    /// </summary>
    void deviceSetup() {
        // Select appropriate Vulkan device and output the name of it
        VkPhysicalDevice device;
        device = select_device();
        if (VK_NULL_HANDLE == device) {
            throw std::runtime_error("No suitable physical device found!");
        }
        // Output the name and vulkan version of the device
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        std::fprintf(stderr, "Selected device: %s (%d.%d.%d)\n", props.deviceName, VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion), VK_API_VERSION_PATCH(props.apiVersion));

        // Create a logical device with the required extensions enabled
        std::vector<char const*> enabledDevExensions;
        enabledDevExensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // TODO: Get the graphics queues

        
    }

    /// <summary>
    /// Selects the best physical device available
    /// </summary>
    /// <returns>Best physical device</returns>
    VkPhysicalDevice select_device(){
        // Find all available devices
        std::uint32_t numDevices = 0;
        vkEnumeratePhysicalDevices(vulkanInstance, &numDevices, nullptr);
        std::vector<VkPhysicalDevice> devices(numDevices, VK_NULL_HANDLE);
        if (auto const res = vkEnumeratePhysicalDevices(vulkanInstance, &numDevices, devices.data()); VK_SUCCESS != res)
        {
            throw std::runtime_error("Unable to find any physical devices");
        }

        // Score all available devices to pick the most suitable
        float bestScore = -1.f;
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        for (VkPhysicalDevice device : devices)
        {
            // Check that the device will work before scoring
            // Get the current device's properties
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            
            // Check the version is greater than Vulkan 1.1
            unsigned int major = VK_API_VERSION_MAJOR(properties.apiVersion);
            unsigned int minor = VK_API_VERSION_MINOR(properties.apiVersion);
            if (major < 1 || (major == 1 && minor < 2)) continue;

            // Get the available extensions
            std::uint32_t deviceExtensionCount = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
            std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
            if (vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, deviceExtensions.data()) != VK_SUCCESS) {
                throw std::runtime_error("Unable to find the device extension properties.");
            }
            std::unordered_set<std::string> deviceExtensionNames;
            for (auto const& ext : deviceExtensions) {
                deviceExtensionNames.insert(ext.extensionName);
            }

            // Check that VK_KHR_swapchain is supported
            if (!deviceExtensionNames.count(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;

            // Check that it can present to the surface
            if (!findQueueFamily(device, VK_QUEUE_GRAPHICS_BIT, surface)) continue;

            // Prioritise a separate GPU to an integrated one
            float score = 0;
            if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == properties.deviceType) score = 500.f;
            else if (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == properties.deviceType) score = 100.f;

            if (score > bestScore)
            {
                bestScore = score;
                bestDevice = device;
            }
        }

        return bestDevice;
    }

    /// <summary>
    /// Finds a graphics queue that supports the given flags
    /// </summary>
    /// <param name="aPhysicalDev">The physical device</param>
    /// <param name="aQueueFlags">The queue flags</param>
    /// <param name="aSurface">The surface of the window</param>
    /// <returns>True if the device supports it and false otherwise</returns>
    bool findQueueFamily(VkPhysicalDevice aPhysicalDev, VkQueueFlags aQueueFlags, VkSurfaceKHR aSurface){
        //Find queue family with the specified queue flags that can present to the surface
        std::uint32_t numQueues = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(aPhysicalDev, &numQueues, nullptr);
        std::vector<VkQueueFamilyProperties> families(numQueues);
        vkGetPhysicalDeviceQueueFamilyProperties(aPhysicalDev, &numQueues, families.data());

        for (std::uint32_t i = 0; i < numQueues; ++i) {
            auto const& family = families[i];
            if (aQueueFlags == (aQueueFlags & family.queueFlags)) {
                if (VK_NULL_HANDLE == aSurface) {
                    return true;
                }
                VkBool32 supported = VK_FALSE;
                auto const res = vkGetPhysicalDeviceSurfaceSupportKHR(aPhysicalDev, i, aSurface, &supported);
                if (VK_SUCCESS == res && supported) {
                    return true;
                }
            }
        }
        return false;
    }

    /// <summary>
    /// Cleans up and destroys all used memory structures before terminating the application
    /// </summary>
    void cleanAndClose() {
#       if !defined(NDEBUG)
        DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
#		endif

        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);

        vkDestroyInstance(vulkanInstance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

}