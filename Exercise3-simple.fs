/******************************************************************************
 * This is a really simple fragment shader that simply sets the output fragment
 * color to yellow.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

in vec3 worldSpacePosition;
in vec3 viewSpacePosition;
in vec3 shaderNormal;
in vec2 shaderTexCoord;
in vec3 shaderLightPosition;

uniform sampler2D diffuseMap;
uniform bool isTile;
uniform float time;
uniform sampler2D shaderTextureSmoke;


out vec4 fragmentColor;

void main()
{
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    float ambientIntensity = 0.15f;
    float specularIntensity = 2.0f;
    float specularPower = 1024.0f;

    vec4 displacement = texture(shaderTextureSmoke, shaderTexCoord + vec2(time * 0.005, -time * 0.005));

    vec2 finalUV;
    if (isTile) {
        finalUV = (worldSpacePosition.xz * 0.2) + (displacement.rg - 0.5) + vec2(time*0.01, -time*0.01);
    } else {
        finalUV = shaderTexCoord;
    }

    // ambient
    vec3 lightAmbient = lightColor * ambientIntensity;
    
    // diffuse
    vec3 normalDir = normalize(shaderNormal);
    vec3 lightDir = normalize(shaderLightPosition - viewSpacePosition);
    vec3 viewDir = normalize(-viewSpacePosition);
    vec3 lightDiffuse = max(dot(normalDir, lightDir), 0.0f) * lightColor;

    // specular
    vec3 reflectDir = reflect(-lightDir, normalDir);
    float spec = pow(max(dot(reflectDir, viewDir), 0.0), specularPower);
    vec3 lightSpecular = spec * lightColor * specularIntensity;

    vec4 texColor = texture(diffuseMap, finalUV);
    fragmentColor = vec4((lightAmbient + lightDiffuse + lightSpecular), 1.0f) * texColor;
}