#version 450
layout(set = 0, binding = 0) uniform uniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexNorm;
layout(location = 2) in vec2 vertexTexCoords;

layout(location = 0) out vec3 fragmentPos;
layout(location = 1) out vec3 fragmentNormal;
layout(location = 2) out vec2 fragmentTexCoords;

void main() {
	fragmentPos = vec3(ubo.model * vec4(vertexPos,  1.0));
	fragmentNormal = vec3(ubo.model * vec4(vertexNorm, 0.0));
	fragmentTexCoords = vertexTexCoords;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vertexPos, 1.0);
}