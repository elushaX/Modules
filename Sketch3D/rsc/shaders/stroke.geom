#version 330 core

#define PI 3.14

layout (lines) in;
layout (triangle_strip, max_vertices = 16) out;

in float thickness[];

uniform float Ratio;

void main2() {
  
  float r = sqrt(Ratio);
  vec4 l1 = gl_in[0].gl_Position;
  vec4 l2 = gl_in[1].gl_Position;

  vec2 l1p = l1.xy / l1.w;
  vec2 l2p = l2.xy / l2.w;

  float t1 = thickness[0] / l1.w;
  float t2 = thickness[1] / l2.w;

  vec2 l = normalize(vec2(l2p.x / r, l2p.y * r) - vec2(l1p.x / r, l1p.y * r));
  vec2 tmp = vec2(-l.y, l.x);

  vec2 tmp_poj = vec2(tmp.x * r, tmp.y / r);
  vec2 l_poj = vec2(l.x * r, l.y / r);

  vec2 p1 = l1p.xy + (tmp_poj * t1);
  vec2 p2 = l1p.xy - (tmp_poj * t1);
  vec2 p3 = l2p.xy + (tmp_poj * t2);
  vec2 p4 = l2p.xy - (tmp_poj * t2);

  gl_Position = vec4(p1.x, p1.y , l1.z, 1);
  EmitVertex();
  gl_Position = vec4(p2.x, p2.y, l1.z, 1);
  EmitVertex();

  gl_Position = vec4(p3.x, p3.y, l2.z, 1);
  EmitVertex();
  gl_Position = vec4(p4.x, p4.y, l2.z, 1);
  EmitVertex();
  EndPrimitive();


  p1 = l1p.xy + (tmp_poj * t1);
  p2 = l1p.xy - (tmp_poj * t1);
  p3 = l1p.xy + (tmp_poj * t1) - (l_poj * t1) / 2.f;
  p4 = l1p.xy - (tmp_poj * t1) - (l_poj * t1) / 2.f;
  vec2 p5 = l1p.xy + (tmp_poj * t1) / 2.f - (l_poj * t1);
  vec2 p6 = l1p.xy - (tmp_poj * t1) / 2.f - (l_poj * t1);

  gl_Position = vec4(p1.x, p1.y, l1.z, 1);
  EmitVertex();
  gl_Position = vec4(p2.x, p2.y, l1.z, 1);
  EmitVertex();
  gl_Position = vec4(p3.x, p3.y, l1.z, 1);
  EmitVertex();
  gl_Position = vec4(p4.x, p4.y, l1.z, 1);
  EmitVertex();
  gl_Position = vec4(p5.x, p5.y, l1.z, 1);
  EmitVertex();
  gl_Position = vec4(p6.x, p6.y, l1.z, 1);
  EmitVertex();

  EndPrimitive();

  p1 = l2p.xy + (tmp_poj * t2);
  p2 = l2p.xy - (tmp_poj * t2);
  p3 = l2p.xy + (tmp_poj * t2) + (l_poj * t2) / 2.f;
  p4 = l2p.xy - (tmp_poj * t2) + (l_poj * t2) / 2.f;
  p5 = l2p.xy + (tmp_poj * t2) / 2.f + (l_poj * t2);
  p6 = l2p.xy - (tmp_poj * t2) / 2.f + (l_poj * t2);

  gl_Position = vec4(p1.x, p1.y, l2.z, 1);
  EmitVertex();
  gl_Position = vec4(p2.x, p2.y, l2.z, 1);
  EmitVertex();
  gl_Position = vec4(p3.x, p3.y, l2.z, 1);
  EmitVertex();
  gl_Position = vec4(p4.x, p4.y, l2.z, 1);
  EmitVertex();
  gl_Position = vec4(p5.x, p5.y, l2.z, 1);
  EmitVertex();
  gl_Position = vec4(p6.x, p6.y, l2.z, 1);
  EmitVertex();

  EndPrimitive();
}

