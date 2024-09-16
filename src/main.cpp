#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <string>
#include <optional>

#include "setup.hpp"
#include "images.hpp"

namespace {

    namespace paths {
        char const* vertexShaderPath = "Shaders/vert.spv";
        char const* fragmentShaderPath = "Shaders/frag.spv";
    }

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

    /// <summary>
    /// Creates a pipeline layout prior to making a pipeline
    /// </summary>
    /// <param name="app">The context of the application</param>
    /// <param name="descriptorSetLayouts">The set of desriptors to apply to the pipeline</param>
    /// <returns>Pipeline Layout</returns>
    VkPipelineLayout createPipelineLayout(app::AppContext& app, std::vector<VkDescriptorSetLayout> descriptorSetLayouts);

    /// <summary>
    /// Reads in and creates a shader module given the path to a shader
    /// </summary>
    /// <param name="app">The context of the application</param>
    /// <param name="shaderPath">File path to a compiled spir-v shader</param>
    /// <returns>Shader module</returns>
    VkShaderModule createShaderModule(app::AppContext& app, char const* shaderPath);

    /// <summary>
    /// Creates a graphics pipeline to set how the rendering should be done
    /// </summary>
    /// <param name="app"> The application context </param>
    /// <param name="pipeLayout">A pipeline layout</param>
    /// <param name="renderPass">The render pass to apply the pipeline to</param>
    /// <param name="vertexShader">The vertex shader to use</param>
    /// <param name="fragmentShader">The fragment shader to use</param>
    /// <returns></returns>
    VkPipeline createPipeline(app::AppContext& app, VkPipelineLayout pipeLayout, 
        VkRenderPass renderPass, VkShaderModule vertexShader, VkShaderModule fragmentShader);


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

        // Create a vector of the descripor sets to use
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        descriptorSetLayouts.emplace_back(descriptorSetLayout);

        // Create a pipeline layout
        VkPipelineLayout pipelineLayout = createPipelineLayout(application, descriptorSetLayouts);

        // Create the shaders
        VkShaderModule vertexShader = createShaderModule(application, paths::vertexShaderPath);
        VkShaderModule fragmentShader = createShaderModule(application, paths::fragmentShaderPath);

        // Create the pipeline
        VkPipeline pipeline = createPipeline(application, pipelineLayout, renderPass, vertexShader, fragmentShader);

        // Create a vkImage and vkImageView to store the colour of the framebuffer
        utility::ImageSet colourImageSet = utility::createBuffer(application, allocator);

        // Main render loop
        while (!glfwWindowShouldClose(application.window)) {
            glfwPollEvents();
        }

        vkDeviceWaitIdle(application.logicalDevice);

        // Clean up and close the application
        vmaDestroyImage(allocator, colourImageSet.image, colourImageSet.allocation);
        vkDestroyImageView(application.logicalDevice, colourImageSet.imageView, nullptr);
        vkDestroyPipeline(application.logicalDevice, pipeline, nullptr);
        vkDestroyShaderModule(application.logicalDevice, vertexShader, nullptr);
        vkDestroyShaderModule(application.logicalDevice, fragmentShader, nullptr);
        vkDestroyPipelineLayout(application.logicalDevice, pipelineLayout, nullptr);
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

    VkPipelineLayout createPipelineLayout(app::AppContext& app, 
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts) {

        // Set the info for the pipeline layout
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = descriptorSetLayouts.size();
        layoutInfo.pSetLayouts = descriptorSetLayouts.data();

        VkPipelineLayout pipelineLayout;
        if (vkCreatePipelineLayout(app.logicalDevice, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout.");
        }

        return pipelineLayout;
    }

    VkShaderModule createShaderModule(app::AppContext& app, char const* shaderPath) {

        // Read in the shader
        std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + std::string(shaderPath));
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        // Get the info for the shader module
        VkShaderModuleCreateInfo shaderInfo{};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = buffer.size();
        shaderInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
        
        // Create the shader module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(app.logicalDevice, &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module");
        }

        return shaderModule;

    }

    VkPipeline createPipeline(app::AppContext& app, VkPipelineLayout pipeLayout,
        VkRenderPass renderPass, VkShaderModule vertexShader, VkShaderModule fragmentShader){

        // Detail the shader stages of the pipeline
        VkPipelineShaderStageCreateInfo shaderStages[2]{};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertexShader;
        shaderStages[0].pName = "main";
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragmentShader;
        shaderStages[1].pName = "main";

        // Inputs into the vertex shader
        VkVertexInputBindingDescription vertexInputs[2]{};
        //Positions 3 floats
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(float) * 3;
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        //Textures 2 floats
        vertexInputs[1].binding = 1;
        vertexInputs[1].stride = sizeof(float) * 2;
        vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Attributes of the above inputs
        VkVertexInputAttributeDescription vertexAttributes[2]{};
        // Positions
        vertexAttributes[0].binding = 0;
        vertexAttributes[0].location = 0;
        vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[0].offset = 0;
        // Textures
        vertexAttributes[1].binding = 1;
        vertexAttributes[1].location = 1;
        vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributes[1].offset = 0;

        // Vertex shader info using the above descriptions
        VkPipelineVertexInputStateCreateInfo vertexInfo{};
        vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInfo.vertexBindingDescriptionCount = 2;
        vertexInfo.pVertexBindingDescriptions = vertexInputs;
        vertexInfo.vertexAttributeDescriptionCount = 2;
        vertexInfo.pVertexAttributeDescriptions = vertexAttributes;

        // Details about the topology of the input vertices
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo.primitiveRestartEnable = VK_FALSE;

        // Set the viewport to have to be the full screen
        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = float(app.swapchainExtent.width);
        viewport.height = float(app.swapchainExtent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset = VkOffset2D{ 0, 0 };
        scissor.extent = VkExtent2D{ app.swapchainExtent.width, app.swapchainExtent.height };

        // Assign the information about the viewport
        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

        // Detail the rasterisation settings
        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.depthClampEnable = VK_FALSE;
        rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationInfo.depthBiasEnable = VK_FALSE;
        rasterizationInfo.lineWidth = 1.f;

        // Multisampling rules
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Colour attatchment blend state
        VkPipelineColorBlendAttachmentState colourBlendStates[1]{};
        colourBlendStates[0].blendEnable = VK_FALSE;
        colourBlendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        // Info on the colour blends
        VkPipelineColorBlendStateCreateInfo colourBlendInfo{};
        colourBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colourBlendInfo.logicOpEnable = VK_FALSE;
        colourBlendInfo.attachmentCount = 1;
        colourBlendInfo.pAttachments = colourBlendStates;

        // Info on the depth attatchment
        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // Set the info about the entire pipeline using the above details set
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.stageCount = 2;
        pipeInfo.pStages = shaderStages;
        pipeInfo.pVertexInputState = &vertexInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterizationInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo; // depth buffers
        pipeInfo.pColorBlendState = &colourBlendInfo;
        pipeInfo.layout = pipeLayout;
        pipeInfo.renderPass = renderPass;
        pipeInfo.subpass = 0;

        VkPipeline pipeline;
        if (vkCreateGraphicsPipelines(app.logicalDevice, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline.");
        }

        return pipeline;
    }

    

}