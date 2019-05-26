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