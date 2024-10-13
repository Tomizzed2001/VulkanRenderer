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
#include <cassert>

#include "glm.hpp"
#include <gtx/transform.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/string_cast.hpp>

#include "setup.hpp"
#include "images.hpp"
#include "utility.hpp"
#include "model.hpp"
#include "FBXFileLoader.hpp"

namespace {

    struct CameraInfo {
        glm::vec3 position = glm::vec3(0);

        glm::vec2 mousePosition;

        glm::mat4 worldCameraMatrix = glm::mat4(1);

        bool isLooking = false;
    };

    struct WorldView {
        glm::mat4 projectionCameraMatrix;
        glm::vec3 cameraPosition;
    };

    struct LightingData {
        //glm::mat4 lightDirectionMatrix;
        alignas(16) glm::vec3 lightPosition;
        alignas(16) glm::vec3 lightColour;
    };

    namespace paths {
        char const* vertexShaderPath = "Shaders/vert.spv";
        char const* fragmentShaderPath = "Shaders/frag.spv";
        char const* alphaFragmentShaderPath = "Shaders/alphaFrag.spv";
        char const* textureFillPath = "EmptyTexture.png";
    }

    /// <summary>
    /// Configure the key callback for the glfw window
    /// </summary>
    /// <param name="window">Glfw window</param>
    /// <param name="key"></param>
    /// <param name="scancode"></param>
    /// <param name="action"></param>
    /// <param name="mods"></param>
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    /// <summary>
    /// Configure the mouse button callback for the glfw window
    /// </summary>
    /// <param name="window">Glfw window</param>
    /// <param name="button">Mouse button</param>
    /// <param name="action">The state of the mouse button</param>
    /// <param name="mods"></param>
    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    /// <summary>
    /// Configure the mouse callback for thee glfw window
    /// </summary>
    /// <param name="window">Glfw window</param>
    /// <param name="mouseX">Direction in x that mouse is looking</param>
    /// <param name="mouseY">Direction in y that mouse is looking</param>
    void mouseCallback(GLFWwindow* window, double mouseX, double mouseY);

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
    VkDescriptorSetLayout createWorldDescriptorSetLayout(app::AppContext& app);

    /// <summary>
    /// Creates a descriptor set layout to feed into the pipeline
    /// </summary>
    /// <param name="app">The context of the application</param>
    /// <returns>Descriptor set layout</returns>
    VkDescriptorSetLayout createTextureDescriptorSetLayout(app::AppContext& app);

    /// <summary>
    /// Creates a descriptor set layout to feed into the pipeline
    /// </summary>
    /// <param name="app">The context of the application</param>
    /// <returns>Descriptor set layout</returns>
    VkDescriptorSetLayout createLightDescriptorSetLayout(app::AppContext& app);

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
    /// <param name="isAlpha">Is the pipeline using alpha masking / blending</param>
    /// <returns></returns>
    VkPipeline createPipeline(app::AppContext& app, VkPipelineLayout pipeLayout, 
        VkRenderPass renderPass, VkShaderModule vertexShader, VkShaderModule fragmentShader,
        bool isAlpha = false);

    /// <summary>
    /// Creates a frame buffer to store the output of a render pass
    /// </summary>
    /// <param name="app">The application context</param>
    /// <param name="renderPass">The render pass that the framebuffer will be used with</param>
    /// <param name="buffers">The image view attatchements of the framebuffer</param>
    /// <returns></returns>
    VkFramebuffer createFramebuffer(app::AppContext& app, VkRenderPass renderPass, std::vector<VkImageView>& buffers);

    /// <summary>
    /// Creates a texture sampler
    /// </summary>
    /// <param name="app">Application context</param>
    /// <returns>Vulkan sampler</returns>
    VkSampler createTextureSampler(app::AppContext& app);

    /// <summary>
    /// Creates a descriptor pool
    /// </summary>
    /// <param name="app">Application context</param>
    /// <returns>A Vulkan descriptor pool</returns>
    VkDescriptorPool createDescriptorPool(app::AppContext& app);

    /// <summary>
    /// Creates a descriptor set
    /// </summary>
    /// <param name="app">Application context</param>
    /// <param name="pool">Descriptor pool</param>
    /// <param name="layout">Descriptor set layout</param>
    /// <returns>Descriptor set (uninitialized)</returns>
    VkDescriptorSet createDescriptorSet(app::AppContext& app, VkDescriptorPool pool, VkDescriptorSetLayout layout);
    
