#version 450
#define NUM_TREASURES 10
#define PI 3.1415926535897932384626433832795

// lights
layout(set = 1, binding = 0) uniform Material {
    vec3 camPos;
    vec3 lightPositions[NUM_TREASURES];
    vec3 lightColors[NUM_TREASURES];
    bool isActive[NUM_TREASURES];
    
} lights;

layout(set = 1, binding = 1) uniform sampler2D albedo_map;
layout(set = 1, binding = 2) uniform sampler2D metallic_map;
layout(set = 1, binding = 3) uniform sampler2D roughness_map;
layout(set = 1, binding = 4) uniform sampler2D ao_map;

//layout(location = 1) in vec3 viewDir;
layout(location = 0) in vec3 fragmentPos;
layout(location = 1) in vec3 fragmentNorm;
layout(location = 2) in vec2 fragmentTexCoords;
layout(location = 0) out vec4 FragColor;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

vec3 albedo = texture(albedo_map, fragmentTexCoords).rgb;
float metallic = texture(metallic_map, fragmentTexCoords).r;
float roughness = texture(roughness_map, fragmentTexCoords).r;
float ao = texture(ao_map, fragmentTexCoords).r;

void main() 
{		    
    vec3 N = normalize(fragmentNorm);
    vec3 V = normalize(lights.camPos - fragmentPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < NUM_TREASURES; ++i) 
    {
        if (lights.isActive[i] == false) { continue; }
        vec3 L = normalize(lights.lightPositions[i] - fragmentPos);
        vec3 H = normalize(V + L);
        
        // calculate per-light radiance
        float distance = length(lights.lightPositions[i] - fragmentPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lights.lightColors[i] * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G = GeometrySmith(N, V, L, roughness);      
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;  
            
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.0));  
    
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