#pragma once

typedef  void(*callback)(GLFWwindow*, int, int, int , int);

class Keyboard {

private :

    std::map<tuple(GLFWwindow*, int, int, int, int), callback>* mKeyMap;

public:

    Keyboard(){
        mKeyMap = new map<tuple(GLFWwindow*, int, int, int, int), callback>();
    }

    ~Keyboard(){
        delete mKeyMap;
    }

    void addKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods, callback function){
        mKeyMap->insert(std::map<tuple(GLFWwindow*, int, int, int, int), callback>::value_type(tuple(window, key, scancode, action, mods), function));
    }

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        //Just acces mKeyMap and call the callback function.
    }
};


