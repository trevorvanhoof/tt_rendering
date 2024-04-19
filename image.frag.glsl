#version 450

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 cd0;

uniform sampler2D uImage;

void main() {
	cd0 = texture(uImage, vUv);
	if(cd0.w < 0.5) discard;
}