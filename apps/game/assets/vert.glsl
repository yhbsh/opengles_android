#version 300 es

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 rotationX;
uniform mat4 rotationY;
uniform mat4 rotationZ;

void main() {
    vColor = aColor;
    gl_Position = rotationY * rotationX * rotationZ * vec4(aPos, 1.0);
}
