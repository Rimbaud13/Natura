#version 330 core

in vec3 vpoint;

out vec3 uv;

uniform mat4 MVP;
uniform float H;

void main() {
    uv = vec3(MVP * vec4(vpoint * 4.0f, 1.0));
    gl_Position = MVP * vec4(vpoint, 1.0);
}