    /// <summary>
    /// Creates a descriptor set for a buffer and initialises it
    /// </summary>
    /// <param name="app">Application context</param>
    /// <param name="pool">Descriptor pool</param>
    /// <param name="layout">Descriptor set layout</param>
    /// <param name="buffer">The buffer storing the data</param>
    /// <param name="descriptorType">The type of descriptor it will be</param>
    /// <returns></returns>
    VkDescriptorSet createBufferDescriptorSet(app::AppContext& app, VkDescriptorPool pool, VkDescriptorSetLayout layout, 
        VkBuffer& buffer, VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    /// <summary>
    /// Creates a descriptor set to contain multiple images and initialises it
    /// </summary>
    /// <param name="app">Application context</param>
    /// <param name="pool">Descriptor pool</param>
    /// <param name="layout">Descriptor set layout</param>
    /// <param name="diffuseImages">The colour / diffuse images to go into the descriptor</param>
    /// <param name="specularImages">The specular images to go into the descriptor</param>
    /// <param name="normalMapImages">The normal map images to go into the descriptor</param>
    /// <param name="sampler">The sampler to be used</param>
    /// <returns>Image descriptor set</returns>
    VkDescriptorSet createBindlessImageDescriptorSet(app::AppContext& app, VkDescriptorPool pool, VkDescriptorSetLayout layout,
        std::vector<utility::ImageSet>& diffuseImages,
        std::vector<utility::ImageSet>& specularImages,
        std::vector<utility::ImageSet>& normalMapImages,
        VkSampler& sampler
    );

    /// <summary>
    /// Updates the world view matrices
    /// </summary>
    /// <param name="worldUniform">The uniform to be updated</param>
    /// <param name="screenAspect">The aspect value of the swapchain</param>
    /// <param name="cameraInfo">The camera info struct defining location and view</param>
    void updateWorldUniforms(WorldView& worldUniform, float screenAspect, CameraInfo& cameraInfo);

    /// <summary>
    /// Updates the lighting uniforms
    /// </summary>
    /// <param name="app">The application context</param>
    /// <param name="lightingBuffer">The lighting buffer</param>
    /// <param name="lightData">The data holding the lighting info</param>
    /// <param name="commandPool">The command pool</param>
    void updateLightingUniforms(app::AppContext& app, VkBuffer lightingBuffer, LightingData lightData,
        VkCommandPool commandPool);


    /// <summary>
    /// Recreates the swapchain
    /// </summary>
    /// <param name="app">Application context</param>
    void recreateSwapchain(app::AppContext& app);

    /// <summary>
    /// Records the rendering information and sets up the draw calls
    /// </summary>
    /// <param name="commandBuffer">The command buffer to record to</param>
    /// <param name="worldUniformBuffer">The world view uniform buffer</param>
    /// <param name="worldUniform">The world view struct</param>
    /// <param name="renderPass">The render pass</param>
    /// <param name="frameBuffer">The framebuffer to use</param>
    /// <param name="renderArea">The render area of the frame buffer</param>
    /// <param name="pipeline">Render pipeline</param>
    /// <param name="pipelineLayout">Render pipeline layout</param>
    /// <param name="worldDescriptorSet">Descriptor set describing the world view uniform</param>
    /// <param name="textureDescriptorSet">Descriptor set describing the textures</param>
    /// <param name="vertexBuffers">The vertex buffer</param>
    /// <param name="vertexOffsets">The vertex offsets</param>
    /// <param name="numVertices">The number of vertices</param>
    void recordCommands(
        VkCommandBuffer commandBuffer,                              // Command buffer
        VkBuffer worldUniformBuffer, WorldView worldUniform,        // World Uniform
        VkRenderPass renderPass, VkFramebuffer frameBuffer,         // Render pass
        VkRect2D renderArea,
        VkPipeline pipeline, VkPipeline alphaPipeline,              // Pipelines
        VkPipelineLayout pipelineLayout,                            // Pipelines
        VkDescriptorSet worldDescriptorSet,                         // World descriptors
        VkDescriptorSet textureDescriptorSet,                       // Texture descriptors
        VkDescriptorSet lightingDescriptorSet,                      // Lighting descriptors
        std::vector<model::Mesh>& meshes,                           // Mesh data
        std::vector<model::Mesh>& alphaMeshes,                      // Mesh data
        std::vector<VkDeviceSize>& vertexOffsets,                   // Per vertex data
        std::vector<fbx::Material>& materials                       // Material data
    );
    
    /// <summary>
    /// Submits a command buffer to the graphics queue
    /// </summary>
    /// <param name="app">Application context</param>
    /// <param name="commandBuffer">Recorded Command buffer</param>
    /// <param name="wait">Wait semaphore</param>
    /// <param name="signal">Signal semaphore</param>
    /// <param name="fence">Fence to wait for</param>
    void submitCommands(app::AppContext app, VkCommandBuffer commandBuffer, VkSemaphore wait, 
        VkSemaphore signal, VkFence fence);

    
    
    /// <summary>
    /// Presents the rendering to screen
    /// </summary>
    /// <param name="app">Application context</param>
    /// <param name="finishedSemaphore">Signal semaphore from submission</param>
    /// <param name="swapchainIndex">Swapchain image index</param>
    /// <returns>True if swapchain needs resizing</returns>
    bool presentToScreen(app::AppContext app, VkSemaphore finishedSemaphore, std::uint32_t swapchainIndex);

}

int main() {
    try {
        // -- The setup -- //
        app::AppContext application = app::setup();

        // Set up the player camera state
        CameraInfo playerCamera;
        playerCamera.position = glm::vec3(0.0, 0.3, 1.0);
        playerCamera.worldCameraMatrix = playerCamera.worldCameraMatrix * glm::translate(playerCamera.position);

        // Set up the GLFW inputs
        // Sets player camera as the user of the window
        glfwSetWindowUserPointer(application.window, &playerCamera);

        // Sets up the key call back function
        glfwSetKeyCallback(application.window, keyCallback);

        // Sets up the mouse button call back function
        glfwSetMouseButtonCallback(application.window, &mouseButtonCallback);

        // Sets up the mouse call back function
        glfwSetCursorPosCallback(application.window, &mouseCallback);
        
        // -- The setup -- //
        
        // Create the memory allocator
        VmaAllocator allocator = createMemoryAllocator(application);

        // Create the renderpass
        VkRenderPass renderPass = createRenderPass(application);

        // Create the descriptor set layouts
        // World descriptor set layout contains the world view matrices
        VkDescriptorSetLayout worldDescriptorSetLayout = createWorldDescriptorSetLayout(application);
        // Texture descriptor set layout contains all the material textures
        VkDescriptorSetLayout textureDescriptorSetLayout = createTextureDescriptorSetLayout(application);
        // Lighting descriptor set layout contains all the lighting data
        VkDescriptorSetLayout lightDescriptorSetLayout = createLightDescriptorSetLayout(application);

        // Create a vector of the descriptor sets to use
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        descriptorSetLayouts.emplace_back(worldDescriptorSetLayout);
        descriptorSetLayouts.emplace_back(textureDescriptorSetLayout);
        descriptorSetLayouts.emplace_back(lightDescriptorSetLayout);

        // Create a pipeline layout
        VkPipelineLayout pipelineLayout = createPipelineLayout(application, descriptorSetLayouts);

        // Create the shaders
        VkShaderModule vertexShader = createShaderModule(application, paths::vertexShaderPath);
        VkShaderModule fragmentShader = createShaderModule(application, paths::fragmentShaderPath);
        VkShaderModule alphaFragmentShader = createShaderModule(application, paths::alphaFragmentShaderPath);

        // Create the pipeline
        VkPipeline pipeline = createPipeline(application, pipelineLayout, renderPass, vertexShader, fragmentShader);
        VkPipeline alphaPipeline = createPipeline(application, pipelineLayout, renderPass, vertexShader, alphaFragmentShader, true);

        // Create a vkImage and vkImageView to store the depth buffer
        utility::ImageSet depthBuffer = utility::createImageSet(application, allocator,
            VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        // Create the swapchain framebuffers (one for each of the image views)
        std::vector<VkFramebuffer> swapchainFramebuffers;
        for (size_t i = 0; i < application.swapchainImageViews.size(); i++) {
            // Get the attatchments
            std::vector<VkImageView> swapchainAttatchments;
            swapchainAttatchments.emplace_back(application.swapchainImageViews[i]);
            swapchainAttatchments.emplace_back(depthBuffer.imageView);
            
            // Create the framebuffer
            swapchainFramebuffers.emplace_back(createFramebuffer(application, renderPass, swapchainAttatchments));
        }

        // Create the command pool
        VkCommandPool commandPool = utility::createCommandPool(application, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        // Load an FBX model
        //fbx::Scene fbxScene = fbx::loadFBXFile("Bistro/BistroExterior.fbx");
        fbx::Scene fbxScene = fbx::loadFBXFile("SunTemple/SunTemple.fbx");

        // Load all textures from the fbx model
        // Load colour (diffuse) textures in
        std::vector<utility::ImageSet> colourTextures;
        for (fbx::Texture texture : fbxScene.diffuseTextures) {
            // If texture is not empty
            if (texture.isEmpty) {
                colourTextures.emplace_back(utility::createPNGTextureImageSet(application, paths::textureFillPath, allocator, commandPool));
            }
            // If texture is a dds texture
            else if (texture.filePath.ends_with(".dds")) {
                colourTextures.emplace_back(utility::createDDSTextureImageSet(application, texture.filePath.c_str(), allocator, commandPool));
            }
            else if (texture.filePath.ends_with(".png") || texture.filePath.ends_with(".jpg")) {
                colourTextures.emplace_back(utility::createPNGTextureImageSet(application, texture.filePath.c_str(), allocator, commandPool));
            }
        }
        // Load specular textures in
        std::vector<utility::ImageSet> specularTextures;
        for (fbx::Texture texture : fbxScene.specularTextures) {
            // If texture is not empty
            if (texture.isEmpty) {
                specularTextures.emplace_back(utility::createPNGTextureImageSet(application, paths::textureFillPath, allocator, commandPool));
            }
            // If texture is a dds texture
            else if (texture.filePath.ends_with(".dds")) {
                specularTextures.emplace_back(utility::createDDSTextureImageSet(application, texture.filePath.c_str(), allocator, commandPool));
            }
            else if (texture.filePath.ends_with(".png") || texture.filePath.ends_with(".jpg")) {
                specularTextures.emplace_back(utility::createPNGTextureImageSet(application, texture.filePath.c_str(), allocator, commandPool));
            }
        }
        // Load normal map textures in
        std::vector<utility::ImageSet> normalTextures;
        for (fbx::Texture texture : fbxScene.normalTextures) {
            // If texture is not empty
            if (texture.isEmpty) {
                normalTextures.emplace_back(utility::createPNGTextureImageSet(application, paths::textureFillPath, allocator, commandPool));
            }
            // If texture is a dds texture
            else if (texture.filePath.ends_with(".dds")) {
                normalTextures.emplace_back(utility::createDDSTextureImageSet(application, texture.filePath.c_str(), allocator, commandPool));
            }
            else if (texture.filePath.ends_with(".png") || texture.filePath.ends_with(".jpg")) {
                normalTextures.emplace_back(utility::createPNGTextureImageSet(application, texture.filePath.c_str(), allocator, commandPool));
            }
        }

        // Load all meshes from the fbx model (Separate the meshes that use alpha textures)
        std::vector<model::Mesh> meshes;
        std::vector<model::Mesh> alphaMeshes;
        for (fbx::Mesh mesh : fbxScene.meshes) {
            // Check all materials of the mesh to see if they require alpha testing
            bool alpha = false;
            for (int texID = 0; texID < mesh.materials.size(); texID++) {
                if (colourTextures[mesh.materials[texID]].isAlpha) {
                    alpha = true;
                    break;
                }
            }
            if (alpha) {
                alphaMeshes.emplace_back(model::createMesh(application, allocator, commandPool,
                    mesh.vertexPositions, mesh.vertexTextureCoords, mesh.vertexNormals, mesh.vertexTangents, mesh.vertexMaterialIDs, mesh.vertexIndices));
            }
            else {
                meshes.emplace_back(model::createMesh(application, allocator, commandPool,
                    mesh.vertexPositions, mesh.vertexTextureCoords, mesh.vertexNormals, mesh.vertexTangents, mesh.vertexMaterialIDs, mesh.vertexIndices));
            }
        }

        // TODO: Load all the lighting from the fbx model
        // (Use a dummy set of values for now)
        LightingData light;
        light.lightColour = glm::vec3(1, 0, 1);
        light.lightPosition = glm::vec3(5,5,5);

        std::cout << "Num meshes: " << fbxScene.meshes.size() << std::endl;
        std::cout << "Num materials: " << fbxScene.materials.size() << std::endl;
        std::cout << "Num colour textures: " << fbxScene.diffuseTextures.size() << std::endl;
        std::cout << "Num normal textures: " << fbxScene.normalTextures.size() << std::endl;
        std::cout << "Num specular textures: " << fbxScene.specularTextures.size() << std::endl;
        std::cout << "Num emissive textures: " << fbxScene.emissiveTextures.size() << std::endl;


        // Create a texture sampler
        VkSampler sampler = createTextureSampler(application);

        // Create descriptor pool
        VkDescriptorPool descriptorPool = createDescriptorPool(application);

        // Create the world uniform buffer
        utility::BufferSet worldUniformBuffer = utility::createBuffer(allocator, sizeof(WorldView),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

        // Create and initialise the world descriptor set
        VkDescriptorSet worldDescriptorSet = createBufferDescriptorSet(application, descriptorPool,
            worldDescriptorSetLayout, worldUniformBuffer.buffer);

        // Create and initialise the texture descriptor sets
        VkDescriptorSet bindlessTextureDescriptorSet = createBindlessImageDescriptorSet(application, descriptorPool, 
            textureDescriptorSetLayout, colourTextures, specularTextures, normalTextures, sampler);

        // Create the lighting uniform buffer
        utility::BufferSet lightingUniformBuffer = utility::createBuffer(allocator, sizeof(LightingData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

        // Set the data for the lighting buffer
        updateLightingUniforms(application, lightingUniformBuffer.buffer, light, commandPool);

        // Create and initialise the lighting descriptor sets
        VkDescriptorSet lightDescriptorSet = createBufferDescriptorSet(application, descriptorPool,
            lightDescriptorSetLayout, lightingUniformBuffer.buffer);

        // Create the command buffers - one for each of the swapchain framebuffers
        std::vector<VkCommandBuffer> commandBuffers;
        for (size_t i = 0; i < swapchainFramebuffers.size(); i++) {
            commandBuffers.emplace_back(utility::createCommandBuffer(application, commandPool));
        }

        // Create fences for each of the swapchain framebuffers / command buffers
        std::vector<VkFence> fences;
        for (size_t i = 0; i < swapchainFramebuffers.size(); i++) {
            fences.emplace_back(utility::createFence(application, VK_FENCE_CREATE_SIGNALED_BIT));
        }

        // Create semaphores
        VkSemaphore imageIsReady = utility::createSemaphore(application, 0);
        VkSemaphore renderHasFinished = utility::createSemaphore(application, 0);

        bool resizeWindow = false;

        // Main render loop
        while (!glfwWindowShouldClose(application.window)) {
            // Check for input events
            glfwPollEvents();

            // Has the window been resized and if so resize the swapchain
            if (resizeWindow) {
                std::cout << "Changing" << std::endl;
                // Remember the old format and size of the swapchain
                VkFormat oldFormat = application.swapchainFormat;
                VkExtent2D oldExtent = application.swapchainExtent;

                // Remake the swapchain
                recreateSwapchain(application);

                // If format has changed the render pass needs remaking before framebuffers
                if (application.swapchainFormat != oldFormat) {
                    // Clean up old render pass
                    vkDestroyRenderPass(application.logicalDevice, renderPass, nullptr);
                    // Remake the render pass
                    renderPass = createRenderPass(application);
                }

                // Destroy the old framebuffers
                for (size_t i = 0; i < swapchainFramebuffers.size(); i++) {
                    vkDestroyFramebuffer(application.logicalDevice, swapchainFramebuffers[i], nullptr);
                }
                swapchainFramebuffers.clear();

                // Remake the depth buffer if size has changed
                if (application.swapchainExtent.height != oldExtent.height ||
                    application.swapchainExtent.width != oldExtent.width) {
                    // Clean up old depth buffer
                    vmaDestroyImage(allocator, depthBuffer.image, depthBuffer.allocation);
                    vkDestroyImageView(application.logicalDevice, depthBuffer.imageView, nullptr);
                    // Remake
                    depthBuffer = utility::createImageSet(application, allocator, VK_FORMAT_D32_SFLOAT,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
                }
                
                // Remake the framebuffers
                for (size_t i = 0; i < application.swapchainImageViews.size(); i++) {
                    // Get the attatchments
                    std::vector<VkImageView> swapchainAttatchments;
                    swapchainAttatchments.emplace_back(application.swapchainImageViews[i]);
                    swapchainAttatchments.emplace_back(depthBuffer.imageView);

                    // Create the framebuffer
                    swapchainFramebuffers.emplace_back(createFramebuffer(application, renderPass, swapchainAttatchments));
                }

                // If size has changed, the pipelines need remaking
                if (application.swapchainExtent.height != oldExtent.height || 
                    application.swapchainExtent.width != oldExtent.width) {
                    // Clean up old pipeline
                    vkDestroyPipeline(application.logicalDevice, pipeline, nullptr);
                    vkDestroyPipeline(application.logicalDevice, alphaPipeline, nullptr);
                    // Remake pipeline
                    pipeline = createPipeline(application, pipelineLayout, renderPass,
                        vertexShader, fragmentShader);
                    alphaPipeline = createPipeline(application, pipelineLayout, renderPass,
                        vertexShader, alphaFragmentShader, true);
                }

                // Reset the resized window bool
                resizeWindow = false;

                // Skip to the next frame
                continue;

            }

            // Get the next image in the swapchain to use
            std::uint32_t nextImageIndex = 0;
            auto const nextImageSuccess = vkAcquireNextImageKHR(application.logicalDevice, application.swapchain,
                std::numeric_limits<std::uint64_t>::max(), imageIsReady, VK_NULL_HANDLE, 
                &nextImageIndex);

            // Check to see if a deadlock has occured
            if (VK_SUBOPTIMAL_KHR == nextImageSuccess || VK_ERROR_OUT_OF_DATE_KHR == nextImageSuccess) {
                resizeWindow = true;
                continue;
            }
            if (nextImageSuccess != VK_SUCCESS) {
                throw std::runtime_error("Failed to get next swapchain image");
            }

            // Wait for the command buffer
            if (vkWaitForFences(application.logicalDevice, 1, &fences[nextImageIndex], VK_TRUE, std::numeric_limits<std::uint64_t>::max()) != VK_SUCCESS) {
                throw std::runtime_error("Fence buffer timed out.");
            }
            // Reset the fence for the next iteration
            if (vkResetFences(application.logicalDevice, 1, &fences[nextImageIndex]) != VK_SUCCESS) {
                throw std::runtime_error("Fence buffer couldn't be reset.");
            }
            
            // Update the world view uniform
            WorldView worldViewUniform;
            float screenAspect = float(application.swapchainExtent.width) / float(application.swapchainExtent.height);
            updateWorldUniforms(worldViewUniform, screenAspect, playerCamera);

            // Get the render area
            VkRect2D renderArea;
            renderArea.extent = application.swapchainExtent;
            renderArea.offset = VkOffset2D{ 0,0 };

            // Get the set of offsets
            std::vector<VkDeviceSize> meshOffsets = {0, 0};


            // Record commands
            recordCommands(
                commandBuffers[nextImageIndex],
                worldUniformBuffer.buffer,
                worldViewUniform,
                renderPass,
                swapchainFramebuffers[nextImageIndex],
                renderArea,
                pipeline,
                alphaPipeline,
                pipelineLayout,
                worldDescriptorSet,
                bindlessTextureDescriptorSet,
                lightDescriptorSet,
                meshes,
                alphaMeshes,
                meshOffsets,
                fbxScene.materials
            );

            // Submit commands
            submitCommands(application, commandBuffers[nextImageIndex], imageIsReady, renderHasFinished, fences[nextImageIndex]);

            // Wait for commands to be submitted
            if (vkWaitForFences(application.logicalDevice, 1, &fences[nextImageIndex], VK_TRUE, std::numeric_limits<std::uint64_t>::max()) != VK_SUCCESS) {
                throw std::runtime_error("Fence buffer timed out.");
            }

            // Present the image
            resizeWindow = presentToScreen(application, renderHasFinished, nextImageIndex);

        }

        // Wait for the GPU to have finished all processes before cleanup
        vkDeviceWaitIdle(application.logicalDevice);

        // Clean up and close the application
        // Destroy buffers
        worldUniformBuffer.~BufferSet();
        for (size_t i = 0; i < meshes.size(); i++) {
            meshes[i].vertexPositions.~BufferSet();
            meshes[i].vertexUVs.~BufferSet();
            meshes[i].vertexNormals.~BufferSet();
            meshes[i].vertexTangents.~BufferSet();
            meshes[i].vertexMaterials.~BufferSet();
            meshes[i].indices.~BufferSet();
        }
        for (size_t i = 0; i < alphaMeshes.size(); i++) {
            alphaMeshes[i].vertexPositions.~BufferSet();
            alphaMeshes[i].vertexUVs.~BufferSet();
            alphaMeshes[i].vertexNormals.~BufferSet();
            alphaMeshes[i].vertexTangents.~BufferSet();
            alphaMeshes[i].vertexMaterials.~BufferSet();
            alphaMeshes[i].indices.~BufferSet();
        }
        lightingUniformBuffer.~BufferSet();
        
        // Destroy command related components
        vkDestroyDescriptorPool(application.logicalDevice, descriptorPool, nullptr);
        vkDestroySemaphore(application.logicalDevice, renderHasFinished, nullptr);
        vkDestroySemaphore(application.logicalDevice, imageIsReady, nullptr);
        vkDestroyCommandPool(application.logicalDevice, commandPool, nullptr);
        for (size_t i = 0; i < swapchainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(application.logicalDevice, swapchainFramebuffers[i], nullptr);
            vkDestroyFence(application.logicalDevice, fences[i], nullptr);
        }

        // Destroy image related components
        vkDestroySampler(application.logicalDevice, sampler, nullptr);
        vmaDestroyImage(allocator, depthBuffer.image, depthBuffer.allocation);
        vkDestroyImageView(application.logicalDevice, depthBuffer.imageView, nullptr);
        for (size_t i = 0; i < colourTextures.size(); i++) {
            vmaDestroyImage(allocator, colourTextures[i].image, colourTextures[i].allocation);
            vkDestroyImageView(application.logicalDevice, colourTextures[i].imageView, nullptr);
        }
        for (size_t i = 0; i < specularTextures.size(); i++) {
            vmaDestroyImage(allocator, specularTextures[i].image, specularTextures[i].allocation);
            vkDestroyImageView(application.logicalDevice, specularTextures[i].imageView, nullptr);
        }
        for (size_t i = 0; i < normalTextures.size(); i++) {
            vmaDestroyImage(allocator, normalTextures[i].image, normalTextures[i].allocation);
            vkDestroyImageView(application.logicalDevice, normalTextures[i].imageView, nullptr);
        }

        // Destroy pipeline related components
        vkDestroyPipeline(application.logicalDevice, pipeline, nullptr);
        vkDestroyPipeline(application.logicalDevice, alphaPipeline, nullptr);
        vkDestroyShaderModule(application.logicalDevice, vertexShader, nullptr);
        vkDestroyShaderModule(application.logicalDevice, fragmentShader, nullptr);
        vkDestroyShaderModule(application.logicalDevice, alphaFragmentShader, nullptr);
        vkDestroyPipelineLayout(application.logicalDevice, pipelineLayout, nullptr);

        // Destroy descriptor set layouts
        vkDestroyDescriptorSetLayout(application.logicalDevice, worldDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(application.logicalDevice, textureDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(application.logicalDevice, lightDescriptorSetLayout, nullptr);

        // Destroy Renderpass
        vkDestroyRenderPass(application.logicalDevice, renderPass, nullptr);
        
        // Destroy memory
        vmaDestroyAllocator(allocator);

        // Destroy the application
        application.cleanup();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 1;
}

namespace {

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        CameraInfo* camera = static_cast<CameraInfo*>(glfwGetWindowUserPointer(window));
        
        // Escape key should close the window
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        bool keyState = action;
        glm::vec3 movement = glm::vec3(0,0,0);

        // W key moves the camera forwards
        if (key == GLFW_KEY_W) {
            movement = glm::vec3(0.0, 0.0, -0.1);
        }

        // A key moves the camera to the left
        if (key == GLFW_KEY_A) {
            movement = glm::vec3(-0.1, 0.0, 0.0);
        }

        // S key moves the camera forwards
        if (key == GLFW_KEY_S) {
            movement = glm::vec3(0.0, 0.0, 0.1);
        }

        // F key moves the camera to the right
        if (key == GLFW_KEY_D) {
            movement = glm::vec3(0.1, 0.0, 0.0);
        }

        // E key moves the camera upwards
        if (key == GLFW_KEY_E) {
            movement = glm::vec3(0.0, 0.1, 0.0);
        }

        // Q key moves the camera downwards
        if (key == GLFW_KEY_Q) {
            movement = glm::vec3(0.0, -0.1, 0.0);
        }

        camera->position = camera->position + movement;
        camera->worldCameraMatrix = camera->worldCameraMatrix * glm::translate(movement);

    }

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        CameraInfo* camera = static_cast<CameraInfo*>(glfwGetWindowUserPointer(window));

        // Right mouse button toggles the viewing function
        if (GLFW_MOUSE_BUTTON_RIGHT == button && GLFW_PRESS == action) {
            camera->isLooking = !camera->isLooking;

            if (camera->isLooking) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }

    void mouseCallback(GLFWwindow* window, double mouseX, double mouseY) {
        CameraInfo* camera = static_cast<CameraInfo*>(glfwGetWindowUserPointer(window));

        if (camera->isLooking) {
            //std::cout << mouseX << " " << mouseY << std::endl;
            // Get the window dimensions
            int width, height;
            glfwGetWindowSize(window, &width, &height);

            // Get the mouse angles in relation to the centre of the screen
            camera->mousePosition.x = 0.005 * float((width / 2) - mouseX);
            camera->mousePosition.y = 0.005 * float((height / 2) - mouseY);

            // Rotate the camera
            /*
            */
            camera->worldCameraMatrix = camera->worldCameraMatrix 
                * glm::rotate(camera->mousePosition.y, glm::vec3(1.0, 0.0, 0.0)) 
                * glm::rotate(camera->mousePosition.x, glm::vec3(0.0, 1.0, 0.0));

            // Reset the mouse position
            glfwSetCursorPos(window, width / 2, height / 2);
        }

    }

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
        VkSubpassDescription subpasses[1]{};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pColorAttachments = &colourAttachment;
        subpasses[0].pDepthStencilAttachment = &depthAttachment;

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
        renderPassInfo.pSubpasses = subpasses;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = subpassDependencies;

        // Create the renderpass
        VkRenderPass renderPass;
        if (vkCreateRenderPass(app.logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create renderpass.");
        }

        return renderPass;
    }

    VkDescriptorSetLayout createWorldDescriptorSetLayout(app::AppContext& app) {
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

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        if (vkCreateDescriptorSetLayout(app.logicalDevice, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }

        return descriptorSetLayout;

    }

    VkDescriptorSetLayout createTextureDescriptorSetLayout(app::AppContext& app) {
        // Set the bindings for the descriptor
        // These are accessed in the shader as binding = n
        // All data passed into the shaders must have a binding
        int const numberOfBindings = 3;
        VkDescriptorSetLayoutBinding bindings[numberOfBindings]{};
        // Colour / Diffuse texture
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 512;         // MAX NUMBER OF BINDINGS
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // Specular textures
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 512;         // MAX NUMBER OF BINDINGS
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // Normal map textures
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = 512;         // MAX NUMBER OF BINDINGS
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Set the info of the descriptor
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = numberOfBindings;
        descriptorSetLayoutInfo.pBindings = bindings;
        descriptorSetLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        if (vkCreateDescriptorSetLayout(app.logicalDevice, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }

        return descriptorSetLayout;
    }

    VkDescriptorSetLayout createLightDescriptorSetLayout(app::AppContext& app) {
        // Set the bindings for the descriptor
        // These are accessed in the shader as binding = n
        // All data passed into the shaders must have a binding
        int const numberOfBindings = 1;
        VkDescriptorSetLayoutBinding bindings[numberOfBindings]{};
        // Number of lights in the descriptor
        //bindings[0].binding = 0;
        //bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        //bindings[0].descriptorCount = 1;         // MAX NUMBER OF BINDINGS
        //bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // Lighting data
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;         // MAX NUMBER OF LIGHTS
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Set the info of the descriptor
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = numberOfBindings;
        descriptorSetLayoutInfo.pBindings = bindings;
        descriptorSetLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
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
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
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
        VkRenderPass renderPass, VkShaderModule vertexShader, VkShaderModule fragmentShader,
        bool isAlpha){

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
        VkVertexInputBindingDescription vertexInputs[5]{};
        // Positions 3 floats
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(float) * 3;
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // Texture coords 2 floats
        vertexInputs[1].binding = 1;
        vertexInputs[1].stride = sizeof(float) * 2;
        vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // Normals 3 floats
        vertexInputs[2].binding = 2;
        vertexInputs[2].stride = sizeof(float) * 3;
        vertexInputs[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // Tangents 4 floats
        vertexInputs[3].binding = 3;
        vertexInputs[3].stride = sizeof(float) * 4;
        vertexInputs[3].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // Material ID 1 int
        vertexInputs[4].binding = 4;
        vertexInputs[4].stride = sizeof(uint32_t);
        vertexInputs[4].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Attributes of the above inputs
        VkVertexInputAttributeDescription vertexAttributes[5]{};
        // Positions
        vertexAttributes[0].binding = 0;
        vertexAttributes[0].location = 0;
        vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[0].offset = 0;
        // Texture coords
        vertexAttributes[1].binding = 1;
        vertexAttributes[1].location = 1;
        vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributes[1].offset = 0;
        // Normals
        vertexAttributes[2].binding = 2;
        vertexAttributes[2].location = 2;
        vertexAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[2].offset = 0;
        // Tangents
        vertexAttributes[3].binding = 3;
        vertexAttributes[3].location = 3;
        vertexAttributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[3].offset = 0;
        // Material ID
        vertexAttributes[4].binding = 4;
        vertexAttributes[4].location = 4;
        vertexAttributes[4].format = VK_FORMAT_R8_SINT;
        vertexAttributes[4].offset = 0;

        // Vertex shader info using the above descriptions
        VkPipelineVertexInputStateCreateInfo vertexInfo{};
        vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInfo.vertexBindingDescriptionCount = 5;
        vertexInfo.pVertexBindingDescriptions = vertexInputs;
        vertexInfo.vertexAttributeDescriptionCount = 5;
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
        if (isAlpha) {
            rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        }
        else {
            rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        }
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
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &colourBlendInfo;
        pipeInfo.layout = pipeLayout;
        pipeInfo.renderPass = renderPass;
        pipeInfo.subpass = 0;

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(app.logicalDevice, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline.");
        }

        return pipeline;
    }

    VkFramebuffer createFramebuffer(app::AppContext& app, VkRenderPass renderPass, std::vector<VkImageView>& buffers) {
        // Provides the information to create the framebuffer with
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.flags = 0;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = std::uint32_t(buffers.size());
        framebufferInfo.pAttachments = buffers.data();
        framebufferInfo.width = app.swapchainExtent.width;
        framebufferInfo.height = app.swapchainExtent.height;
        framebufferInfo.layers = 1;

        // Create the framebuffer
        VkFramebuffer framebuffer;
        if (vkCreateFramebuffer(app.logicalDevice, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer.");
        }
    
        return framebuffer;
    }

    VkSampler createTextureSampler(app::AppContext& app) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.mipLodBias = 0.f;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;

        VkSampler sampler = VK_NULL_HANDLE;
        if (vkCreateSampler(app.logicalDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sampler.");
        }

        return sampler;
    }

    VkDescriptorPool createDescriptorPool(app::AppContext& app) {
        // How many different descriptors should be available
        VkDescriptorPoolSize descriptorPoolSize[2];
        // Uniform descriptors
        descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSize[0].descriptorCount = 1024;
        // Texture descriptors
        descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorPoolSize[1].descriptorCount = 1024;

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = 2;
        descriptorPoolInfo.pPoolSizes = descriptorPoolSize;
        descriptorPoolInfo.maxSets = 2048;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

        VkDescriptorPool descriptorPool;
        if(vkCreateDescriptorPool(app.logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS){
            throw std::runtime_error("Failed to create decriptor pool.");
        }

        return descriptorPool;
    }

    VkDescriptorSet createDescriptorSet(app::AppContext& app, VkDescriptorPool pool, VkDescriptorSetLayout layout) {
        VkDescriptorSetAllocateInfo descriptorSetInfo{};
        descriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetInfo.descriptorPool = pool;
        descriptorSetInfo.descriptorSetCount = 1;
        descriptorSetInfo.pSetLayouts = &layout;

        VkDescriptorSet descriptorSet;
        if (vkAllocateDescriptorSets(app.logicalDevice, &descriptorSetInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set.");
        }

        return descriptorSet;
    }

    VkDescriptorSet createBufferDescriptorSet(app::AppContext& app, VkDescriptorPool pool, 
        VkDescriptorSetLayout layout, VkBuffer& buffer, VkDescriptorType descriptorType) {
        // Create the world descriptor set and fill with the information
        VkDescriptorSet descriptorSet = createDescriptorSet(app, pool, layout);

        // Buffer Info
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer;
        bufferInfo.range = VK_WHOLE_SIZE;

        // Descritor info set up
        VkWriteDescriptorSet descriptor{};
        descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor.dstSet = descriptorSet;
        descriptor.dstBinding = 0;      // Binding in the shader
        descriptor.descriptorCount = 1;
        descriptor.descriptorType = descriptorType;
        descriptor.pBufferInfo = &bufferInfo;

        // Update / initialise
        vkUpdateDescriptorSets(app.logicalDevice, 1, &descriptor, 0, nullptr);

        return descriptorSet;
    }

    VkDescriptorSet createBindlessImageDescriptorSet(app::AppContext& app, VkDescriptorPool pool, VkDescriptorSetLayout layout,
        std::vector<utility::ImageSet>& diffuseImages,
        std::vector<utility::ImageSet>& specularImages,
        std::vector<utility::ImageSet>& normalMapImages,
        VkSampler& sampler
    ) {
        // Create the world descriptor set and fill with the information
        VkDescriptorSet descriptorSet = createDescriptorSet(app, pool, layout);

        // Image info (Max number of textures in descriptor is 512)
        std::vector<VkDescriptorImageInfo> diffuseImageInfos(diffuseImages.size());
        for (size_t i = 0; i < diffuseImages.size(); i++) {
            diffuseImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            diffuseImageInfos[i].imageView = diffuseImages[i].imageView;
            diffuseImageInfos[i].sampler = sampler;
        }
        std::vector<VkDescriptorImageInfo> specularImageInfos(specularImages.size());
        for (size_t i = 0; i < specularImages.size(); i++) {
            specularImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            specularImageInfos[i].imageView = specularImages[i].imageView;
            specularImageInfos[i].sampler = sampler;
        }
        std::vector<VkDescriptorImageInfo> normalMapImageInfos(normalMapImages.size());
        for (size_t i = 0; i < normalMapImages.size(); i++) {
            normalMapImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalMapImageInfos[i].imageView = normalMapImages[i].imageView;
            normalMapImageInfos[i].sampler = sampler;
        }


        // Descritor info set up
        VkWriteDescriptorSet descriptor[3] {};
        // Colour / Diffuse
        descriptor[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor[0].dstSet = descriptorSet;
        descriptor[0].dstBinding = 0;      // Binding in the shader
        descriptor[0].descriptorCount = static_cast<uint32_t>(diffuseImageInfos.size());
        descriptor[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor[0].pImageInfo = diffuseImageInfos.data();
        // Specular
        descriptor[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor[1].dstSet = descriptorSet;
        descriptor[1].dstBinding = 1;      // Binding in the shader
        descriptor[1].descriptorCount = static_cast<uint32_t>(specularImageInfos.size());
        descriptor[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor[1].pImageInfo = specularImageInfos.data();
        // Normal map
        descriptor[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor[2].dstSet = descriptorSet;
        descriptor[2].dstBinding = 2;      // Binding in the shader
        descriptor[2].descriptorCount = static_cast<uint32_t>(normalMapImageInfos.size());
        descriptor[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor[2].pImageInfo = normalMapImageInfos.data();

        // Update / initialise
        vkUpdateDescriptorSets(app.logicalDevice, 3, descriptor, 0, nullptr);

        return descriptorSet;
    }

    void updateWorldUniforms(WorldView& worldUniform, float screenAspect, CameraInfo& cameraInfo) {
        // Update the projection matrix
        glm::mat4 projectionMatrix = glm::perspectiveRH_ZO(float(glm::radians(60.0f)), screenAspect, 0.1f, 100.0f);
        projectionMatrix[1][1] *= -1.f;

        // Update the camara matrix
        glm::mat4 cameraMatrix = glm::inverse(cameraInfo.worldCameraMatrix);

        // Create the projection camera matrix
        worldUniform.projectionCameraMatrix = projectionMatrix * cameraMatrix;
    
        // Update the camera position
        worldUniform.cameraPosition = cameraInfo.position;
    }

    void updateLightingUniforms(app::AppContext& app, VkBuffer lightingBuffer, LightingData lightData,
        VkCommandPool commandPool) {
        
        // Create the command buffer
        VkCommandBuffer commandBuffer = utility::createCommandBuffer(app, commandPool);

        // Begin recording into the command buffer
        VkCommandBufferBeginInfo recordInfo{};
        recordInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffer, &recordInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to start command buffer recording.");
        }

        // Place the data into the uniform buffer
        vkCmdUpdateBuffer(commandBuffer, lightingBuffer, 0, sizeof(LightingData), &lightData);

        // Transition to shader readable
        utility::createBufferBarrier(lightingBuffer, VK_WHOLE_SIZE,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );

        // End the recording
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to end command buffer recording.");
        }

        // Use a fence to ensure that transfers are complete before moving on
        VkFence submitComplete = utility::createFence(app);

        // Submit the recorded commands for execution
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        if (vkQueueSubmit(app.graphicsQueue, 1, &submitInfo, submitComplete)) {
            throw std::runtime_error("Failed to submit recorded commands.");
        }

        // Wait for the fence before clean up
        if (vkWaitForFences(app.logicalDevice, 1, &submitComplete, VK_TRUE, std::numeric_limits<std::uint64_t>::max())) {
            throw std::runtime_error("Fence failed to return as complete.");
        }

        vkFreeCommandBuffers(app.logicalDevice, commandPool, 1, &commandBuffer);
        vkDestroyFence(app.logicalDevice, submitComplete, nullptr);
    }

    void recreateSwapchain(app::AppContext& app) {
        // Wait for the device to idle
        vkDeviceWaitIdle(app.logicalDevice);

        // Clear the old swapchain image views
        // Destroy swapchain image views
        for (size_t i = 0; i < app.swapchainImageViews.size(); i++) {
            vkDestroyImageView(app.logicalDevice, app.swapchainImageViews[i], nullptr);
        }
        app.swapchainImages.clear();
        app.swapchainImageViews.clear();

        // Destroy old swapchain
        vkDestroySwapchainKHR(app.logicalDevice, app.swapchain, nullptr);

        // Recreate the swapchain
        app::swapchainSetup(&app);

        // Recreate the swapchain images
        app::createSwapchainImages(&app);
    }

    void recordCommands(
        VkCommandBuffer commandBuffer,                              // Command buffer
        VkBuffer worldUniformBuffer, WorldView worldUniform,        // World Uniform
        VkRenderPass renderPass, VkFramebuffer frameBuffer,         // Render pass
        VkRect2D renderArea,
        VkPipeline pipeline, VkPipeline alphaPipeline,              // Pipelines
        VkPipelineLayout pipelineLayout,                            // Pipelines
        VkDescriptorSet worldDescriptorSet,                         // World descriptors
        VkDescriptorSet textureDescriptorSet,                       // Texture descriptors
        VkDescriptorSet lightingDescriptorSet,                      // Lighting descriptors
        std::vector<model::Mesh>& meshes,                           // Mesh data
        std::vector<model::Mesh>& alphaMeshes,                      // Mesh data
        std::vector<VkDeviceSize>& vertexOffsets,                   // Per vertex data
        std::vector<fbx::Material>& materials                       // Material data
    ) {

        // Set up and start the command buffer recording
        VkCommandBufferBeginInfo recordInfo{};
        recordInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        recordInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &recordInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to start command buffer recording.");
        }

        // Upload any uniforms that may have been updated
        // Re-assign the usage of the buffer
        utility::createBufferBarrier(worldUniformBuffer, VK_WHOLE_SIZE,
            VK_ACCESS_UNIFORM_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        // Update
        vkCmdUpdateBuffer(commandBuffer, worldUniformBuffer, 0, sizeof(WorldView), &worldUniform);
        // Re-assign the usage of the buffer back to its original state
        utility::createBufferBarrier(worldUniformBuffer, VK_WHOLE_SIZE,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );

        // Define a colour for background of the renderpass
        VkClearValue backgroundColour[2]{};
        // Swapchain colour background
        backgroundColour[0].color.float32[0] = 0.6f;
        backgroundColour[0].color.float32[1] = 0.1f;
        backgroundColour[0].color.float32[2] = 0.1f;
        backgroundColour[0].color.float32[3] = 1.0f;
        // Depth buffer clear background
        backgroundColour[1].depthStencil.depth = 1.0f;

        // Begin the render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea = renderArea;
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = backgroundColour;
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Select a pipeline to draw with
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        // Bind the uniforms to the pipeline
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &worldDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &textureDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &lightingDescriptorSet, 0, nullptr);

        // Draw each separate mesh to screen
        for (size_t i = 0; i < meshes.size(); i++) {
            // Bind the per vertex buffers
            VkBuffer buffers[5] = { meshes[i].vertexPositions.buffer, meshes[i].vertexUVs.buffer, meshes[i].vertexNormals.buffer, meshes[i].vertexTangents.buffer,meshes[i].vertexMaterials.buffer};
            VkDeviceSize offsets[5]{};
            vkCmdBindVertexBuffers(commandBuffer, 0, 5, buffers, offsets);

            // Bind the index buffer
            vkCmdBindIndexBuffer(commandBuffer, meshes[i].indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            // Do the draw call
            vkCmdDrawIndexed(commandBuffer, meshes[i].numberOfIndices, 1, 0, 0, 0);
        }

        // Select the alpha pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaPipeline);
        // Draw each separate mesh to screen
        for (size_t i = 0; i < alphaMeshes.size(); i++) {
            // Bind the per vertex buffers
            VkBuffer buffers[5] = { alphaMeshes[i].vertexPositions.buffer, alphaMeshes[i].vertexUVs.buffer, alphaMeshes[i].vertexNormals.buffer, alphaMeshes[i].vertexTangents.buffer, alphaMeshes[i].vertexMaterials.buffer};
            VkDeviceSize offsets[5]{};
            vkCmdBindVertexBuffers(commandBuffer, 0, 5, buffers, offsets);

            // Bind the index buffer
            vkCmdBindIndexBuffer(commandBuffer, alphaMeshes[i].indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            // Do the draw call
            vkCmdDrawIndexed(commandBuffer, alphaMeshes[i].numberOfIndices, 1, 0, 0, 0);
        }

        // End the renderpass
        vkCmdEndRenderPass(commandBuffer);

        // End the command buffer recording
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record to the command buffer.");
        }

        return;
    }

    void submitCommands(app::AppContext app, VkCommandBuffer commandBuffer, VkSemaphore wait, VkSemaphore signal, VkFence fence) {
        // Create the submission info
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Wait for the colour attatchment to be finished
        VkPipelineStageFlags waitForColour = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &wait;
        submitInfo.pWaitDstStageMask = &waitForColour;

        // Signal that there has been a submission
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signal;

        // Submit
        if (vkQueueSubmit(app.graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit the command buffer.");
        }

        return;
    }

    bool presentToScreen(app::AppContext app, VkSemaphore finishedSemaphore, std::uint32_t swapchainIndex) {
        // Set up the presentation info
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &finishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &app.swapchain;
        presentInfo.pImageIndices = &swapchainIndex;
        presentInfo.pResults = nullptr;

        // Present
        VkResult result = vkQueuePresentKHR(app.presentQueue, &presentInfo);
            
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            return true;
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present the swapchain.");
        }

        return false;
    }
}