void main() {
  
  float r = sqrt(Ratio);
  vec4 l1 = gl_in[0].gl_Position;
  vec4 l2 = gl_in[1].gl_Position;

  vec2 l1p = l1.xy / l1.w;
  vec2 l2p = l2.xy / l2.w;

  float t1 = thickness[0] / l1.w;
  float t2 = thickness[1] / l2.w;

  vec2 l = normalize(vec2(l2p.x / r, l2p.y * r) - vec2(l1p.x / r, l1p.y * r));
  vec2 tmp = vec2(-l.y, l.x);

  vec2 tmp_poj = vec2(tmp.x * r, tmp.y / r);
  vec2 l_poj = vec2(l.x * r, l.y / r);

  vec2 p1 = l1p.xy + (tmp_poj * t1);
  vec2 p2 = l1p.xy - (tmp_poj * t1);
  vec2 p3 = l2p.xy + (tmp_poj * t2);
  vec2 p4 = l2p.xy - (tmp_poj * t2);

  gl_Position = vec4(p1.x * l1.w, p1.y * l1.w, l1.z, l1.w);
  EmitVertex();
  gl_Position = vec4(p2.x * l1.w, p2.y * l1.w, l1.z, l1.w);
  EmitVertex();

  gl_Position = vec4(p3.x * l2.w, p3.y * l2.w, l2.z, l2.w);
  EmitVertex();
  gl_Position = vec4(p4.x * l2.w, p4.y * l2.w, l2.z, l2.w);
  EmitVertex();
  EndPrimitive();


  p1 = l1p.xy + (tmp_poj * t1);
  p2 = l1p.xy - (tmp_poj * t1);
  p3 = l1p.xy + (tmp_poj * t1) - (l_poj * t1) / 2.f;
  p4 = l1p.xy - (tmp_poj * t1) - (l_poj * t1) / 2.f;
  vec2 p5 = l1p.xy + (tmp_poj * t1) / 2.f - (l_poj * t1);
  vec2 p6 = l1p.xy - (tmp_poj * t1) / 2.f - (l_poj * t1);

  gl_Position = vec4(p1.x * l1.w, p1.y * l1.w, l1.z, 1 * l1.w);
  EmitVertex();
  gl_Position = vec4(p2.x * l1.w, p2.y * l1.w, l1.z, 1 * l1.w);
  EmitVertex();
  gl_Position = vec4(p3.x * l1.w, p3.y * l1.w, l1.z, 1 * l1.w);
  EmitVertex();
  gl_Position = vec4(p4.x * l1.w, p4.y * l1.w, l1.z, 1 * l1.w);
  EmitVertex();
  gl_Position = vec4(p5.x * l1.w, p5.y * l1.w, l1.z, 1 * l1.w);
  EmitVertex();
  gl_Position = vec4(p6.x * l1.w, p6.y * l1.w, l1.z, 1 * l1.w);
  EmitVertex();

  EndPrimitive();

  p1 = l2p.xy + (tmp_poj * t2);
  p2 = l2p.xy - (tmp_poj * t2);
  p3 = l2p.xy + (tmp_poj * t2) + (l_poj * t2) / 2.f;
  p4 = l2p.xy - (tmp_poj * t2) + (l_poj * t2) / 2.f;
  p5 = l2p.xy + (tmp_poj * t2) / 2.f + (l_poj * t2);
  p6 = l2p.xy - (tmp_poj * t2) / 2.f + (l_poj * t2);

  gl_Position = vec4(p1.x * l2.w, p1.y * l2.w, l2.z, 1 * l2.w);
  EmitVertex();
  gl_Position = vec4(p2.x * l2.w, p2.y * l2.w, l2.z, 1 * l2.w);
  EmitVertex();
  gl_Position = vec4(p3.x * l2.w, p3.y * l2.w, l2.z, 1 * l2.w);
  EmitVertex();
  gl_Position = vec4(p4.x * l2.w, p4.y * l2.w, l2.z, 1 * l2.w);
  EmitVertex();
  gl_Position = vec4(p5.x * l2.w, p5.y * l2.w, l2.z, 1 * l2.w);
  EmitVertex();
  gl_Position = vec4(p6.x * l2.w, p6.y * l2.w, l2.z, 1 * l2.w);
  EmitVertex();

  EndPrimitive();
}
