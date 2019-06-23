#pragma once

const char* SkyBoxLODVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 localPos;

void main()
{
    localPos = aPos;

    mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
    vec4 clipPos = projection * rotView * vec4(localPos, 1.0);

    // note gl_Position.z is 1.0
    // ensures the depth of the skybox is 1.0, the maximum depth value
    gl_Position = clipPos.xyww;
}
)";

const char* SkyBoxLODFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube environmentMap;

void main()
{
    vec3 envColor = textureLod(environmentMap, localPos, 0.0).rgb;

    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));

    FragColor = vec4(envColor, 1.0);
}
)";