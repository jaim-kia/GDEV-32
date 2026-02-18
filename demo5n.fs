#version 330 core

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    float innerCutoff;
    float outerCutoff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 shaderPosition;
in mat3 shaderTBN;
in vec2 shaderTexCoord;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

uniform DirLight dir_light;
uniform SpotLight spotlights[2];

out vec4 fragmentColor;

vec3 CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir){
    vec3 lightDir = (-light.direction); // for directional light, the light direction is the opposite of the light's direction vector
    
    // ambient
    // vec3 ambient = light.ambient * vec3(texture(diffuseMap, shaderTexCoord));

    // diffuse 
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseMap, shaderTexCoord));

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f); // place specular exponent and intensity here
    vec3 specular = light.specular * spec;

    return (diffuse + specular);
} 

vec3 CalculateSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 fragPos) {
    vec3 lightDir = normalize(light.position - fragPos);

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance)); 

    // soft edge
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0f, 1.0f);

    // ambient
    // vec3 ambient = light.ambient * vec3(texture(diffuseMap, shaderTexCoord));

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseMap, shaderTexCoord));

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f); // place specular exponent and intensity here
    vec3 specular = light.specular * spec;
    // vec3 specular = vec3(0.0f); 
    return (diffuse + specular) * intensity * attenuation;
}

void main()
{
    // define some constant properties for the light
    // (you should really be passing these parameters into the shader as uniform vars instead)
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);  // diffuse
    float ambientIntensity = 0.15f;            // ambient
    float specularIntensity = 2.0f;            // specular (better implementation: look this up from a specular map!)
    float specularPower = 1024.0f;               // specular exponent

    // look up the normal from the normal map, then reorient it with the current model transform via the TBN matrix
    vec3 textureNormal = vec3(texture(normalMap, shaderTexCoord));
    textureNormal = normalize(textureNormal * 2.0f - 1.0f);  // convert range from [0, 1] to [-1, 1]
    vec3 normalDir = normalize(shaderTBN * textureNormal);

    vec3 viewDir = normalize(-shaderPosition);

    vec3 result = vec3(0.0f);

    // average of ambient light of all light sources
    vec3 ambient = vec3(0.0f);
    ambient += dir_light.ambient;
    for (int i = 0; i < 2; i++) {
        ambient += spotlights[i].ambient;
    }
    ambient /= 3.0f; // average the ambient light contributions
    
    // directional light
    result += CalculateDirLight(dir_light, normalDir, viewDir);

    // calculate specular
    vec3 viewDir = normalize(-shaderPosition);
    vec3 reflectDir = reflect(-lightDir, normalDir);

    vec3 textureSpecular = vec3(texture(specularMap, shaderTexCoord));
    vec3 lightSpecular = pow(max(dot(reflectDir, viewDir), 0), specularPower) * lightColor * specularIntensity * textureSpecular;
    // spotlights
    for (int i = 0; i < 2; i++) {
        result += CalculateSpotLight(spotlights[i], normalDir, viewDir, shaderPosition);
    }

    fragmentColor = vec4(result, 1.0f);

}
