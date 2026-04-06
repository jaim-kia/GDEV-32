#version 330 core

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 color;
    float specular_exponent;
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
    vec3 color;
    float specular_exponent;
};

in vec3 shaderPosition;
in mat3 shaderTBN;
in vec2 shaderTexCoord;
in vec4 shaderLightSpacePositions[2];

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;
uniform sampler2D shadowMaps[2];

uniform bool enableShadows;

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
    vec3 textureSpecular = vec3(texture(specularMap, shaderTexCoord));
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(reflectDir, viewDir), 0), light.specular_exponent);
    vec3 specular = light.specular * spec * light.color * textureSpecular;

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

    // specular shading
    vec3 textureSpecular = vec3(texture(specularMap, shaderTexCoord));
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(reflectDir, viewDir), 0), light.specular_exponent);
    vec3 specular = light.specular * spec * light.color * textureSpecular;

    return (diffuse + specular) * intensity * attenuation;
}

bool inShadowSpotlight(int index)
{
    // perform perspective division and rescale to the [0, 1] range to get the coordinates into the depth texture
    vec3 position = shaderLightSpacePositions[index].xyz / shaderLightSpacePositions[index].w;
    position = position * 0.5f + 0.5f;

    // if the position is outside the light-space frustum, do NOT put the
    // fragment in shadow, to prevent the scene from becoming dark "by default"
    // (note that if you have a spot light, you might want to do the opposite --
    // that is, everything outside the spot light's cone SHOULD be dark by default)
    if (position.x < 0.0f || position.x > 1.0f
        || position.y < 0.0f || position.y > 1.0f
        || position.z < 0.0f || position.z > 1.0f)
    {
        return true;
    }

    // access the shadow map at this position
    float shadowMapZ = texture(shadowMaps[index], position.xy).r;

    // add a bias to prevent shadow acne
    float bias = 0.0005f;
    shadowMapZ += bias;

    // if the depth stored in the texture is less than the current fragment's depth, we are in shadow
    return shadowMapZ < position.z;
    // return false;
}

void main()
{
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
    // result += CalculateDirLight(dir_light, normalDir, viewDir);

    // spotlights
    for (int i = 0; i < 2; i++) {
        vec3 lighting = CalculateSpotLight(spotlights[i], normalDir, viewDir, shaderPosition);
        
        if (enableShadows) {
            // zero-out lighting if the fragment is in shadow
            float visibility = inShadowSpotlight(i) ? 0.0f : 1.0f;
            lighting *= visibility;
        }
        result += lighting;
    }

    result += ambient * vec3(texture(diffuseMap, shaderTexCoord));


    fragmentColor = vec4(result, 1.0f);

}
