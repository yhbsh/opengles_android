#version 300 es

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

uniform float angleX;
uniform float angleY;
uniform float angleZ;

void main() {
    float cosX = cos(angleX);
    float sinX = sin(angleX);
    float cosY = cos(angleY);
    float sinY = sin(angleY);
    float cosZ = cos(angleZ);
    float sinZ = sin(angleZ);

    mat4 rotationX = mat4(
        +1.0, +0.0 , +0.0 , +0.0,
        +0.0, +cosX, -sinX, +0.0,
        +0.0, +sinX, +cosX, +0.0,
        +0.0, +0.0 , +0.0 , +1.0
    );

    mat4 rotationY = mat4(
        +cosY, +0.0, +sinY, +0.0,
        +0.0 , +1.0, +0.0 , +0.0,
        -sinY, +0.0, +cosY, +0.0,
        +0.0 , +0.0, +0.0 , +1.0
    );

    mat4 rotationZ = mat4(
        +cosZ, -sinZ, +0.0, +0.0,
        +sinZ, +cosZ, +0.0, +0.0,
        +0.0 , +0.0 , +1.0, +0.0,
        +0.0 , +0.0 , +0.0, +1.0
    );

    mat4 rotation = rotationZ * rotationY * rotationX;

    vColor = aColor;
    gl_Position = rotation * vec4(aPos, 1.0);
}
