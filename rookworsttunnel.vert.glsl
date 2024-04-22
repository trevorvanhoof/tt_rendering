#version 450

layout(std140, binding = 1) uniform PassInfo {
	mat4 uVP;
	// World space camera pos
	vec3 uCameraPos;
};

layout(std140, binding = 2) uniform MaterialInfo {
	float uSeconds;
};

layout(push_constant) uniform PushConstants {
	mat4 uModelMatrix;
	mat4 uExtraData;
};

layout(location = 0) in vec2 aPosition;

layout(location = 0) out vec2 vUv;
// This is world space vertex - world space camera position, NOT camera space vertex.
layout(location = 1) out vec3 vCameraRelativeVertex;

void main() {
	float z = -(mod(float(gl_InstanceID) + uSeconds * 0.1, 1000.0) - 500.0) * 50.0;

	float x = sin(uSeconds + z) * 25.0 * clamp((uCameraPos.z - z) - 200.0, 0.0, 400.0) * 0.02;
	float y = -cos(uSeconds + z) * 25.0 * clamp((uCameraPos.z - z) - 200.0, 0.0, 400.0) * 0.02;

	float angle = sin(-z);
	float ca = cos(angle) * 0.25;
	float sa = sin(angle) * 0.25;

	mat4 inst = mat4(
		ca, sa, 0.0, 0.0,
		-sa, ca, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		x, y, z, 1.0);
		
	vec4 P = inst * uModelMatrix * vec4(aPosition, 0.0, 1.0);
	vCameraRelativeVertex = P.xyz - uCameraPos;

	gl_Position = uVP * P;
	vUv = vec2(aPosition.x, 1.0 - aPosition.y);
}
