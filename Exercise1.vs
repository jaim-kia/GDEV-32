/******************************************************************************
 * This is a really simple vertex shader that simply sets the output vertex's
 * position to be the same as the input.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 textureCoord;
layout (location = 2) in vec3 norm;
layout (location = 3) in vec3 vertexColor;
layout (location = 4) in vec3 aOffset;

out vec3 worldSpacePosition;
out vec2 shaderTexCoord;
out vec3 worldSpaceNorm;
out vec3 objectColor;

uniform mat4 projview;
uniform mat4 model;

void main()
{
    worldSpacePosition = (model * vec4(vertexPosition, 1.0f)).xyz + aOffset;

    gl_Position = projview * vec4(worldSpacePosition, 1.0f);

    shaderTexCoord = textureCoord;
    worldSpaceNorm = (model * vec4(norm, 0.0f)).xyz;
    objectColor = vertexColor;
}

