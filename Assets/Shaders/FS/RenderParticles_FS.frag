#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 vColor;

void main() {
	vec2 coord = gl_PointCoord - vec2(0.5);
    outColor = vec4(vColor.xyz, 0.5 - length(coord));
}