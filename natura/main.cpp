// glew must be before glfw
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// contains helper functions such as shader compiler
#include "icg_helper.h"

#include "game/game.h"

/* The following includes are not useful. But if you are using CLion not adding
 * them will throw you errors in all the following files (even though the code
 * compiles just fine.).
 */
#include "misc/io/input/handlers/keyboard/keyboard_handler.h"
#include "misc/io/input/handlers/mouse/mouse_button_handler.h"
#include "misc/io/input/handlers/mouse/mouse_cursor_handler.h"
#include "misc/io/input/handlers/framebuffer/framebuffer_size_handler.h"
#include "water_grid/water_grid.h"
#include "physics/material_point.h"
#include "physics/ball.h"
#include "camera/camera.h"

using namespace glm;


//calibration values

void ErrorCallback(int error, const char *description) {
    fputs(description, stderr);
}

int main(int argc, char *argv[]) {
    int window_width = 800;
    int window_height = 600;
    // GLFW Initialization
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(ErrorCallback);

    // hint GLFW that we would like an OpenGL 3 context (at least)
    // http://www.glfw.org/faq.html#how-do-i-create-an-opengl-30-context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // attempt to open the window: fails if required version unavailable
    // note some Intel GPUs do not support OpenGL 3.2
    // note update the driver of your graphic card
    GLFWwindow *window = glfwCreateWindow(window_width, window_height,
                                          "Natura", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // makes the OpenGL context of window current on the calling thread
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // GLEW Initialization (must have a context)
    // https://www.opengl.org/wiki/OpenGL_Loading_Library
    glewExperimental = GL_TRUE; // fixes glew error (see above link)
    if (glewInit() != GLEW_NO_ERROR) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return EXIT_FAILURE;
    }

    cout << "OpenGL" << glGetString(GL_VERSION) << endl;
    Game game(window);
    game.run();
    // close OpenGL window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
