// WALLS
#version 450
layout(set = 1, binding = 0) uniform Material {
    vec4 specular;
} material;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 viewDir;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

// OUTPUT
layout(location = 0) out vec4 outColor;

void main() {
    
    // material params
    vec3 base_color = texture(texSampler, texCoord).rgb;
    vec3 material_specular = vec3(1.0, 1.0, 1.0);
    float material_shininess = 32;
    
    // light params
    vec3 ambient_light_color = vec3(0.8, 0.8, 0.8);
    vec3 diffuse_light_color = vec3(0.8, 0.1, 0.1);
    vec3 specular_light_color = vec3(1.0, 1.0, 1.0);
    vec3 light_position = vec3(-5, 0.1, -5);

	// ambient
    float ambient_strength = 0.1;
    vec3 ambient_color = ambient_strength * ambient_light_color * base_color;
  	
    // diffuse 
    vec3 normal_direction = normalize(normal);
    vec3 light_direction = normalize(light_position - fragPos);
    float angle_diffuse = dot(normal_direction, light_direction);
    float diffuse_strength = max(angle_diffuse, 0.0);
    vec3 diffuse_color = diffuse_strength * diffuse_light_color * base_color;
    
    // specular
    vec3 reflect_direction = reflect(-light_direction, normal_direction);  
    float specular_intensity = 0.8;
    float angle_specular = dot(viewDir, reflect_direction);
    float specular_strength = pow(max(angle_specular, 0.0), material_shininess);
    vec3 specular_color = specular_intensity * specular_strength * specular_light_color * material_specular;  

    // result
    outColor = vec4(ambient_color + diffuse_color + specular_color, 1.0);	
}