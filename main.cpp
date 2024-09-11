#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <string>
#include <optional>

#include "setup.hpp"

namespace {


}

int main() {
    try {
        // -- The setup -- //

        app::AppContext application = app::setup();

        // -- The setup -- //


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

}