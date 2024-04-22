#version 450

layout(std140, binding = 1) uniform PassInfo {
	mat4 uVP;
};

// layout(std140, binding = 2) uniform MaterialInfo {};

layout(push_constant) uniform PushConstants {
	mat4 uModelMatrix;
	mat4 uExtraData;
};

layout(location = 0) in vec2 aPosition;

layout(location = 0) out vec2 vUv;

void main() {
	gl_Position = uVP * uModelMatrix * vec4(aPosition, 0.0, 1.0);
	vUv = vec2(aPosition.x, 1.0 - aPosition.y);
}
