#version 450

uniform sampler2D uImage;

in vec2 vUV;
in vec4 vColor;

out vec4 fragColor;

void main() {
	fragColor = vec4(vColor.xyz, vColor.w * texture(uImage, vUV).x);
}
