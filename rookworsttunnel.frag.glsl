#version 450

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec3 vCameraRelativeVertex;

layout(location = 0) out vec4 cd0;

uniform sampler2D uImage;

void main() {
	cd0 = texture(uImage, vUv);
	cd0.xyz *= 1.0 / (1.0 + length(vCameraRelativeVertex) * 0.001);
	if(cd0.w < 0.5) discard;
}
