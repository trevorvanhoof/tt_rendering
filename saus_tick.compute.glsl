#version 450

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding=0) buffer data { float[] element; };

layout(std140, binding = 2) uniform MaterialInfo {
	float uDeltaTime;
};

void main() {
	const int stride = 7;
	const uint offset = gl_GlobalInvocationID.x * stride;

	// age
	element[offset + 6] += uDeltaTime;

	// ignore premature particle
	if(element[offset + 6] < 0.0)
		return;

	// place particle if it does not exist yet, or has to wrap around
	if(isnan(element[offset + 0]) || element[offset + 6] > 1.0) {
		element[offset + 0] = 0.0;
		element[offset + 1] = 0.0;
		element[offset + 2] = 0.0;
		element[offset + 6] = 0.0;
	}

	const float friction = 2.0;
	const float gravity = 9.81;
	const float bounce = 0.6;

	// apply gravity
	element[offset + 4] -= gravity * uDeltaTime;

	// apply drag
	// float f = max(0.0, 1.0 - friction * uDeltaTime);
	// element[offset + 3] *= f;
	// element[offset + 4] *= f;
	// element[offset + 5] *= f;

	// apply velocity over time
	element[offset + 0] += element[offset + 3] * uDeltaTime;
	element[offset + 1] += element[offset + 4] * uDeltaTime;
	element[offset + 2] += element[offset + 5] * uDeltaTime;

	// apply floor plane
	if(element[offset + 1] < 0.0) {
		element[offset + 1] = 0.0;
		element[offset + 4] = -element[offset + 4] * bounce;
	}
}
