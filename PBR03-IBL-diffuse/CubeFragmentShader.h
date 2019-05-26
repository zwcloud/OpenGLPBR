#pragma once

//fragment shader used when rendering equirectangular map to a unit cube
//see https://stackoverflow.com/q/48494389/3427520 for the theory of SampleSphericalMap
const char* cubeFragmentShader= R"(
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
    vec3 color = texture(equirectangularMap, vec2(uv.s, 1.0f-uv.t)).rgb;

    FragColor = vec4(color, 1.0);
}
)";