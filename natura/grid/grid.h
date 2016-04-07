#pragma once

#include "icg_helper.h"
#include <glm/gtc/type_ptr.hpp>
#include <cstdint>

class Grid {

private:
    GLuint vertex_array_id_;                // vertex array object
    GLuint vertex_buffer_object_position_;  // memory buffer for positions
    GLuint vertex_buffer_object_index_;     // memory buffer for indices
    GLuint program_id_;                     // GLSL shader program ID
    GLuint texture_id_;                     // texture ID
    GLuint num_indices_;                    // number of vertices to render
    GLuint MVP_id_;                         // model, view, proj matrix ID
    uint32_t mSideNbPoints;                 // grids side X nb of vertices;
    bool mCleanedUp;                        // check if the grid is cleaned before its destruction.

public:

    Grid(uint32_t sideSize){
        mSideNbPoints = sideSize;
        mCleanedUp = true;
    }

    ~Grid(){
        if (!mCleanedUp)
            Cleanup();
    }

    void Cleanup() {
        mCleanedUp = true;
        glBindVertexArray(0);
        glUseProgram(0);
        glDeleteBuffers(1, &vertex_buffer_object_position_);
        glDeleteBuffers(1, &vertex_buffer_object_index_);
        glDeleteVertexArrays(1, &vertex_array_id_);
        glDeleteProgram(program_id_);
        glDeleteTextures(1, &texture_id_);
    }

    void Init(int texture_) {
        texture_id_ = texture_;
        mCleanedUp = false; // Until the next Cleanup() call ...
        // compile the shaders.
        program_id_ = icg_helper::LoadShaders("grid_vshader.glsl",
                                              "grid_fshader.glsl");
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
            // always two subsequent entries in 'vertices' form a 2D vertex position.

            // the given code below are the vertices for a simple perlin_quad.
            // your grid should have the same dimension as that perlin_quad, i.e.,
            // reach from [-1, -1] to [1, 1].

            float sideX = 1 / float(mSideNbPoints);

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



        // load texture
        {
            //glGenTextures(1, &texture_id_);
            glBindTexture(GL_TEXTURE_2D, texture_id_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            GLuint tex_id_2 = glGetUniformLocation(program_id_, "tex");
            glUniform1i(tex_id_2, 0 /*GL_TEXTURE0*/);

            // cleanup
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // other uniforms
        MVP_id_ = glGetUniformLocation(program_id_, "MVP");

        // to avoid the current object being polluted
        glBindVertexArray(0);
        glUseProgram(0);
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

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id_);

        // draw
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLE_STRIP, num_indices_, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glUseProgram(0);
    }
};
