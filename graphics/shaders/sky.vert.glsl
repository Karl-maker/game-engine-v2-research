// Sky shader (fullscreen triangle)
#version 410 core

out vec2 v_Uv;

void main() {
  // Fullscreen triangle (no VBO).
  vec2 p;
  if (gl_VertexID == 0) p = vec2(-1.0, -1.0);
  else if (gl_VertexID == 1) p = vec2(3.0, -1.0);
  else p = vec2(-1.0, 3.0);

  v_Uv = p * 0.5 + 0.5;
  gl_Position = vec4(p, 0.0, 1.0);
}

