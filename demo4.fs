/******************************************************************************
 * This fragment shader is exactly the same as in the previous demo. ;)
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

in vec3 shaderColor;
in vec2 shaderTexCoord;
uniform sampler2D shaderTexture;
out vec4 fragmentColor;

void main()
{
    fragmentColor = vec4(shaderColor, 1.0f) * texture(shaderTexture, shaderTexCoord);
}
