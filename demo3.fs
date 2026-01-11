/******************************************************************************
 * This is a fragment shader that looks up a texture pixel (texel) from the
 * texture using the coordinates passed from the vertex shader, then blends
 * that with the (interpolated) vertex color.
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
