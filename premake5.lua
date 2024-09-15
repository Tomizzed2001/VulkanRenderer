workspace "VulkanRenderer"
    language "C++"
    cppdialect "C++20"
    platforms { "x64" }
    configurations { "debug", "release" }

    --configurations
    filter "debug"
        defines { "_DEBUG=1" }
        symbols "On"

    filter "release"
        defines { "NDEBUG=1" }
        optimize "On"

    filter "*"

    -- Include files
    includedirs{"ExternalLibraries/Vulkan/Include", "ExternalLibraries/glfw/include", "ExternalLibraries/glm", "ExternalLibraries/VulkanMemoryAllocator/include"}

    -- Libraries
    libdirs{"ExternalLibraries/glfw/lib-vc2022", "ExternalLibraries/Vulkan/Lib"}
    links{"vulkan-1.lib", "glfw3.lib"}

project "Renderer"
    kind "ConsoleApp"
    location "src"
    files {"src/**.cpp", "src/**.hpp"}

    dependson "glm" 

project "Shaders"
    kind "Utility"
    location "src/Shaders"
    files {"src/Shaders/**.vert", "src/Shaders/**.frag"}
        
