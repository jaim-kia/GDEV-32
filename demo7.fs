/******************************************************************************
 * This fragment shader inverts the colors of the framebuffer.
 *
 * Modify this file to experiment with different post-processing effects!
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

in vec2 shaderTexCoord;
uniform sampler2D screenTexture;
out vec4 fragmentColor;

void main()
{
    vec4 color = texture(screenTexture,
                         shaderTexCoord);
    fragmentColor = 1.0f - color;
}
