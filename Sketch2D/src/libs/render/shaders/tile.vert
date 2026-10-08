#version 450

layout(push_constant) uniform PushConstants {
	vec2 screenPos;
	vec2 screenSize;
	vec2 viewport;
	vec2 rotation;
} pc;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec2 viewPosition;

void main() {
	vec2 corner = vec2((gl_VertexIndex & 1), (gl_VertexIndex >> 1));
	uv = corner;

	vec2 view = pc.screenPos + corner * pc.screenSize;
	viewPosition = view;
	vec2 pixel = vec2(view.x * pc.rotation.x - view.y * pc.rotation.y, view.x * pc.rotation.y + view.y * pc.rotation.x);
	vec2 ndc = (pixel / pc.viewport) * 2.0 - 1.0;
	gl_Position = vec4(ndc, 0.0, 1.0);
}
