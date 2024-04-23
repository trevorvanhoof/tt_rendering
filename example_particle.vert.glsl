#version 450

layout(std140, binding = 1) uniform PassInfo {
	mat4 uVP;
};

layout(std140, binding = 2) uniform MaterialInfo {
	int uImageCount;
};

uniform sampler2D uImage[10];

layout(push_constant) uniform PushConstants {
	mat4 uModelMatrix;
	mat4 uExtraData;
};

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec3 aOffset;

layout(location = 0) out vec2 vUv;
layout(location = 1) out flat int vInstanceID;

void main() {
	vInstanceID = gl_InstanceID % uImageCount;
	gl_Position = uVP * (uModelMatrix * vec4(aOffset * 500, 1.0) + vec4((aPosition - 0.5) * 0.25 * vec2(textureSize(uImage[vInstanceID], 0)), 0.0, 1.0));
	vUv = vec2(aPosition.x, 1.0 - aPosition.y);
}
