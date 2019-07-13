#pragma once

//vertex shader used when rendering equirectangular map to a unit cube
const char* cubeVertexShader= R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    localPos = aPos;
    gl_Position =  projection * view * vec4(localPos, 1.0);
}
)";

//vertex shader used when rendering equirectangular map to a unit cube
const char* cubeFragmentShader= R"(
#version 330 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube cubeMap;

void main()
{
    vec3 envColor = texture(cubeMap, localPos).rgb;

    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));

    FragColor = vec4(envColor, 1.0);
}
)";

//fragment shader used when rendering equirectangular map to a unit cube
//see https://stackoverflow.com/q/48494389/3427520 for the theory of SampleSphericalMap
const char* equirectangularToCubemapFragmentShader= R"(
#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(localPos)); // make sure to normalize localPos
    uv.t = 1.0 - uv.t; //HDRLoader decoded pixels is upside-down for OpenGL
    vec3 color = texture(equirectangularMap, uv).rgb;

    FragColor = vec4(color, 1.0);
}
)";