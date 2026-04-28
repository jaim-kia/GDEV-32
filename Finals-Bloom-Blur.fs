#version 330 core

in vec2 shaderTexCoord;
uniform sampler2D image;
uniform bool horizontal;
out vec4 fragmentColor;

// 9-tap gaussian weights
uniform float weight[5] = float[](0.227027, 0.194595, 0.121622, 0.054054, 0.016216);

void main() {
    vec2 texOffset = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, shaderTexCoord).rgb * weight[0];

    if (horizontal) {
        for (int i = 1; i < 5; i++) {
            result += texture(image, shaderTexCoord + vec2(texOffset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, shaderTexCoord - vec2(texOffset.x * i, 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; i++) {
            result += texture(image, shaderTexCoord + vec2(0.0, texOffset.y * i)).rgb * weight[i];
            result += texture(image, shaderTexCoord - vec2(0.0, texOffset.y * i)).rgb * weight[i];
        }
    }

    fragmentColor = vec4(result, 1.0);
}