#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform GlobalUniformBufferObject {
	mat4 model;
	mat4 lightPos;
	mat4 lightColors; // DA SISTEMARE PER GESTIRE OGNI LUCE
	vec3 eyePos;
	vec4 decay;
} gubo;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

int N = 4;

mat4 point_light_dir(vec3 pos) {
	mat4 point_light_dirs;
	for (int i = 0; i < N; i++) {
		vec3 light_dir = normalize(gubo.lightPos[i].xyz - pos);
		point_light_dirs[i] = vec4(light_dir, 1.0f);
	}
	return point_light_dirs;
}

mat4 point_light_color(vec3 pos) {
	float g = gubo.decay.z;
	float beta = gubo.decay.w;
	mat4 point_light_colors;
	for (int i = 0; i < N; i++) {
		float decay = pow(g/length(gubo.lightPos[i].xyz - pos), beta);
		point_light_colors[i] = vec4(gubo.lightColors[i].xyz * decay, 1.0f);
	}
	return point_light_colors;
}

void main() {
	vec3 Norm = normalize(fragNorm);
	vec3 EyeDir = normalize(gubo.eyePos.xyz - fragPos);
	
	mat4 lD = point_light_dir(fragPos);	// light directions from the light model
	mat4 lC = point_light_color(fragPos);	// light colors and intensities from the light model

	// Lambert Diffuse
	vec3 diffuse = vec3(0.0f); // intanto prova a sommare tutto
	for (int i = 0; i < N; i++) {
		diffuse += (texture(texSampler, fragTexCoord).rgb) * (max(dot(Norm, lD[i].xyz),0.0f)) * lC[i].xyz;
	}
	// Phong Specular
	vec3 specular = vec3(0.0f);
	for (int i = 0; i < N; i ++) {
		specular += vec3(pow(max(dot(EyeDir, -reflect(lD[i].xyz, Norm)), 0.0f), 64.0f)) * lC[i].xyz;
	}
	// Hemispheric ambient
	vec3 ambient = (vec3(0.1f,0.1f, 0.1f) * (1.0f + Norm.y) + vec3(0.0f,0.0f, 0.1f) * (1.0f - Norm.y)) * texture(texSampler, fragTexCoord).rgb;

	outColor = vec4(diffuse + specular + ambient, 1.0f);	
}