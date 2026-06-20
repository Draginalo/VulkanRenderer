#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 fragTexCoords;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

void main() {
	outColor = texture(texSampler, fragTexCoords);
}