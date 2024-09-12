#include "setup.hpp"

#include <cassert>
#include <algorithm>

namespace {
    /// <summary>
    /// GLFW window creation settings
    /// </summary>
    namespace windowSettings {
        const uint32_t width = 800;
        const uint32_t height = 600;
        const char* name = "Vulkan Renderer";
    }

    // Declaration of functions to be implemented
    GLFWwindow* createWindow(uint32_t width, uint32_t height, const char* name);
    VkInstance createInstance();
    VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance aInstance);
    void deviceSetup(app::AppContext* aApp);
    VkPhysicalDevice selectPhysicalDevice(VkInstance aInstance, VkSurfaceKHR aSurface);
    VkDevice createLogicalDevice(VkPhysicalDevice aPhysicalDev, std::vector<std::uint32_t>& aQueueIndices, std::vector<char const*>& aExtensions);
    std::optional<std::uint32_t> findQueueFamily(VkPhysicalDevice aPhysicalDev, VkQueueFlags aQueueFlags, VkSurfaceKHR aSurface);
    void swapchainSetup(app::AppContext* aApp);
    void createSwapchainImages(app::AppContext* aApp);

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

namespace app {

	void AppContext::cleanup() {
        // Destroy swapchain image views
        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            vkDestroyImageView(logicalDevice, swapchainImageViews[i], nullptr);
        }

        // Destroy the swapchain
        vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);

        // Destroy the logical device
        vkDestroyDevice(logicalDevice, nullptr);

        // Destroy the debug messenger if defined
