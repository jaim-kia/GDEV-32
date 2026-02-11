/******************************************************************************
 * This fragment shader implements the Phong lighting formula for a single
 * light source.
 *
 * Many of the light's parameters are hardcoded for now -- you should change
 * this in your own programs for improved control over lighting.
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
    // define some constant properties for the light
    // (you should really be passing these parameters into the shader as uniform vars instead)
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);  // diffuse
    float ambientIntensity = 0.15f;            // ambient
    float specularIntensity = 0.5f;            // specular (better implementation: look this up from a specular map!)
    float specularPower = 32.0f;               // specular exponent

    // normalize the interpolated normal from the vertex shader
    // (lighting calculations will break if the normal direction is not a unit vector)
    vec3 normalDir = normalize(shaderNormal);

    // calculate ambient
    vec3 lightAmbient = lightColor * ambientIntensity;

    // calculate diffuse
    vec3 lightDir = normalize(shaderLightPosition - shaderPosition);
    vec3 lightDiffuse = max(dot(normalDir, lightDir), 0.0f) * lightColor;

    // calculate specular
    vec3 viewDir = normalize(-shaderPosition);
    vec3 reflectDir = reflect(-lightDir, normalDir);
    vec3 lightSpecular = pow(max(dot(reflectDir, viewDir), 0), specularPower) * lightColor * specularIntensity;

    // compute final fragment color
    fragmentColor = vec4((lightAmbient + lightDiffuse + lightSpecular), 1.0f) * texture(diffuseMap, shaderTexCoord);
}
