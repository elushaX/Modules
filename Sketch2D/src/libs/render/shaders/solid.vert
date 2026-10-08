#version 450

layout(push_constant) uniform PushConstants {
	vec4 rect;
	vec4 viewportAndRotation;
	vec4 color;
} pc;

void main() {
	vec2 corner = vec2((gl_VertexIndex & 1), (gl_VertexIndex >> 1));
	vec2 view = pc.rect.xy + corner * pc.rect.zw;
	vec2 rotation = pc.viewportAndRotation.zw;
	vec2 pixel = vec2(view.x * rotation.x - view.y * rotation.y, view.x * rotation.y + view.y * rotation.x);
	vec2 ndc = (pixel / pc.viewportAndRotation.xy) * 2.0 - 1.0;
	gl_Position = vec4(ndc, 0.0, 1.0);
}
