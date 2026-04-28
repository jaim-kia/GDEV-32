#version 330 core

in vec2 shaderTexCoord;
uniform sampler2D hdrScene;
uniform sampler2D bloomBlur;
uniform float bloomStrength;
uniform float exposure;
out vec4 fragmentColor;

void main() {
    vec3 hdrColor   = texture(hdrScene,  shaderTexCoord).rgb;
    vec3 bloomColor = texture(bloomBlur, shaderTexCoord).rgb;

    hdrColor += bloomColor * bloomStrength;

    // ACES filmic tonemapping
    // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    vec3 x = hdrColor * exposure;
    vec3 result = (x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f);
    result = clamp(result, 0.0f, 1.0f);

    result = pow(result, vec3(1.0f / 2.2f));

    fragmentColor = vec4(result, 1.0f);
}