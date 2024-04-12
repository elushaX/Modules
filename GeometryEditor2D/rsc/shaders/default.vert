#version 330 core

layout(location = 0) in vec2 point;

uniform vec4 Viewport;

void main() {
    vec2 transformedPoint = point;

    transformedPoint.x = transformedPoint.x - Viewport.x;
    transformedPoint.y = transformedPoint.y - Viewport.y;

    transformedPoint.x = transformedPoint.x / Viewport.z;
    transformedPoint.y = transformedPoint.y / Viewport.w;

    transformedPoint.y = -transformedPoint.y;

    transformedPoint.y = transformedPoint.y * 2;
    transformedPoint.x = transformedPoint.x * 2;

	gl_Position = vec4(transformedPoint.x - 1, transformedPoint.y + 1, 1.0, 1.0);
}