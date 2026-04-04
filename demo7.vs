/******************************************************************************
 * This vertex shader facilitates drawing a fullscreen quadrilateral
 * (using the custom framebuffer texture) onto the onscreen framebuffer.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
out vec2 shaderTexCoord;

void main()
{
    shaderTexCoord = vertexTexCoord;
    gl_Position = vec4(vertexPosition, 0.0f, 1.0f);
}
