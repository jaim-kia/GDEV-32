/******************************************************************************
 * This vertex shader improves on the previous one by taking an additional
 * vertex attribute (vertexTangent), computing a TBN matrix from vertexTangent
 * and vertexNormal, and passes this TBN matrix (instead of the normal) to the
 * fragment shader, to facilitate normal mapping.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

layout (location = 0) in vec3 aPos;       
layout (location = 1) in vec2 aTexCoord;  
layout (location = 2) in vec3 aNormal;
layout (location = 4) in mat4 instanceMatrix;  

out vec2 shaderTexCoord;
out vec3 worldSpacePosition; 
out vec3 viewSpacePosition;     
out vec3 shaderNormal;        
out vec3 shaderLightPosition; 

uniform mat4 projectionTransform;
uniform mat4 viewTransform;
uniform mat4 modelTransform;
uniform vec3 lightPosition;
uniform bool isInstanced;

void main()
{
    mat4 finalModel = isInstanced ? instanceMatrix : modelTransform;

    vec4 worldPos = finalModel * vec4(aPos, 1.0);
    worldSpacePosition = worldPos.xyz;
    
    vec4 viewSpacePos = viewTransform * worldPos;
    viewSpacePosition = viewSpacePos.xyz;

    mat3 normalMatrix = mat3(transpose(inverse(viewTransform * finalModel)));
    shaderNormal = normalize(normalMatrix * aNormal);
    shaderLightPosition = (viewTransform * vec4(lightPosition, 1.0)).xyz;

    shaderTexCoord = aTexCoord;
    gl_Position = projectionTransform * viewSpacePos;
}