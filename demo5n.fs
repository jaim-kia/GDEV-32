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

uniform DirLight dir_light;
uniform SpotLight spotlights[2];

out vec4 fragmentColor;

vec3 CalculateDirLight(DirLight light, vec3 normal, vec3 viewDir){
    vec3 lightDir = (-light.direction); // for directional light, the light direction is the opposite of the light's direction vector
    
    // ambient
    vec3 ambient = light.ambient * vec3(texture(diffuseMap, shaderTexCoord));

    // diffuse 
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseMap, shaderTexCoord));

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f); // place specular exponent and intensity here
    vec3 specular = light.specular * spec;

    return (ambient + diffuse + specular);
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
    vec3 ambient = light.ambient * vec3(texture(diffuseMap, shaderTexCoord));

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseMap, shaderTexCoord));

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f); // place specular exponent and intensity here
    vec3 specular = light.specular * spec;
    // vec3 specular = vec3(0.0f); 
    return (ambient + diffuse + specular) * intensity * attenuation;
}

void main()
{
    // look up the normal from the normal map, then reorient it with the current model transform via the TBN matrix
    vec3 textureNormal = vec3(texture(normalMap, shaderTexCoord));
    textureNormal = normalize(textureNormal * 2.0f - 1.0f);  // convert range from [0, 1] to [-1, 1]
    vec3 normalDir = normalize(shaderTBN * textureNormal);

    vec3 viewDir = normalize(-shaderPosition);

    vec3 result = vec3(0.0f);

    // directional light
    result += CalculateDirLight(dir_light, normalDir, viewDir);

    // spotlights
    for (int i = 0; i < 2; i++) {
        result += CalculateSpotLight(spotlights[i], normalDir, viewDir, shaderPosition);
    }

    fragmentColor = vec4(result, 1.0f);

}
