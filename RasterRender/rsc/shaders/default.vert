#version 330 core

layout(location = 0) in vec3 Point;

uniform vec4 Origin;
uniform mat4 Basis;
uniform mat4 Camera;

void main() {
    // vec4 transformed = vec4(Point.xyz, 1.0);
    vec4 transformed = (Basis * vec4(Point.xyz, 1.0)) + Origin;
    gl_Position = Camera * transformed;
}