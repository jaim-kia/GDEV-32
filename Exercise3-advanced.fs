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
in vec4 dirLightSpacePositions[1];
in vec4 spotLightSpacePositions[2];

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;
uniform sampler2D directionalShadowTextures[1];
uniform sampler2D spotShadowTextures[2];

uniform bool enableShadows;

uniform DirLight dir_lights[1];
uniform SpotLight spotlights[2];
uniform vec2 shadowTexelStep;

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

float inShadowSpotlight(int index)
{
    // perform perspective division and rescale to the [0, 1] range to get the coordinates into the depth texture
    vec3 position = spotLightSpacePositions[index].xyz / spotLightSpacePositions[index].w;
    position = position * 0.5f + 0.5f;

    // if the position is outside the light-space frustum, do NOT put the
    // fragment in shadow, to prevent the scene from becoming dark "by default"
    // (note that if you have a spot light, you might want to do the opposite --
    // that is, everything outside the spot light's cone SHOULD be dark by default)
    if (position.x < 0.0f || position.x > 1.0f
        || position.y < 0.0f || position.y > 1.0f
        || position.z < 0.0f || position.z > 1.0f)
    {
        return 0.0f;
    }

    // access the shadow map at this position
    // float shadowMapZ = texture(shadowMaps[index], position.xy).r;

    // add a bias to prevent shadow acne
    float bias = 0.0005f;
    // shadowMapZ += bias;
    float shadow = 0.0f;

    int kernelRadius = 2; // 1 is 3x3 2 is 5 and so on
    int samples = 0;

    for (int x = -kernelRadius; x <= kernelRadius; x++)
    {
        for (int y = -kernelRadius; y <= kernelRadius; y++)
        {
            // shift the pixel based on kernel
            // shadowTexelStep converts offset from pixelunit (from kernel) to uv space (for texture)
            float shadowMapZ = texture(shadowMaps[index], position.xy + vec2(x, y) * shadowTexelStep).r;
            shadow += (shadowMapZ + bias < position.z) ? 0.0f : 1.0f;
            samples++;
        }
    }

    return shadow / float(samples); // 0.0 = fully in shadow, 1.0 = fully lit
    // if the depth stored in the texture is less than the current fragment's depth, we are in shadow
    // return shadowMapZ < position.z;
    // return false;
}

void main() {
    // look up the normal from the normal map, then reorient it with the current model transform via the TBN matrix
    vec3 textureNormal = vec3(texture(normalMap, shaderTexCoord));
    textureNormal = normalize(textureNormal * 2.0f - 1.0f);  // convert range from [0, 1] to [-1, 1]
    vec3 normalDir = normalize(shaderTBN * textureNormal);

    vec3 viewDir = normalize(-shaderPosition);

    vec3 result = vec3(0.0f);

    // average of ambient light of all light sources
    vec3 ambient = vec3(0.0f);
    for (int i = 0; i < 1; i++) {
        ambient += dir_lights[i].ambient;
    }
    for (int i = 0; i < 2; i++) {
        ambient += spotlights[i].ambient;
    }
    ambient /= 3.0f; // average the ambient light contributions
    
    // directional light
    for (int i = 0; i < 1; i++) {
        vec3 lighting = CalculateDirLight(dir_lights[i], normalDir, viewDir);
        
        if (enableShadows) {
            float visibility = inShadowDirLight(i) ? 0.0f : 1.0f;
            lighting *= visibility;
        }
        result += lighting;
    }

    // spotlights
    for (int i = 0; i < 2; i++) {
        vec3 lighting = CalculateSpotLight(spotlights[i], normalDir, viewDir, shaderPosition);
        float shadow = inShadowSpotlight(i);
        
        if (enableShadows) {
            // zero-out lighting if the fragment is in shadow
            float visibility = inShadowSpotlight(i) ? 0.0f : 1.0f;
            lighting *= visibility;
        }
        result += lighting;
    }

    vec3 diffuseColor = vec3(texture(diffuseMap, shaderTexCoord));
    result += ambient * diffuseColor;
    result += diffuseColor * 0.2f;
    // result += vec3(texture(diffuseMap, shaderTexCoord));

    // result = vec3(0.5f);
    fragmentColor = vec4(result, 1.0f);
}
