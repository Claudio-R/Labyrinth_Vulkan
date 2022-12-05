// WALLS
#version 450
layout(set = 1, binding = 0) uniform Material {
    vec3 specular;
    float shininess;
} material;

layout(set = 1, binding = 1) uniform Light {
    mat4 model;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} light;

layout(set = 1, binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 viewDir;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

// OUTPUT
layout(location = 0) out vec4 outColor;

void main() {
	// ambient
    vec3 ambient = light.ambient * texture(texSampler, texCoord).rgb;
  	
    // diffuse 
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * texture(texSampler, texCoord).rgb);
    
    // specular
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;  
    vec3 result = ambient + diffuse + (specular * 0.1);

    outColor = vec4(result, 1.0);	
}