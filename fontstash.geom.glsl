#version 450 core

layout (lines) in;

layout (triangle_strip, max_vertices = 4) out;

layout(std140, binding = 1) uniform PassInfo {
	mat4 uVP;
};

// layout(std140, binding = 2) uniform MaterialInfo {};

layout(push_constant) uniform PushConstants {
	mat4 uModelMatrix;
	mat4 uExtraData;
};

layout(location = 0) in vec2 gUv[];
layout(location = 1) in vec4 gColor[];

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;

void main() {
	vec2 A = gl_in[0].gl_Position.xy;
	vec2 B = gl_in[1].gl_Position.xy;
	vColor = gColor[0];

    gl_Position = uVP * uModelMatrix * vec4(A.x, A.y, 0.0, 1.0);
	vUv = gUv[0];
    EmitVertex();
    
	gl_Position = uVP * uModelMatrix * vec4(B.x, A.y, 0.0, 1.0);
	vUv = vec2(gUv[1].x, gUv[0].y);
    EmitVertex();
	
    gl_Position = uVP * uModelMatrix * vec4(A.x, B.y, 0.0, 1.0);
	vUv = vec2(gUv[0].x, gUv[1].y);
    EmitVertex();

    gl_Position = uVP * uModelMatrix * vec4(B.x, B.y, 0.0, 1.0); 
	vUv = gUv[1];
    EmitVertex();

    EndPrimitive();
}
