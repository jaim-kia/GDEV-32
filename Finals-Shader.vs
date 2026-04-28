#version 330 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec3 vertexNormal;
layout (location = 3) in vec3 vertexTangent;
layout (location = 4) in mat4 instanceMatrix;  

// uniform int numLights;

uniform mat4 projectionTransform;
uniform mat4 viewTransform;
uniform mat4 modelTransform;
// uniform mat4 lightTransforms[MAX_LIGHTS];
uniform bool isInstanced;
uniform vec4 clipPlane;

uniform mat4 directionalLightTransforms[1];
uniform mat4 spotLightTransforms[2];

out vec3 shaderPosition;
out mat3 shaderTBN;
out vec2 shaderTexCoord;
out vec4 dirLightSpacePositions[1];
out vec4 spotLightSpacePositions[2];
out vec3 worldSpacePosition;

void main()
{
    // getting final Model
    mat4 finalModel = isInstanced ? instanceMatrix : modelTransform;

    // combine the model and view transforms to get the camera space transform
    mat4 modelViewTransform = viewTransform * finalModel;

    // compute the vertex's attributes in camera space
    shaderPosition = vec3(modelViewTransform * vec4(vertexPosition, 1.0f));
    shaderTexCoord = vertexTexCoord;
    vec4 worldPos = finalModel * vec4(vertexPosition, 1.0f);
    worldSpacePosition = worldPos.xyz;

    // compute the normal transform as the transpose of the inverse of the camera transform,
    // then compute a TBN matrix using this transform
    mat3 normalTransform = mat3(transpose(inverse(modelViewTransform)));
    vec3 normal = normalize(normalTransform * vertexNormal);
    vec3 tangent = normalize(normalTransform * vertexTangent);
    vec3 bitangent = cross(normal, tangent);
    shaderTBN = mat3(tangent, bitangent, normal);

    // also compute the light position in camera space
    // (we want all lighting calculations to be done in camera space to avoid losing precision)
    // shaderLightPosition = vec3(viewTransform * vec4(lightPosition, 1.0f));

    // we still need OpenGL to compute the final vertex position in projection space
    // to correctly determine where the fragments of the triangle actually go on the screen
    gl_Position = projectionTransform * vec4(shaderPosition, 1.0f);

    for (int i = 0; i < 1; i++) {
        dirLightSpacePositions[i] = directionalLightTransforms[i] * finalModel * vec4(vertexPosition, 1.0f);
    }

    for (int i = 0; i < 2; i++) {
        spotLightSpacePositions[i] = spotLightTransforms[i] * finalModel * vec4(vertexPosition, 1.0f);
    }

    gl_ClipDistance[0] = dot(worldPos, clipPlane);
}
