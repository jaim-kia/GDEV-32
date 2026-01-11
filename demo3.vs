/******************************************************************************
 * This is a vertex shader that passes the vertex color and texture
 * coordinates to the fragment shader, to facilitate texturing.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexColor;
layout (location = 2) in vec2 vertexTexCoord;
out vec3 shaderColor;
out vec2 shaderTexCoord;

void main()
{
    gl_Position = vec4(vertexPosition, 1.0f);
    shaderColor = vertexColor;
    shaderTexCoord = vertexTexCoord;
}
