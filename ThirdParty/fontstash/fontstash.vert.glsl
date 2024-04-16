#version 450

uniform vec2 uResolution;

layout(location = 0) in vec2 aVertex;
layout(location = 1) in vec2 aUV;
layout(location = 2) in uvec4 aColor;

out vec2 vUV;
out vec4 vColor;

void main() {
	gl_Position = vec4((aVertex / uResolution) * 2.0 - 1.0, 0.0, 1.0);
	gl_Position.y = -gl_Position.y;
	vUV = aUV;
	vColor = vec4(aColor) / 256.0;
}
5