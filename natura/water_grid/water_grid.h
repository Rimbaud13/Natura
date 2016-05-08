#pragma once

#include "icg_helper.h"
#include <glm/gtc/type_ptr.hpp>

class WaterGrid {

private:
    GLuint vertex_array_id_;                // vertex array object
    GLuint vertex_buffer_object_position_;  // memory buffer for positions
    GLuint vertex_buffer_object_index_;     // memory buffer for indices
    GLuint program_id_;                     // GLSL shader program ID
    GLuint texture_id_;                     // texture ID
    GLuint num_indices_;                    // number of vertices to render
    GLuint MVP_id_;                         // model, view, proj matrix ID

public:
    void Init() {
        // compile the shaders.
        program_id_ = icg_helper::LoadShaders("water_grid_vshader.glsl",
                                              "water_grid_fshader.glsl");
        if (!program_id_) {
            exit(EXIT_FAILURE);
        }

        glUseProgram(program_id_);

        // vertex one vertex array
        glGenVertexArrays(1, &vertex_array_id_);
        glBindVertexArray(vertex_array_id_);

        // vertex coordinates and indices
        {
            std::vector<GLfloat> vertices;
            std::vector<GLuint> indices;
            int mSideNbPoints = 64;
            float sideX = 1 / float(mSideNbPoints);
            mSideNbPoints ++; // OFF BY ONE BY @Rimbaut
            for (int i = 0; i < mSideNbPoints; i++) {
                for (int j = 0; j < mSideNbPoints; j++) {
                    vertices.push_back(i * sideX);
                    vertices.push_back(j * sideX);
                }
            }

            for (unsigned int j = 0; j < mSideNbPoints - 1; j++) {
                if (j % 2 == 0) {
                    for (int i = 0; i < mSideNbPoints; i++) {
                        indices.push_back(mSideNbPoints * j + i);
                        indices.push_back(mSideNbPoints * j + i + mSideNbPoints);
                    }
                } else {
                    for (int i = mSideNbPoints - 1; i >= 0; i--) {
                        indices.push_back(mSideNbPoints * j + i);
                        indices.push_back(mSideNbPoints * j + i + mSideNbPoints);
                    }
                }
            }

            num_indices_ = indices.size();

            // position buffer
            glGenBuffers(1, &vertex_buffer_object_position_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object_position_);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                         &vertices[0], GL_STATIC_DRAW);

            // vertex indices
            glGenBuffers(1, &vertex_buffer_object_index_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer_object_index_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                         &indices[0], GL_STATIC_DRAW);

            // position shader attribute
            GLuint loc_position = glGetAttribLocation(program_id_, "position");
            glEnableVertexAttribArray(loc_position);
            glVertexAttribPointer(loc_position, 2, GL_FLOAT, DONT_NORMALIZE,
                                  ZERO_STRIDE, ZERO_BUFFER_OFFSET);
        }

        // create 1D texture (colormap)
        {
            const int ColormapSize = 2;
            GLfloat tex[3 * ColormapSize] = {0.0, 0.2, 0.45, 158.0f/255.0f, 181.0f/255.0f, 210.0f/255.0f};
            glGenTextures(1, &texture_id_);
            glBindTexture(GL_TEXTURE_1D, texture_id_);
            glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, ColormapSize, 0, GL_RGB, GL_FLOAT, tex);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GLuint tex_id = glGetUniformLocation(program_id_, "colormap");
            glUniform1i(tex_id, 0 /*GL_TEXTURE0*/);
            // check_error_gl();
        }

        // other uniforms
        MVP_id_ = glGetUniformLocation(program_id_, "MVP");

        // to avoid the current object being polluted
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Cleanup() {
        glBindVertexArray(0);
        glUseProgram(0);
        glDeleteBuffers(1, &vertex_buffer_object_position_);
        glDeleteBuffers(1, &vertex_buffer_object_index_);
        glDeleteVertexArrays(1, &vertex_array_id_);
        glDeleteProgram(program_id_);
        glDeleteTextures(1, &texture_id_);
    }

    void Draw(float time, const glm::mat4 &model = IDENTITY_MATRIX,
              const glm::mat4 &view = IDENTITY_MATRIX,
              const glm::mat4 &projection = IDENTITY_MATRIX) {
        glUseProgram(program_id_);
        glBindVertexArray(vertex_array_id_);

        // setup MVP
        glm::mat4 MVP = projection * view * model;
        glUniformMatrix4fv(MVP_id_, ONE, DONT_TRANSPOSE, glm::value_ptr(MVP));

        // pass the current time stamp to the shader.
        glUniform1f(glGetUniformLocation(program_id_, "time"), time);

        // draw
        // TODO 5: for debugging it can be helpful to draw only the wireframe.
        // You can do that by uncommenting the next line.
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        // TODO 5: depending on how you set up your vertex index buffer, you
        // might have to change GL_TRIANGLE_STRIP to GL_TRIANGLES.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLE_STRIP, num_indices_, GL_UNSIGNED_INT, 0);
        //glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
        glUseProgram(0);
    }
};
