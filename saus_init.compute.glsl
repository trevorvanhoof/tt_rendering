#version 450

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding=0) buffer data { float[] element; };

// layout(std140, binding = 2) uniform MaterialInfo {
	// float uLifetime;
	// uint uInstanceCount;
// }
	
void main() {
	const float uLifetime = 10.0;
	const uint uInstanceCount = 1000;

	const int stride = 7;

	vec4 h;

	// NaN position, we spawn them over time and then loop around
	element[gl_GlobalInvocationID.x * stride + 0] = 1.0 / 0.0;
	element[gl_GlobalInvocationID.x * stride + 1] = 1.0 / 0.0;
	element[gl_GlobalInvocationID.x * stride + 2] = 1.0 / 0.0;
	
	// set -ID as initial lifetime
	element[gl_GlobalInvocationID.x * stride + 6] = -(float(gl_GlobalInvocationID.x) / float(uInstanceCount)) * uLifetime;
}
