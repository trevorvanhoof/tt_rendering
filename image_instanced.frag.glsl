#version 450

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 cd0;

void main() {
	// cd0 = texture(uImage, vUv.xy);
	// if(cd0.w < 0.5) discard;
	cd0 = vec4(1.0, 1.0, 1.0, 1.0);
}
