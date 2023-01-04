#version 450
layout(set = 0, binding = 0) uniform globalUniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} gubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragViewDir;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragTexCoord;

void main() {
	gl_Position = gubo.proj * gubo.view * gubo.model * vec4(pos, 1.0);
	vec3 fragPos = vec3(gubo.model * vec4(pos, 1.0));
	fragViewDir = normalize(-fragPos);
//	fragViewDir  = (gubo.view[3]).xyz - (gubo.model * vec4(pos,  1.0)).xyz;
	fragNorm     = (gubo.model * vec4(norm, 0.0)).xyz;
	fragTexCoord = texCoord;
}