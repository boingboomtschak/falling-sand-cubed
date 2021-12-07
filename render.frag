#version 130

in vec3 vPoint;
in vec3 vNormal;

out vec4 pColor;

uniform vec4 color = vec4(1, 0, 0, 1);
uniform vec4 light_color = vec4(1, 1, 1, 1);
uniform vec3 light_pos = vec3(-.2, .1, -3);
uniform float amb = 0.5; // ambient intensity
uniform float dif = 0.4; // diffusivity
uniform float spc = 0.7; // specularity

void main() {
	vec3 N = normalize(vNormal);           // surface normal
    vec3 L = normalize(light_pos-vPoint);  // light vector
    vec3 E = normalize(vPoint);            // eye vertex
    vec3 R = reflect(L, N);                // highlight vector
	float d = dif*max(0, dot(N, L));       // one-sided Lambert
	float h = max(0, dot(R, E));           // highlight term
	float s = spc*pow(h, 100);             // specular term
    float intensity = clamp(amb+d+s, 0, 1);
	pColor = vec4(intensity*light_color.rgb, 1) * color;
}