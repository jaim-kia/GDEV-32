/******************************************************************************
 * This fragment shader is a modification of demo5.vs, adding a function that
 * determines whether a fragment is in shadow.
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

///////////////////////////////////////////////////////////////////////////////
// added for shadow mapping
in vec4 shaderLightSpacePosition;
uniform sampler2D shadowMap;

bool inShadow()
{
    // perform perspective division and rescale to the [0, 1] range to get the coordinates into the depth texture
    vec3 position = shaderLightSpacePosition.xyz / shaderLightSpacePosition.w;
    position = position * 0.5f + 0.5f;

    // if the position is outside the light-space frustum, do NOT put the
    // fragment in shadow, to prevent the scene from becoming dark "by default"
    // (note that if you have a spot light, you might want to do the opposite --
    // that is, everything outside the spot light's cone SHOULD be dark by default)
    if (position.x < 0.0f || position.x > 1.0f
        || position.y < 0.0f || position.y > 1.0f
        || position.z < 0.0f || position.z > 1.0f)
    {
        return false;
    }

    // access the shadow map at this position
    float shadowMapZ = texture(shadowMap, position.xy).r;

    // add a bias to prevent shadow acne
    float bias = 0.0005f;
    shadowMapZ += bias;

    // if the depth stored in the texture is less than the current fragment's depth, we are in shadow
    return shadowMapZ < position.z;
}
///////////////////////////////////////////////////////////////////////////////

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

    ///////////////////////////////////////////////////////////////////////////
    // zero-out the diffuse and specular components if the fragment is in shadow
    if (inShadow())
        lightDiffuse = lightSpecular = vec3(0.0f, 0.0f, 0.0f);
    ///////////////////////////////////////////////////////////////////////////

    // compute final fragment color
    fragmentColor = vec4((lightAmbient + lightDiffuse + lightSpecular), 1.0f) * texture(diffuseMap, shaderTexCoord);
}
