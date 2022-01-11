#version 460 core

uniform mat4 u_invViewProj;
uniform vec3 u_viewPos;
uniform vec3 u_sunDir;
uniform float u_blendDay;

in vec2 vTexcoord;

out vec4 fragColor;

vec3 Unproject(vec2 uv, mat4 invXProj)
{
  vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, 1.0, 1.0); // [0, 1] -> [-1, 1]

  // undo projection
  vec4 worldSpacePosition = invXProj * clipSpacePosition;
  worldSpacePosition /= worldSpacePosition.w;

  return worldSpacePosition.xyz;
}

void main()
{
    vec3 dir = normalize(Unproject(vTexcoord, u_invViewProj) - u_viewPos);
    vec3 skyDay = vec3(9.0 / 255, 118.0 / 255, 148.0 / 255);
    vec3 skyNight = vec3(7.0 / 255, 8.0 / 255, 15.0 / 255);
    vec3 skyLow = vec3(145.0 / 255, 69.0 / 255, 41.0 / 255);
    vec3 skyHigh = mix(skyNight, skyDay, u_blendDay); // no reference intended
    fragColor.rgb = mix(skyLow, skyHigh, smoothstep(0.0, 0.2, dir.y));


    float sun = dot(u_sunDir, -dir);
    if (sun > .995)
    {
        vec3 sunColor = vec3(255.0 / 255, 253.0 / 255, 163.0 / 255);
        if (sun > .999) sun = 1.0;
        float blendSun = smoothstep(0.995, 1.0, sun);
        fragColor.rgb = vec3(mix(fragColor.rgb, sunColor, blendSun));
    }

    fragColor.rgb = mix(vec3(0.0), fragColor.rgb, smoothstep(0.0, 0.03, dir.y));

    fragColor.a = 1.0;
}