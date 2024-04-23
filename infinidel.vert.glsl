#version 450

layout(std140, binding = 1) uniform PassInfo {
	mat4 uVP;
	// World space camera pos
	vec3 uCameraPos;
};

layout(std140, binding = 2) uniform MaterialInfo {
	float uSeconds;
	int uInstanceCount;
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
	vUv = vec2(aPosition.x, 1.0 - aPosition.y);
	vUv.x /= 3.0;
	if(gl_InstanceID == uInstanceCount - 1)
		vUv.x += 2.0 / 3.0;
	else if(gl_InstanceID != 0)
		vUv.x += 1.0 / 3.0;

	vec4 P = vec4(aPosition, 0.0, 1.0);
	P.x += gl_InstanceID;

	P = uModelMatrix * P;
	vCameraRelativeVertex = P.xyz - uCameraPos;

	gl_Position = uVP * P;
}
