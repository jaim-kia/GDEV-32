/******************************************************************************
 * This vertex shader facilitates drawing a model from the point of view of a
 * light source, for the purpose of shadow mapping.
 *
 * Since we don't need any colors or texturing in a shadow map, only the vertex
 * positions are used by this shader.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 4) in mat4 instanceMatrix;

uniform mat4 lightTransform;
uniform mat4 modelTransform;
uniform bool isInstanced;

void main()
{
    mat4 finalModel = isInstanced ? instanceMatrix : modelTransform;
    gl_Position = lightTransform * finalModel * vec4(vertexPosition, 1.0f);
}
