#version 330 core

layout(location = 0) in vec4 point;

out float thickness;

uniform mat4 MVP;

void main() {
	thickness = point.w;
	// gl_Position = MVP * vec4(point.xyz, 1);
	
	vec4 vec = MVP * vec4(point.xyz, 1);
	vec.z *= vec.w;
	gl_Position = vec;
}