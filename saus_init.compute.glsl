#version 450

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding=0) buffer data { float[] element; };

vec4 h4(vec4 p4) {
	p4 = fract(p4 * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy + 19.19);
    return fract((p4.xxyz + p4.yzzw) * p4.zywx);
}

void main() {
	const int stride = 7;

	vec4 h;

	// NaN position, we spawn them over time and then loop around
	element[gl_GlobalInvocationID.x * stride + 0] = 1.0 / 0.0;
	element[gl_GlobalInvocationID.x * stride + 1] = 1.0 / 0.0;
	element[gl_GlobalInvocationID.x * stride + 2] = 1.0 / 0.0;
	
	// random start velocity
	h = h4(vec4(gl_GlobalInvocationID.xxx, 1.0));
	const float s = 60.0;
	element[gl_GlobalInvocationID.x * stride + 3] = (h.x - 0.5) * s * 2.0;
	element[gl_GlobalInvocationID.x * stride + 4] = (h.y + 0.75) * s;
	element[gl_GlobalInvocationID.x * stride + 5] = (h.z - 0.5) * s * 2.0;

	// set -ID as initial lifetime
	element[gl_GlobalInvocationID.x * stride + 6] = -gl_GlobalInvocationID.x * 0.001;
}
