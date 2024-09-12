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

    /// <summary>
    /// Creates a render pass for the currently running application
    /// </summary>
    /// <param name="app">The context of the application</param>
    /// <returns>Render pass</returns>
    VkRenderPass createRenderPass(app::AppContext& app);

    /// <summary>
    /// Creates a descriptor set layout to feed into the pipeline
    /// </summary>
    /// <param name="app">The context of the application</param>
    /// <returns>Descriptor set layout</returns>
    VkDescriptorSetLayout createDescriptorSetLayout(app::AppContext& app);
}

int main() {
    try {
        // -- The setup -- //
        app::AppContext application = app::setup();

        // Create the memory allocator
        VmaAllocator allocator = createMemoryAllocator(application);

        // Create the renderpass
        VkRenderPass renderPass = createRenderPass(application);

        // Create the descriptor set layout
        VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(application);

        // Main render loop
        while (!glfwWindowShouldClose(application.window)) {
            glfwPollEvents();
        }

        // Clean up and close the application
        vkDestroyDescriptorSetLayout(application.logicalDevice, descriptorSetLayout, nullptr);
        vkDestroyRenderPass(application.logicalDevice, renderPass, nullptr);
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

    VkRenderPass createRenderPass(app::AppContext& app) {
        // Define the attatchments of the render pass
        // The swapchain attatchment
        VkAttachmentDescription attachments[2]{};
        attachments[0].format = app.swapchainFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // The depth buffer attachment
        attachments[1].format = VK_FORMAT_D32_SFLOAT;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Define the attatchment propeties for a subpass
        VkAttachmentReference colourAttachment{};
        // Colour part of subpass using attatchment 0
        colourAttachment.attachment = 0;
        colourAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // Depth part of subpass using attatchment 1
        VkAttachmentReference depthAttachment{};
        depthAttachment.attachment = 1; // this refers to attachments[1]
        depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Provide a description of the subpass
        VkSubpassDescription subpasses{};
        subpasses.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses.colorAttachmentCount = 1;
        subpasses.pColorAttachments = &colourAttachment;
        subpasses.pDepthStencilAttachment = &depthAttachment;

        // Set the dependencies of each subpass
        VkSubpassDependency subpassDependencies[2]{};
        // For the colour
        subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[0].srcAccessMask = 0;
        subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependencies[0].dstSubpass = 0;
        subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // For the depth
        subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        subpassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependencies[1].dstSubpass = 0;
        subpassDependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        // Combine all the data to create the renderpass info
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpasses;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = subpassDependencies;

        // Create the renderpass
        VkRenderPass renderPass;
        if (vkCreateRenderPass(app.logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create renderpass.");
        }

        return renderPass;
    }

    VkDescriptorSetLayout createDescriptorSetLayout(app::AppContext& app) {
        // Set the bindings for the descriptor
        // These are accessed in the shader as binding = n
        // All data passed into the shaders must have a binding
        int const numberOfBindings = 1;
        VkDescriptorSetLayoutBinding bindings[numberOfBindings]{};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // Set the info of the descriptor
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = numberOfBindings;
        descriptorSetLayoutInfo.pBindings = bindings;

        VkDescriptorSetLayout descriptorSetLayout;
        if (vkCreateDescriptorSetLayout(app.logicalDevice, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }

        return descriptorSetLayout;

    }
}