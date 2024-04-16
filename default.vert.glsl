#version 450

uniform mat4 uV;
uniform mat4 uP;

in vec3 aVertex;

void main() {
	gl_Position = uP * uV * vec4(aVertex, 1.0);
}