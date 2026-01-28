/******************************************************************************
 * This is a really simple vertex shader that simply sets the output vertex's
 * position to be the same as the input.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec3 vertexPos;
out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = vertexPos;
    mat4 staticView = mat4(mat3(view)); 
    vec4 pos = projection * staticView * vec4(vertexPos, 1.0);

    gl_Position = pos.xyww;
}