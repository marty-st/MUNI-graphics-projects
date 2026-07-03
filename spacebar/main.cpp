#define GLFW_INCLUDE_NONE

#include <string>
#include <vector>
#include "application.hpp"
#include "gui_manager.h"

int main(int argc, char** argv) {
    int initial_width = 1280;
    int initial_height = 720;

    std::vector<std::string> arguments(argv, argv + argc);

    ImGuiManager manager;

    /* MSAA */
    manager.set_multisampling_per_pixel(4);

    manager.init(initial_width, initial_height, "PV112 Template", 4, 5);
    if(!manager.is_fail()) 
    {
        // Note that the application has to be created after the manager is initialized.
        Application application(initial_width, initial_height, arguments);
        manager.run(application);

        // Free the entire application before terminating glfw. If this were done in the wrong order
        // application may crash on calling OpenGL (Delete*) calls after destruction of a context.
        // Freeing is done implicitly by enclosing this part of code in block {}.
    }

    manager.terminate();
    return 0;
}
