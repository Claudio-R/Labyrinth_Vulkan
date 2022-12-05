// LIGHTS

#version 450
layout(set = 0, binding = 0) uniform uniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragViewDir;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
	fragPos = vec3(ubo.model * vec4(pos,  1.0));
	fragViewDir = normalize((ubo.view[3]).xyz - fragPos);
	fragNormal = vec3(ubo.model * vec4(norm, 0.0));
	fragTexCoord = texCoord;
}