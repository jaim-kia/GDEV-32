/******************************************************************************
 * This is a really simple fragment shader that simply sets the output fragment
 * color to yellow.
 *
 * Happy hacking! - eric
 *****************************************************************************/

#version 330 core

in vec3 worldSpacePosition;
in vec2 shaderTexCoord;
in vec3 worldSpaceNorm;
in vec3 objectColor;

// in vec3 normal;
// in vec3 fragPos;
// in vec3 trans;
// in mat3 rotate;
// in vec3 final_light_pos;
// in float scale_factor;
out vec4 fragmentColor;
// uniform float is_border;

uniform sampler2D texture_file;
uniform bool isTile;
// uniform sampler2D woodTex;
// uniform sampler2D goldTex;

uniform vec3 eye;

void main()
{
    float strength = 0.7;

    vec3 lightPosition = vec3(10.0f, 5.0f, 5.0f);
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    vec2 finalUV = isTile ? (worldSpacePosition.xz * 0.2) : shaderTexCoord;
    vec4 texelColor = texture(texture_file, finalUV);

    vec3 objectColor = texelColor.xyz;

    // diffuse and ambient lighting
    vec3 lightVector = normalize(lightPosition - worldSpacePosition);
    vec3 normalVector = normalize(worldSpaceNorm);

    vec3 colorDiffuse = clamp(dot(lightVector, normalVector), 0, 1) * lightColor * objectColor;
    vec3 colorAmbient = objectColor * 0.10f;

    // phong specular lighting
    float s = 5096.0f;
    vec3 reflectionVector = reflect(-lightVector, normalVector);
    vec3 colorSpecular = pow(max(dot(reflectionVector, eye), 0), s) * lightColor;

    // vec3 colorFinal = colorDiffuse + colorAmbient + colorSpecular;
    vec3 colorFinal = colorDiffuse + colorAmbient;

    // fragmentColor = vec4(colorFinal * strength, 1.0f);
    fragmentColor = texelColor;
}
