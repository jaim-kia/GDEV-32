#version 330 core

in vec2 shaderTexCoord;
uniform sampler2D hdrScene;
uniform sampler2D bloomBlur;
uniform float bloomStrength;
uniform float exposure;
out vec4 fragmentColor;

// Replace the current composite shader main():
void main() {
    vec3 hdrColor   = texture(hdrScene,  shaderTexCoord).rgb;
    vec3 bloomColor = texture(bloomBlur, shaderTexCoord).rgb;

    // Add bloom
    hdrColor += bloomColor * bloomStrength;

    // ACES filmic tonemapping — preserves color saturation much better than reinhard
    // Produces a natural S-curve: shadows stay dark, highlights compress without washing out
    vec3 x = hdrColor * exposure;
    vec3 result = (x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f);
    result = clamp(result, 0.0f, 1.0f);

    // Gamma correction
    result = pow(result, vec3(1.0f / 2.2f));

    fragmentColor = vec4(result, 1.0f);
}