#version 300 es

layout(location = 0) in vec2 vPosition;
uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(vPosition, 0.0, 1.0);
}
