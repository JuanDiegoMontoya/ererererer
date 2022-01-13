#version 460 core

in VS_OUT
{
  vec3 position;
  vec3 normal;
  vec2 uv;
}fs_in;

out vec4 fragColor;

void main()
{
  fragColor = vec4(fs_in.uv, 0, 1);
}