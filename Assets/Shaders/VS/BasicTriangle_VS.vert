#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 fragTexCoords;

layout(set = 0, binding = 0) uniform ModelViewProjectionUniformObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} mvpBuffer;

void main() {
	gl_Position = mvpBuffer.proj * mvpBuffer.view * mvpBuffer.model * vec4(inPosition, 0.0, 1.0);
	vColor = inColor;
	fragTexCoords = inTexCoords;
}