#version 460 core

uniform mat4 u_viewProj;
uniform mat4 u_model;

out VS_OUT
{
  vec3 vPosition;
  vec3 vNormal;
  vec2 vTexcoord;
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
  uint triangleIndex = gl_VertexID / 6;
  uint vertexIndex = gl_VertexID % 6;
  vec3 aNormal = vec3(0, 1, 0);
  vec2 aTexCoord = tex_corners[indices[vertexIndex]];
  vec3 aPosition = vec3(aTexCoord.x - .5, 0, aTexCoord.y - .5);

  vs_out.vPosition = (u_model * vec4(aPosition, 1.0)).xyz;
  vs_out.vNormal = (u_model * vec4(aNormal, 0.0)).xyz;
  vs_out.vTexcoord = aTexCoord;

  gl_Position = u_viewProj * vec4(vs_out.vPosition, 1.0);
}