#       if !defined(NDEBUG)
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#		endif

        // Destroy the surface of the window
		vkDestroySurfaceKHR(instance, surface, nullptr);

        // Destroy the Vulkan instance
		vkDestroyInstance(instance, nullptr);

        // Destroy the GLFW window
		glfwDestroyWindow(window);

        // End the program
		glfwTerminate();
	}

	AppContext setup() {
        AppContext context;

        // Create a window using GLFW
        context.window = createWindow(windowSettings::width, windowSettings::height, windowSettings::name);

        // Create a vulkan instance
        context.instance = createInstance();

        // Set up the debug messenger if in debug mode
#		if !defined(NDEBUG)
        context.debugMessenger = createDebugMessenger(context.instance);
#		endif

        // Get the surface from the window
        if (glfwCreateWindowSurface(context.instance, context.window, nullptr, &context.surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get surface from glfw window");
        }

        // Get and set the device to use with Vulkan (physical and logical)
        deviceSetup(&context);

        // Set up the swapchain
        swapchainSetup(&context);

        // Get the swapchain image and the image views for each of the 
        // swapchain images created
        createSwapchainImages(&context);      

        return context;
	}
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
        for (auto& extension : supportedExtensions) {
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
    /// <param name="aInstance">The instance of the application</param>
    /// <returns>Debug Messenger</returns>
    VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance aInstance) {
        // Prepare the debug info
        VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = debugCallback;
        debugInfo.pUserData = nullptr;

        VkDebugUtilsMessengerEXT debugMessenger;

        // Before returning create the debug messenger
        if (CreateDebugUtilsMessengerEXT(aInstance, &debugInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Debug messenger creation failed.");
        }

        return debugMessenger;
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
    void deviceSetup(app::AppContext *aApp) {
        // Select appropriate Vulkan device and output the name of it
        VkPhysicalDevice physicalDevice;
        aApp->physicalDevice = selectPhysicalDevice(aApp->instance, aApp->surface);
        if (VK_NULL_HANDLE == aApp->physicalDevice) {
            throw std::runtime_error("No suitable physical device found!");
        }
        // Output the name and vulkan version of the device
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(aApp->physicalDevice, &props);
        std::fprintf(stderr, "Selected device: %s (%d.%d.%d)\n", props.deviceName, VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion), VK_API_VERSION_PATCH(props.apiVersion));


        // Get the extensions required for the logical device
        std::vector<char const*> extensionsToEnable;
        extensionsToEnable.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Get the graphics queue(s) Ideally one graphics queue can do both jobs
        // Store the indices of the graphics queue and the present queue if used.
        if (auto const index = findQueueFamily(aApp->physicalDevice, VK_QUEUE_GRAPHICS_BIT, aApp->surface)) {
            aApp->graphicsFamilyIndex = *index;
            aApp->queueFamilyIndices.emplace_back(*index);
        }
        else {
            auto graphics = findQueueFamily(aApp->physicalDevice, VK_QUEUE_GRAPHICS_BIT, VK_NULL_HANDLE);
            auto present = findQueueFamily(aApp->physicalDevice, 0, aApp->surface);

            assert(graphics && present);

            aApp->graphicsFamilyIndex = *graphics;
            aApp->presentFamilyIndex = *present;

            aApp->queueFamilyIndices.emplace_back(*graphics);
            aApp->queueFamilyIndices.emplace_back(*present);
        }

        // Create the logical device
        aApp->logicalDevice = createLogicalDevice(aApp->physicalDevice, aApp->queueFamilyIndices, extensionsToEnable);

        // Set the queues in the app context
        vkGetDeviceQueue(aApp->logicalDevice, aApp->graphicsFamilyIndex, 0, &aApp->graphicsQueue);
        if (aApp->queueFamilyIndices.size() >= 2) {
            vkGetDeviceQueue(aApp->logicalDevice, aApp->presentFamilyIndex, 0, &aApp->presentQueue);
        }
        else {
            aApp->presentFamilyIndex = aApp->graphicsFamilyIndex;
            aApp->presentQueue = aApp->graphicsQueue;
        }
    }

    /// <summary>
    /// Selects the best physical device available
    /// </summary>
    /// <returns>Best physical device</returns>
    VkPhysicalDevice selectPhysicalDevice(VkInstance aInstance, VkSurfaceKHR aSurface) {
        // Find all available devices
        std::uint32_t numDevices = 0;
        vkEnumeratePhysicalDevices(aInstance, &numDevices, nullptr);
        std::vector<VkPhysicalDevice> devices(numDevices, VK_NULL_HANDLE);
        if (auto const res = vkEnumeratePhysicalDevices(aInstance, &numDevices, devices.data()); VK_SUCCESS != res)
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
            if (major < 1 || (major == 1 && minor < 1)) continue;

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
            if (!findQueueFamily(device, VK_QUEUE_GRAPHICS_BIT, aSurface)) continue;

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
    /// <returns>The set of queue families that can be used</returns>
    std::optional<std::uint32_t> findQueueFamily(VkPhysicalDevice aPhysicalDev, VkQueueFlags aQueueFlags, VkSurfaceKHR aSurface) {
        //Find queue family with the specified queue flags that can present to the surface
        std::uint32_t numQueues = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(aPhysicalDev, &numQueues, nullptr);
        std::vector<VkQueueFamilyProperties> families(numQueues);
        vkGetPhysicalDeviceQueueFamilyProperties(aPhysicalDev, &numQueues, families.data());

        for (std::uint32_t i = 0; i < numQueues; i++) {
            auto const& family = families[i];
            if (aQueueFlags & family.queueFlags) {
                // If no surface specified a valid queue is found
                if (VK_NULL_HANDLE == aSurface) {
                    return i;
                }
                // Check if the queue supports the given surface
                VkBool32 supported = VK_FALSE;
                if ((vkGetPhysicalDeviceSurfaceSupportKHR(aPhysicalDev, i, aSurface, &supported) == VK_SUCCESS) && supported) {
                    return i;
                }
            }
        }
        return {};
    }

    /// <summary>
    /// Creates a logical device
    /// </summary>
    /// <param name="aPhysicalDev">The physical device selected</param>
    /// <param name="aQueueIndices">The indices of the queues to use</param>
    /// <param name="aExtensions">Any extensions to be enabled</param>
    /// <returns>A logical Vulkan device</returns>
    VkDevice createLogicalDevice(VkPhysicalDevice aPhysicalDev, std::vector<std::uint32_t>& aQueueIndices, std::vector<char const*>& aExtensions) {
        // Priority for all queues requested
        float queuePriority = 1.0f;
        
        // Make the info for the set of queues
        std::vector<VkDeviceQueueCreateInfo> queueInfos(aQueueIndices.size());
        for (std::size_t i = 0; i < aQueueIndices.size(); ++i)
        {
            auto& queueInfo = queueInfos[i];
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = aQueueIndices[i];
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &queuePriority;
        }

        // Set of features to give to the logical device (Only apply ones that are needed)
        VkPhysicalDeviceFeatures features{};
        // Get the set of features available using the latest version of device features
        VkPhysicalDeviceFeatures2 availableFeatures{};
        availableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        // Get the available features
        vkGetPhysicalDeviceFeatures2(aPhysicalDev, &availableFeatures);
        // Check for Anisotropic Filtering support - Enable if so
        if (availableFeatures.features.samplerAnisotropy) {
            std::printf("Anisotropic filtering enabled\n");
            features.samplerAnisotropy = VK_TRUE;
        }

        // Set up the device info prior to creation
        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = std::uint32_t(queueInfos.size());
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        deviceInfo.enabledExtensionCount = std::uint32_t(aExtensions.size());
        deviceInfo.ppEnabledExtensionNames = aExtensions.data();
        deviceInfo.pEnabledFeatures = &features;

        VkDevice device = VK_NULL_HANDLE;
        if (vkCreateDevice(aPhysicalDev, &deviceInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a logical device");
        }

        return device;
    }

    /// <summary>
    /// Creates the swapchain for the application
    /// </summary>
    /// <param name="aApp">The application context</param>
    void swapchainSetup(app::AppContext* aApp) {
        // Get the capabilities of the swapchain
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(aApp->physicalDevice, aApp->surface, &capabilities);

        // How many images can be in the swapchain
        // Start with the minimum
        std::uint32_t imageCount = capabilities.minImageCount + 1;
        // Check that there is a maximum number of swapchain images and that we
        // haven't exceeded the maximum amount.
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            // Set to the maximum
            imageCount = capabilities.maxImageCount;
        }

        // Get the swapchain extent
        aApp->swapchainExtent = capabilities.currentExtent;
        // Check that the window can be modified
        if (capabilities.currentExtent.width == std::numeric_limits<std::uint32_t>::max()) {
            int width, height;
            glfwGetFramebufferSize(aApp->window, &width, &height);

            aApp->swapchainExtent.width = std::clamp(std::uint32_t(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            aApp->swapchainExtent.height = std::clamp(std::uint32_t(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        // Get the available surface formats for the swapchain
        std::uint32_t numSurfaceFormats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(aApp->physicalDevice, aApp->surface, &numSurfaceFormats, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(numSurfaceFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(aApp->physicalDevice, aApp->surface, &numSurfaceFormats, surfaceFormats.data());

        // Default to any format in the list before attempting to find the best
        VkSurfaceFormatKHR bestSurfaceFormat = surfaceFormats[0];

        // Select the most desireable surface format
        for (VkSurfaceFormatKHR format : surfaceFormats) {
            // Best option
            if (VK_FORMAT_R8G8B8A8_SRGB == format.format && VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == format.colorSpace) {
                bestSurfaceFormat = format;
                break;
            }
            // Second best option
            if (VK_FORMAT_B8G8R8A8_SRGB == format.format && VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == format.colorSpace) {
                bestSurfaceFormat = format;
                //break;
            }
        }

        aApp->swapchainFormat = bestSurfaceFormat.format;

        // Use the FIFO present mode which is guaranteed to be available
        // To get other options follow a similar format to the one used to 
        // find the best surface format
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        // Create swap chain info
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = aApp->surface;
        swapchainInfo.minImageCount = imageCount;
        swapchainInfo.imageFormat = bestSurfaceFormat.format;
        swapchainInfo.imageColorSpace = bestSurfaceFormat.colorSpace;
        swapchainInfo.imageExtent = aApp->swapchainExtent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentMode;
        swapchainInfo.clipped = VK_TRUE;
        //chainInfo.oldSwapchain = aOldSwapchain;
        if (aApp->queueFamilyIndices.size() <= 1) {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else {
            //Multiple queues may access this
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainInfo.queueFamilyIndexCount = std::uint32_t(aApp->queueFamilyIndices.size());
            swapchainInfo.pQueueFamilyIndices = aApp->queueFamilyIndices.data();
        }

        // Create the swapchain
        aApp->swapchain;
        if (vkCreateSwapchainKHR(aApp->logicalDevice, &swapchainInfo, nullptr, &aApp->swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain.");
        }

        return;
    }

    /// <summary>
    /// Creates the swapchain images for the application's swapchain
    /// </summary>
    /// <param name="aApp">The application context</param>
    void createSwapchainImages(app::AppContext* aApp) {
        // Get swapchain image handles with vkGetSwapchainImagesKHR
        std::uint32_t numImages = 0;
        vkGetSwapchainImagesKHR(aApp->logicalDevice, aApp->swapchain, &numImages, nullptr);
        aApp->swapchainImages.resize(numImages);
        vkGetSwapchainImagesKHR(aApp->logicalDevice, aApp->swapchain, &numImages, aApp->swapchainImages.data());
    
        // For each of the images in the swapchain
        for (size_t i = 0; i < aApp->swapchainImages.size(); i++) {
            // Create a VkImageView
            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = aApp->swapchainImages[i];
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = aApp->swapchainFormat;
            imageViewInfo.components = VkComponentMapping{
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            };
            imageViewInfo.subresourceRange = VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1,
                0, 1
            };

            VkImageView imageView;
            if (vkCreateImageView(aApp->logicalDevice, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("Couldn't make an image view for the swapchain images");
            }

            aApp->swapchainImageViews.emplace_back(imageView);
        }
    
    }
}