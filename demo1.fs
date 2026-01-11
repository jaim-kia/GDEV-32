/******************************************************************************
 * This is a really simple fragment shader that simply sets the output fragment
 * color to yellow.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

out vec4 fragmentColor;

void main()
{
    fragmentColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}
