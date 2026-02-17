/******************************************************************************
 * This is a really simple fragment shader that simply sets the output fragment
 * color to yellow.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

in vec3 shaderPosition;
in vec3 shaderNormal;
in vec2 shaderTexCoord;
in vec3 shaderLightPosition;

uniform sampler2D diffuseMap;

out vec4 fragmentColor;

void main()
{
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    float ambientIntensity = 0.15f;
    float specularIntensity = 2.0f;
    float specularPower = 1024.0f;
  	
    // ambient
    vec3 lightAmbient = lightColor * ambientIntensity;

    // diffuse
    vec3 normalDir = normalize(shaderNormal);
    vec3 lightDir = normalize(shaderLightPosition - shaderPosition);
    vec3 lightDiffuse = max(dot(normalDir, lightDir), 0.0f) * lightColor;

    // specular
    vec3 viewDir = normalize(-shaderPosition);
    vec3 reflectDir = reflect(-lightDir, normalDir);

    float spec = pow(max(dot(reflectDir, viewDir), 0.0), specularPower);
    vec3 lightSpecular = spec * lightColor * specularIntensity;

    vec4 texColor = texture(diffuseMap, shaderTexCoord);
    fragmentColor = vec4((lightAmbient + lightDiffuse + lightSpecular), 1.0f) * texColor;
}