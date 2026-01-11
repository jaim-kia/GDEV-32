/******************************************************************************
 * This is a vertex shader that improves the previous demo's shader by
 * supporting transformation matrices: projection, view, and model transform.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexColor;
layout (location = 2) in vec2 vertexTexCoord;
uniform mat4 projectionTransform;
uniform mat4 viewTransform;
uniform mat4 modelTransform;
out vec3 shaderColor;
out vec2 shaderTexCoord;

void main()
{
    gl_Position = projectionTransform * viewTransform * modelTransform * vec4(vertexPosition, 1.0f);
    shaderColor = vertexColor;
    shaderTexCoord = vertexTexCoord;
}
