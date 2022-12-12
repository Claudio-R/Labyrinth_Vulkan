#version 450

// lights
layout(set = 1, binding = 0) uniform Material {
    vec3 lightPositions;
    vec3 lightColors;
} l;

// material parameters
layout(set = 1, binding = 1) uniform sampler2D albedo_map;
layout(set = 1, binding = 2) uniform sampler2D metallic_map;
layout(set = 1, binding = 3) uniform sampler2D roughness_map;
layout(set = 1, binding = 4) uniform sampler2D ao_map;

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 viewDir;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec2 TexCoords;

// OUTPUT
layout(location = 0) out vec4 FragColor;

const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main()
{		
    vec3 albedo = texture(albedo_map, TexCoords).rgb;
    float metallic = texture(metallic_map, TexCoords).r;
    float roughness = texture(roughness_map, TexCoords).r;
    float ao = texture(ao_map, TexCoords).r;
    
    vec3 N = normalize(Normal);
    vec3 V = viewDir;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
//    for(int i = 0; i < 4; ++i) 
//    {
        // calculate per-light radiance
        //vec3 L = normalize(lightPositions[i] - FragPos);
        vec3 L = normalize(l.lightPositions - FragPos);
        vec3 H = normalize(V + L);
        //float distance    = length(lightPositions[i] - FragPos);
        float distance    = length(l.lightPositions - FragPos);
        float attenuation = 1.0 / (distance * distance);
        //vec3 radiance     = lightColors[i] * attenuation;        
        vec3 radiance     = l.lightColors * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
//    }   
  
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    FragColor = vec4(color, 1.0);
} 

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}