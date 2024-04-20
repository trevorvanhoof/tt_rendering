#version 450

layout(location = 0) in vec2 vUv;
layout(location = 1) in flat int vInstanceID;

layout(location = 0) out vec4 cd0;

uniform sampler2D uImage[10];

void main() {
	cd0 = texture(uImage[vInstanceID], vUv.xy);
	if(cd0.w < 0.5) discard;
}