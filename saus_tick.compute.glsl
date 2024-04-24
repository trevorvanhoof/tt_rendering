#version 450

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding=0) buffer data { float[] element; };

layout(std140, binding = 2) uniform MaterialInfo {
	float uDeltaTime;
	float uSeconds;
	// float uLifetime;
};

vec4 h4(vec4 p4) {
	p4 = fract(p4 * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy + 19.19);
    return fract((p4.xxyz + p4.yzzw) * p4.zywx);
}

void pR(inout vec2 v, float a) { v = v * cos(a) + vec2(v.y, -v.x) * sin(a); }

void main() {
	const float uLifetime = 10.0;

	const int stride = 7;
	const uint offset = gl_GlobalInvocationID.x * stride;

	// age
	element[offset + 6] += uDeltaTime;

	// ignore premature particle
	if(element[offset + 6] < 0.0)
		return;

	// place particle if it does not exist yet, or has to wrap around
	if(isinf(element[offset + 0]) || element[offset + 6] > uLifetime) {
		element[offset + 0] = 0.0;
		element[offset + 1] = 0.0;
		element[offset + 2] = 0.0;
		
		// random start velocity
		// vec4 h = h4(vec4(gl_GlobalInvocationID.xxx, 1.0));
		// const float s = 60.0;
		// element[gl_GlobalInvocationID.x * stride + 3] = (h.x - 0.5) * s * 2.0;
		// element[gl_GlobalInvocationID.x * stride + 4] = (h.y + 0.75) * s;
		// element[gl_GlobalInvocationID.x * stride + 5] = (h.z - 0.5) * s * 2.0;

		// spiral velocity
		float sa = sin(uSeconds * 5.0);
		float ca = cos(uSeconds * 5.0);
		vec3 v = vec3(ca, 2.0, sa) * 30.0;
		pR(v.xy, 0.5);
		element[gl_GlobalInvocationID.x * stride + 3] = v.x;
		element[gl_GlobalInvocationID.x * stride + 4] = v.y;
		element[gl_GlobalInvocationID.x * stride + 5] = v.z;
	
		// reset lifetime
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
