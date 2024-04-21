#version 450

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUv;
layout(location = 2) in uint aColor;

layout(location = 0) out vec2 gUv;
layout(location = 1) out vec4 gColor;

void main() {
	gl_Position = vec4(aPosition, 0.0, 1.0);
	gUv = aUv;
	gColor = unpackUnorm4x8(aColor);
}
