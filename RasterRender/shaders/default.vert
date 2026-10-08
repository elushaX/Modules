#version 450

layout(location = 0) in vec3 Point;

layout(push_constant) uniform PushConstants {
	mat4 Mvp;
	vec4 Offset;
} pc;

void main() {
	gl_Position = (pc.Mvp * vec4(Point.xyz, 1.0)) + pc.Offset;
}
