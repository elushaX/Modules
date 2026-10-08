#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 viewPosition;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D tileTex;

layout(push_constant) uniform PushConstants {
	layout(offset = 32) vec4 rows[4];
	vec4 clip;
} pc;

void main() {
	if (viewPosition.x < pc.clip.x || viewPosition.y < pc.clip.y || viewPosition.x > pc.clip.z || viewPosition.y > pc.clip.w)
		discard;
	vec4 texel = texture(tileTex, uv);
	color = vec4(dot(pc.rows[0], texel), dot(pc.rows[1], texel), dot(pc.rows[2], texel), dot(pc.rows[3], texel));
}
