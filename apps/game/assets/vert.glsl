#version 300 es

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 transform;

void main() {
    vColor = aColor;
    gl_Position = transform * vec4(aPos, 0.0, 1.0);
}
