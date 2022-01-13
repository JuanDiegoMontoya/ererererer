#version 460 core

uniform mat4 u_viewProj;
uniform mat4 u_model;
uniform uint u_width;
uniform uint u_height;

layout(binding = 0) uniform sampler2D s_heightmap;

out VS_OUT
{
  vec3 position;
  vec3 normal;
  vec2 uv;
}vs_out;

// counterclockwise from bottom right texture coordinates
const vec2 tex_corners[] =
{
  { 1, 0 },
  { 1, 1 },
  { 0, 1 },
  { 0, 0 },
};
const uint indices[6] = { 3, 1, 0, 2, 1, 3 };

void main()
{
  uint quadIndex = gl_VertexID / 6;
  uint vertexIndex = gl_VertexID % 6;
  vec2 trianglePos = vec2(quadIndex % u_width, quadIndex / u_width);

  vec3 aPosition = vec3(trianglePos.x, 0, trianglePos.y);
  aPosition.xz += tex_corners[indices[vertexIndex]];
  aPosition.xz /= vec2(u_width, u_height);
  aPosition.xz -= 0.5;

  vec3 aNormal = vec3(0, 1, 0);
  vec2 uv = tex_corners[indices[vertexIndex]] + trianglePos;
  uv /= vec2(u_width, u_height);

  aPosition.y = textureLod(s_heightmap, uv, 0).r;
  //aPosition.y = uv.y;

  vs_out.position = (u_model * vec4(aPosition, 1.0)).xyz;
  vs_out.normal = normalize((u_model * vec4(aNormal, 0.0)).xyz);
  vs_out.uv = uv;

  gl_Position = u_viewProj * vec4(vs_out.position, 1.0);
}