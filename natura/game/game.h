#pragma once

#include "../projection.h"
#include "../camera/camera.h"
#include "../perlin_noise/perlinnoise.h"
#include "../trackball.h"
#include "../../external/glm/detail/type_mat.hpp"
#include "../skybox/skybox.h"
#include "../terrain/terrain.h"
#include "../misc/observer_subject/messages/keyboard_handler_message.h"
#include "../misc/io/input/handlers/keyboard/keyboard_handler.h"
#include "../misc/io/input/handlers/mouse/mouse_button_handler.h"
#include "../misc/io/input/handlers/mouse/mouse_cursor_handler.h"
#include "../misc/io/input/handlers/framebuffer/framebuffer_size_handler.h"
#include "../config.h"
#include "../shadows/shadowbuffer.h"
#include "../shadows/attrib_locations.h"
#include <glm/gtc/matrix_transform.hpp>

class Game : public Observer {
public:
    Game(GLFWwindow *window) : m_keyboard_handler(window), m_mouse_button_handler(window),
                               m_mouse_cursor_handler(window), m_frame_buffer_size_handler(window) {
        glfwGetWindowSize(window, &m_window_width, &m_window_height);
        m_window = window;
        m_amplitude = 9.05f;
        Init();
        glfwGetFramebufferSize(window, &m_window_width, &m_window_height);
        FrameBufferSizeHandlerMessage m(window, m_window_width, m_window_height);
        resize_callback(&m);
        m_look_curve.setTimeLength(10.f);
        m_pos_curve.setTimeLength(10.f);
        m_draw_curves = false;
        m_loop_curves = false;
    }

    ~Game() {
        m_perlinNoise->Cleanup();
        m_terrain->Cleanup();
        delete m_perlinNoise;
    }

    void run() {
        m_keyboard_handler.attach(this);
        m_mouse_button_handler.attach(this);
        m_mouse_cursor_handler.attach(this);
        m_frame_buffer_size_handler.attach(this);
        // render loop
        while (!glfwWindowShouldClose(m_window)) {
            glm::vec3 pos = m_camera->getPosition();

            Display();
            glfwSwapBuffers(m_window);
            glfwPollEvents();
        }
    }

    void update(Message *msg) {
        switch (msg->getType()) {
            case Message::Type::KEYBOARD_HANDLER_INPUT :
                keyCallback(reinterpret_cast<KeyboardHandlerMessage *>(msg));
                break;

            case Message::Type::MOUSE_BUTTON_INPUT :
                mouseButtonCallback(
                        reinterpret_cast<MouseButtonHandlerMessage *>(msg));
                break;

            case Message::Type::MOUSE_CURSOR_INPUT :
                mouseCursorCallback(reinterpret_cast<MouseCursorHandlerMessage *>(msg));
                break;

            case Message::Type::FRAMEBUFFER_SIZE_CHANGE :
                resize_callback(reinterpret_cast<FrameBufferSizeHandlerMessage *>(msg));
                break;

            default:
                throw std::string("Error : Unexpected message in class Game");
        }
    }

private:

    enum CameraMode {
        FLYTHROUGH, FPS
    };


    double m_last_mouse_xpos, m_last_mouse_ypos;
    float m_last_time_tick;
    float m_last_time_frame;

    /* Window size */
    int m_window_width;
    int m_window_height;
    GLFWwindow *m_window;

    /* Camera and view */
    Camera *m_camera;
    glm::mat4 m_grid_model_matrix;
    Projection *m_projection;
    float m_fps_sensitivity = 0.1;

    Trackball *m_trackball;

    /* Perlin noise generator for the game. */
    PerlinNoise *m_perlinNoise;

    /* Terrain and sky */
    Terrain *m_terrain;
    float m_amplitude;


    /* Bezier Curve for camera */
    BezierCurve m_pos_curve;
    BezierCurve m_look_curve;
    bool m_draw_curves;
    bool m_loop_curves;


