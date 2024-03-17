
#version 330 core

uniform vec4 Color;

layout(location = 0) out vec4 color;

uniform float Target;
uniform vec4 BGCol;

#define A 0.1f

void main() {
  float x = gl_FragCoord.z;
  
  float val = A; 

  float f = Target * 2;

  if (x < f) {
    val = ((A - 1) / f) * x + 1;
  }
  
  val = clamp(val, A, 1);


  color = vec4((Color.xyz - BGCol.xyz) * val + BGCol.xyz, 1.f);
}