/******************************************************************************
 * This vertex shader transforms the position, normal, and texture coordinates
 * of a vertex into "camera space" (also known as model-view space).
 *
 * The (single) light position is also transformed to camera space in this
 * shader, so the fragment shader does not have to do it for each fragment.
 *
 * We can still make this shader faster by computing some of the parameters
 * from the C++ program instead of doing it here...
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexTexCoord;
uniform mat4 projectionTransform;
uniform mat4 viewTransform;
uniform mat4 modelTransform;
uniform vec3 lightPosition;
out vec3 shaderPosition;
out vec3 shaderNormal;
out vec2 shaderTexCoord;
out vec3 shaderLightPosition;

void main()
{
    // combine the model and view transforms to get the camera space transform
    mat4 modelViewTransform = viewTransform * modelTransform;

    // compute the vertex's attributes in camera space
    shaderPosition = vec3(modelViewTransform * vec4(vertexPosition, 1.0f));
    shaderTexCoord = vertexTexCoord;

    // compute the normal transform as the transpose of the inverse of the camera transform,
    // then transform the normal vector accordingly
    mat3 normalTransform = mat3(transpose(inverse(modelViewTransform)));
    shaderNormal = normalTransform * vertexNormal;

    // also compute the light position in camera space
    // (we want all lighting calculations to be done in camera space to avoid losing precision)
    shaderLightPosition = vec3(viewTransform * vec4(lightPosition, 1.0f));

    // we still need OpenGL to compute the final vertex position in projection space
    // to correctly determine where the fragments of the triangle actually go on the screen
    gl_Position = projectionTransform * vec4(shaderPosition, 1.0f);
}