    /* Input handlers */
    KeyboardHandler m_keyboard_handler;
    MouseButtonHandler m_mouse_button_handler;
    MouseCursorHandler m_mouse_cursor_handler;
    FrameBufferSizeHandler m_frame_buffer_size_handler;

    FrameBuffer framebufferFloor;

    bool m_reverse = false;

    /* Shadows. */
    GLuint m_default_pid; /* Default pid. */
    ShadowBuffer m_shadow_buffer;
    GLuint m_shadow_pid;      // Handle for the shadow map genration shader program
    GLuint m_depth_tex;       // Handle for the shadow map
    vec3 m_light_dir;         // Direction towards the light
    mat4 m_light_projection;  // Projection matrix for light source
    bool m_show_shadow = true;
    bool m_do_pcf = true;
    float m_bias = 0.0f;
    glm::mat4 m_offset_matrix;
    bool m_draw_from_light_pov = false;
    float m_near = -10.f;
    float m_light_height = 5.f;

    /* Private function. */
    void Init() {
        const bool top_down_view = false;
        const int TERRAIN_SIZE = 10;
        const int VERT_PER_GRID_SIDE = 16;
        const float cam_posxy = TERRAIN_SCALE * ((float) (TERRAIN_SIZE * CHUNK_SIDE_TILE_COUNT)) / 2.0f;

        vec3 starting_camera_position;
        vec2 starting_camera_rotation;
        if (top_down_view) {
            starting_camera_position = vec3(0.0f, 0.0f, -0.0f);
            starting_camera_rotation = vec2(180.0f, 0.0f);
        }
        else {
            starting_camera_position = vec3(-cam_posxy, -5.0f, -cam_posxy);
            starting_camera_rotation = vec2(0.0f);
        }
        m_trackball = new Trackball();
        m_projection = new Projection(45.0f, (GLfloat) m_window_width / m_window_height, 0.025f, 400.0f);
        m_perlinNoise = new PerlinNoise(m_window_width, m_window_height, glm::vec2(TERRAIN_SIZE, TERRAIN_SIZE));
        m_terrain = new Terrain(TERRAIN_SIZE, VERT_PER_GRID_SIDE, m_perlinNoise);
        m_camera = new Camera(starting_camera_position, starting_camera_rotation, m_terrain);
        //m_camera->enableFpsMode();

        // sets background color b
        glClearColor(0, 0, 0/*gray*/, 1.0 /*solid*/);

        // enable depth test.
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        m_grid_model_matrix = IDENTITY_MATRIX;
        //m_grid_model_matrix = translate(m_grid_model_matrix, vec3(-4.0f, -0.25f, -4.0f));
        m_grid_model_matrix = scale(m_grid_model_matrix, vec3(TERRAIN_SCALE, TERRAIN_SCALE, TERRAIN_SCALE));


        //int perlinNoiseTex = perlinNoise.generateNoise(H, frequency, lacunarity, offset, octaves);
        BASE_TILE = new Grid(VERT_PER_GRID_SIDE, glm::vec2(0, 0));
        BASE_TILE->Init(0);
        m_perlinNoise->Init();
        GLuint fb_tex = framebufferFloor.Init(m_window_width, m_window_height, GL_RGB8);
        m_terrain->Init(fb_tex);

        m_light_dir = vec3(0.0, m_light_height, 0.0);
        m_light_dir = normalize(m_light_dir);
        m_default_pid = BASE_TILE->getPID();
        if(!m_default_pid) {
            exit(EXIT_FAILURE);
        }

        m_shadow_pid = icg_helper::LoadShaders("shadow_map_vshader.glsl",
                                             "shadow_map_fshader.glsl");
        if(!m_shadow_pid) {
            exit(EXIT_FAILURE);
        }
        BASE_TILE->setShadowPID(m_shadow_pid);
        glBindAttribLocation(m_shadow_pid, ATTRIB_LOC_position, "position");
        glLinkProgram(m_shadow_pid);

        glViewport(0,0,m_window_width,m_window_height);

        float ratio = m_window_width / (float) m_window_height;

        // Orthographic projection, assuming directional light.
        //float ext = 1500.0f;
        //m_light_projection = ortho(-ext, ext, -ext, ext, -10.f, ext);

        // Matrix that can be used to move a point's components from [-1, 1] to [0, 1].
        m_offset_matrix = mat4(0.5f, 0.0f, 0.0f, 0.0f,
                             0.0f, 0.5f, 0.0f, 0.0f,
                             0.0f, 0.0f, 0.5f, 0.0f,
                             0.5f, 0.5f, 0.5f, 1.0f);

        check_error_gl();
        m_depth_tex = m_shadow_buffer.Init();
        BASE_TILE->setDepthTex(m_depth_tex);
    }

