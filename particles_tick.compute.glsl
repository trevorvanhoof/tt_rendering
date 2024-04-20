#version 450

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding=0) buffer data { float[] element; };

layout(std140, binding = 2) uniform MaterialInfo {
	float uDeltaTime;
};

void main() {
	const float friction = 2.0;
	const float gravity = 9.81;
	const float bounce = 0.6;

	// apply gravity
	element[gl_GlobalInvocationID.x * 6 + 4] -= gravity * uDeltaTime;

	// apply drag
	float f = max(0.0, 1.0 - friction * uDeltaTime);
	element[gl_GlobalInvocationID.x * 6 + 3] *= f;
	element[gl_GlobalInvocationID.x * 6 + 4] *= f;
	element[gl_GlobalInvocationID.x * 6 + 5] *= f;

	// apply velocity over time
	element[gl_GlobalInvocationID.x * 6 + 0] += element[gl_GlobalInvocationID.x * 6 + 3] * uDeltaTime;
	element[gl_GlobalInvocationID.x * 6 + 1] += element[gl_GlobalInvocationID.x * 6 + 4] * uDeltaTime;
	element[gl_GlobalInvocationID.x * 6 + 2] += element[gl_GlobalInvocationID.x * 6 + 5] * uDeltaTime;

	// apply floor plane
	if(element[gl_GlobalInvocationID.x * 6 + 1] < 0.0) {
		element[gl_GlobalInvocationID.x * 6 + 1] = 0.0;
		element[gl_GlobalInvocationID.x * 6 + 4] = -element[gl_GlobalInvocationID.x * 6 + 4] * bounce;
	}
}
