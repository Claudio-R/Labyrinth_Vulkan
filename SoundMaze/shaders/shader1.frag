// LIGHTS

#version 450

layout(set = 1, binding = 0) uniform Light {
	mat4 model;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} light;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 viewDir;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main() {
    
    vec3 result = light.ambient + light.diffuse + light.specular;
    outColor = vec4(result, 1.0);

}