#version 450

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec3 vCameraRelativeVertex;

layout(location = 0) out vec4 cd0;

uniform sampler2D uImage;

void main() {
	cd0 = texture(uImage, vUv);
	if(length(cd0 - vec4(0.5, 0.5, 0.5, 1.0)) < 0.01) discard;
}