    void Display() {
        glViewport(0, 0, m_window_width, m_window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float time = glfwGetTime();

        cout << "Frames : " << 1 / (time - m_last_time_frame) << endl;
        m_last_time_frame = time;

        //tick 60 times per second
        if (time - m_last_time_tick >= TICK) {
            cout << "Ticks : " << 1 / (time - m_last_time_tick) << endl;
            m_last_time_tick = time;
            m_camera->tick();
        }

        glm::vec3 tmp = -m_camera->getPosition();
        m_light_dir = vec3(tmp.x+25, tmp.z+m_light_height, tmp.z-25);
        //m_light_dir = normalize(m_light_dir);
        float ext = 100.0f;
        m_light_projection = ortho(-ext, ext, -ext, ext, -ext, ext);
        //draw as often as possible
        /* First the shadow map.*/
        glUseProgram(m_shadow_pid);
        m_shadow_buffer.Bind();

        vec3 up(0.0f, 1.0f, 0.0f);
        if (abs(dot(m_light_dir, up)) > 0.99) {
            //up = vec3(0.0f, 0.0f, 1.0f);
        }
        //glm::vec3 eye = -glm::vec3(4*m_light_dir.x, 4*m_light_dir.y, 4*m_light_dir.z);
        mat4 light_view = lookAt(m_light_dir, tmp, up);
        mat4 depth_vp = m_light_projection * light_view;
        glUniformMatrix4fv(glGetUniformLocation(m_shadow_pid, "depth_vp"), 1,
                           GL_FALSE, value_ptr(depth_vp));


        glClear(GL_DEPTH_BUFFER_BIT);
        BASE_TILE->setUseShadowPID(true);
        m_terrain->Draw(m_amplitude, time, m_camera->getPosition(), true, false, m_grid_model_matrix,
                        light_view,
                        m_light_projection);
        BASE_TILE->setUseShadowPID(false);
        m_shadow_buffer.Unbind();

        glUseProgram(m_default_pid);
        glUniform3fv(glGetUniformLocation(m_default_pid, "sun_light_dir"), 1,
                     value_ptr(m_light_dir));

        // Set matrix to transform from world space into NDC and then into [0, 1] ranges.
        mat4 depth_vp_offset = m_offset_matrix * depth_vp;
        glUniformMatrix4fv(glGetUniformLocation(m_default_pid, "depth_vp_offset"), 1,
                           GL_FALSE, value_ptr(depth_vp_offset));

        glUniform1f(glGetUniformLocation(m_default_pid, "bias"), m_bias);

        glUniform1i(glGetUniformLocation(m_default_pid, "show_shadow"), m_show_shadow);
        glUniform1i(glGetUniformLocation(m_default_pid, "do_pcf"), m_do_pcf);



        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* Reflection */
        glEnable(GL_CLIP_PLANE0);
        framebufferFloor.Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_terrain->Draw(m_amplitude, time, m_camera->getPosition(), true, true, m_grid_model_matrix,
                        m_camera->getMirroredMatrix(m_terrain->m_water_height * -CHUNK_SIDE_TILE_COUNT * TERRAIN_SCALE),
                        m_projection->perspective());
        framebufferFloor.Unbind();
        glDisable(GL_CLIP_PLANE0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_terrain->ExpandTerrain(m_camera->getPosition());
        if (!m_draw_from_light_pov) {
            m_terrain->Draw(m_amplitude, time, m_camera->getPosition(), false, true, m_grid_model_matrix,
                            m_camera->GetMatrix(),
                            m_projection->perspective());
        }
        else {
            m_terrain->Draw(m_amplitude, time, m_camera->getPosition(), false, false, m_grid_model_matrix,
                            light_view,
                            m_light_projection);
        }
        if (m_look_curve.Size() > 1 && m_pos_curve.Size() > 1 && m_draw_curves) {
            m_look_curve.Draw(m_grid_model_matrix, m_camera->GetMatrix(), m_projection->perspective());
            m_pos_curve.Draw(m_grid_model_matrix, m_camera->GetMatrix(), m_projection->perspective());
        }
    }

    // transforms glfw screen coordinates into normalized OpenGL coordinates.
    static vec2 TransformScreenCoords(GLFWwindow *window, int x, int y) {
        // the framebuffer and the window doesn't necessarily have the same size
        // i.e. hidpi screens. so we need to get the correct one
        int width;
        int height;
        glfwGetWindowSize(window, &width, &height);
        return vec2(2.0f * (float) x / width - 1.0f,
                    1.0f - 2.0f * (float) y / height);
    }

    void mouseButtonCallback(MouseButtonHandlerMessage *message) {
        int button = message->getButton();
        int action = message->getAction();
        GLFWwindow *window = message->getWindow();
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            double x_i, y_i;
            glfwGetCursorPos(window, &x_i, &y_i);
            m_last_mouse_xpos = x_i;
            m_last_mouse_ypos = y_i;
        }
    }

    void mouseCursorCallback(MouseCursorHandlerMessage *message) {
        GLFWwindow *window = message->getWindow();
        double x = message->getCoordX();
        double y = message->getCoordY();
        double diffx = x - m_window_width / 2; //check the difference between the current x and the last x position
        double diffy = y - m_window_height / 2; //check the difference between the current y and the last y position
        m_last_mouse_xpos = x; //set lastx to the current x position
        m_last_mouse_ypos = y; //set lasty to the current y position
        float xrot =
                (float) diffx * 0.1f; //set the xrot to xrot with the addition of the difference in the y position
        float yrot =
                (float) diffy * 0.1f;// set the xrot to yrot with the addition of the difference in the x position
        vec2 tmp = vec2(xrot * m_fps_sensitivity, yrot * m_fps_sensitivity);
        m_camera->AddRotationFPS(tmp);
        glfwSetCursorPos(window, m_window_width / 2, m_window_height / 2);
    }

    // Gets called when the windows/framebuffer is resized.
    void resize_callback(FrameBufferSizeHandlerMessage *message) {
        int width = message->getWidth();
        int height = message->getHeight();
        m_window_width = width;
        m_window_height = height;
        m_projection->reGenerateMatrix((GLfloat) m_window_width / m_window_height);
        glViewport(0, 0, m_window_width, m_window_height);
        framebufferFloor.Cleanup();
        GLuint fb_tex = (GLuint) framebufferFloor.Init(m_window_width, m_window_height, GL_RGB8);
        m_terrain->Init(fb_tex);
    }

    void clearCurves() {
        if (m_camera->getCameraMode() == CAMERA_MODE::Bezier) {
            m_camera->enableFlyThroughtMode();
        }
        m_look_curve.Clear();
        m_pos_curve.Clear();
    }

    void keyCallback(KeyboardHandlerMessage *message) {
        GLFWwindow *window = message->getWindow();
        int key = message->getKey();
        int mods = message->getMods();
        int action = message->getAction();
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_W && !m_camera->hasAcceleration(DIRECTION::Forward)) {
                m_camera->setMovement(DIRECTION::Forward);
            }
            if (key == GLFW_KEY_S && !m_camera->hasAcceleration(DIRECTION::Backward)) {
                m_camera->setMovement(DIRECTION::Backward);
            }
            if (key == GLFW_KEY_A && !m_camera->hasAcceleration(DIRECTION::Left)) {
                m_camera->setMovement(DIRECTION::Left);
            }
            if (key == GLFW_KEY_D && !m_camera->hasAcceleration(DIRECTION::Right)) {
                m_camera->setMovement(DIRECTION::Right);
            }
            if (key == GLFW_KEY_Q && !m_camera->hasAcceleration(DIRECTION::Up)) {
                m_camera->setMovement(DIRECTION::Up);
            }
            if (key == GLFW_KEY_E && !m_camera->hasAcceleration(DIRECTION::Down)) {
                m_camera->setMovement(DIRECTION::Down);
            }
            if (key == GLFW_KEY_R) {
                glm::vec3 pos_point = -m_camera->getPosition() / TERRAIN_SCALE;
                glm::vec3 look_point = -m_camera->getFrontPoint(2.0f) / TERRAIN_SCALE;
                cout << "Point added to bezier curve : (" << pos_point.x << ", " << pos_point.y << ", " <<
                pos_point.z << ") looking at (" << look_point.x << ", " << look_point.y << ", " << look_point.z <<
                ")" << endl;
                m_look_curve.addPoint(look_point);
                m_pos_curve.addPoint(pos_point);
            }
            if (key == GLFW_KEY_T) {
                m_draw_curves = !m_draw_curves;
                cout << "Bezier curve draw " << (m_draw_curves ? "ON." : "OFF.") << endl;
            }
            if (key == GLFW_KEY_C) {
                clearCurves();
                cout << "Bezier curve cleared" << endl;
            }
            if (key == GLFW_KEY_SPACE) {
                if (m_camera->getCameraMode() == CAMERA_MODE::Bezier)
                    m_camera->enableFlyThroughtMode();
                else if (m_look_curve.Size() > 1 && m_pos_curve.Size() > 1)
                    m_camera->enableBezierMode(&m_pos_curve, &m_look_curve);
            }
            if (key == GLFW_KEY_L) {
                m_loop_curves = !m_loop_curves;
                m_look_curve.enableLoop(m_loop_curves);
                m_pos_curve.enableLoop(m_loop_curves);
            }
            if (key == GLFW_KEY_P) {
                m_reverse = !m_reverse;
            }
            if (key == GLFW_KEY_F){
                if (m_camera->getCameraMode() == CAMERA_MODE::Fps)
                    m_camera->enableFlyThroughtMode();
                else
                    m_camera->enableFpsMode();
            }
            if (key == GLFW_KEY_Z){
                m_draw_from_light_pov = !m_draw_from_light_pov;
            }
            if (key == GLFW_KEY_LEFT && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
                m_bias -= 0.0005f;
            }

            else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
                m_bias += 0.0005f;
            }
        }

        if (action == GLFW_RELEASE) {
            if (key == GLFW_KEY_W && m_camera->hasAcceleration(DIRECTION::Forward)) {
                m_camera->stopMovement(DIRECTION::Forward);
            }
            if (key == GLFW_KEY_S && m_camera->hasAcceleration(DIRECTION::Backward)) {
                m_camera->stopMovement(DIRECTION::Backward);
            }
            if (key == GLFW_KEY_A && m_camera->hasAcceleration(DIRECTION::Left)) {
                m_camera->stopMovement(DIRECTION::Left);
            }
            if (key == GLFW_KEY_D && m_camera->hasAcceleration(DIRECTION::Right)) {
                m_camera->stopMovement(DIRECTION::Right);
            }
            if (key == GLFW_KEY_Q && m_camera->hasAcceleration(DIRECTION::Up)) {
                m_camera->stopMovement(DIRECTION::Up);
            }
            if (key == GLFW_KEY_E && m_camera->hasAcceleration(DIRECTION::Down)) {
                m_camera->stopMovement(DIRECTION::Down);
            }
        }

        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_UP) {
                m_near += 0.5;
            }

            if (key == GLFW_KEY_DOWN) {
                m_near -= 0.5;
            }
            if (key == GLFW_KEY_RIGHT) {
                m_light_height += 1;
            }

            if (key == GLFW_KEY_LEFT) {
                m_light_height-= 1;
            }
            switch (key) {
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window,
                                             GL_TRUE);
                    break;

                case GLFW_KEY_H:
                    m_perlinNoise->
                            setProperty(PerlinNoiseProperty::H,
                                        m_perlinNoise
                                                ->
                                                        getProperty(PerlinNoiseProperty::H)
                                        + .05f);
                    break;

                case GLFW_KEY_N:
                    m_perlinNoise->
                            setProperty(PerlinNoiseProperty::H,
                                        m_perlinNoise
                                                ->
                                                        getProperty(PerlinNoiseProperty::H)
                                        - .05f);
                    break;

              /*  case GLFW_KEY_F:
                    m_perlinNoise->
                            setProperty(PerlinNoiseProperty::FREQUENCY,
                                        m_perlinNoise
                                                ->
                                                        getProperty(PerlinNoiseProperty::FREQUENCY)
                                        + 0.1f);
                    break;

                case GLFW_KEY_V:
                    m_perlinNoise->
                            setProperty(PerlinNoiseProperty::FREQUENCY,
                                        m_perlinNoise
                                                ->
                                                        getProperty(PerlinNoiseProperty::FREQUENCY)
                                        - 0.1f);
                    break;*/

                case GLFW_KEY_O:
                    m_perlinNoise->
                            setProperty(PerlinNoiseProperty::OFFSET,
                                        m_perlinNoise
                                                ->
                                                        getProperty(PerlinNoiseProperty::OFFSET)
                                        + 0.05);
                    break;

                    /*case GLFW_KEY_L:
                        m_perlinNoise->
                                setProperty(PerlinNoiseProperty::OFFSET,
                                            m_perlinNoise
                                                    ->
                                                            getProperty(PerlinNoiseProperty::OFFSET)
                                            - 0.05);
                        break;

                    case GLFW_KEY_I:
                        m_perlinNoise->
                                setProperty(PerlinNoiseProperty::LACUNARITY,
                                            m_perlinNoise
                                                    ->
                                                            getProperty(PerlinNoiseProperty::LACUNARITY)
                                            + 0.05f);
                        break;*/

                case GLFW_KEY_K:
                    m_perlinNoise->setProperty(PerlinNoiseProperty::LACUNARITY,
                                               m_perlinNoise->getProperty(PerlinNoiseProperty::LACUNARITY)
                                               - 0.05f);
                    break;

                case GLFW_KEY_X:
                    m_amplitude += 0.1f;
                    break;

                case GLFW_KEY_C:
                    m_amplitude -= 0.1f;
                    break;

                case GLFW_KEY_J:
                    m_perlinNoise->
                            setProperty(PerlinNoiseProperty::OCTAVE,
                                        m_perlinNoise
                                                ->
                                                        getProperty(PerlinNoiseProperty::OCTAVE)
                                        - 1);
                    break;

                case GLFW_KEY_U:
                    m_perlinNoise->setProperty(PerlinNoiseProperty::OCTAVE,
                                               m_perlinNoise->getProperty(PerlinNoiseProperty::OCTAVE) + 1);
                    break;

                case GLFW_KEY_G:
                    m_terrain->m_water_height += 0.05f;
                    break;

                case GLFW_KEY_B:
                    m_terrain->m_water_height -= 0.05f;
                    cout << "Water height = " << m_terrain->m_water_height << endl;
                    break;


                default:
                    break;
            }
        }
    }
};