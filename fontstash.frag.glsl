#version 450

uniform sampler2D uImage;

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 cd0;

void main() {
	cd0 = vec4(vColor.xyz, vColor.w * texture(uImage, vUv).x);
}
