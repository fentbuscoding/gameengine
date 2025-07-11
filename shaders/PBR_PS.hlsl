// Physically-Based Rendering Pixel Shader
struct PS_INPUT {
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 bitangent : TEXCOORD3;
    float2 texCoord : TEXCOORD4;
    float4 color : TEXCOORD5;
    float3 viewDir : TEXCOORD6;
    float4 lightSpacePos : TEXCOORD7;
};

// PBR Textures
Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metallicMap : register(t2);
Texture2D roughnessMap : register(t3);
Texture2D aoMap : register(t4);
Texture2D emissiveMap : register(t5);
Texture2D shadowMap : register(t6);
TextureCube environmentMap : register(t7);
TextureCube irradianceMap : register(t8);
Texture2D brdfLUT : register(t9);

SamplerState defaultSampler : register(s0);
SamplerState shadowSampler : register(s1);

// Lighting constants
cbuffer LightingBuffer : register(b0) {
    float3 lightPositions[4];
    float3 lightColors[4];
    float lightIntensities[4];
    int numLights;
    float3 ambientLight;
    float exposure;
    float gamma;
    float3 fogColor;
    float fogDensity;
    float3 cameraPosition;
    float time;
};

// Material constants
cbuffer MaterialBuffer : register(b1) {
    float3 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    float3 emissiveFactor;
    float alphaCutoff;
    bool useAlbedoMap;
    bool useNormalMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useAOMap;
    bool useEmissiveMap;
    bool useIBL;
    float iblStrength;
};

// Constants
static const float PI = 3.14159265359f;
static const float EPSILON = 1e-6f;

// Utility functions
float3 getNormalFromMap(float2 texCoord, float3 worldPos, float3 worldNormal) {
    float3 tangentNormal = normalMap.Sample(defaultSampler, texCoord).xyz * 2.0f - 1.0f;
    tangentNormal.xy *= normalScale;
    
    float3 Q1 = ddx(worldPos);
    float3 Q2 = ddy(worldPos);
    float2 st1 = ddx(texCoord);
    float2 st2 = ddy(texCoord);
    
    float3 N = normalize(worldNormal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    
    return normalize(mul(tangentNormal, TBN));
}

// PBR Functions
float DistributionGGX(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float calculateShadowFactor(float4 lightSpacePos) {
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5f + 0.5f;
    projCoords.y = 1.0f - projCoords.y;
    
    if (projCoords.x < 0.0f || projCoords.x > 1.0f || projCoords.y < 0.0f || projCoords.y > 1.0f)
        return 1.0f;
    
    float currentDepth = projCoords.z;
    float bias = 0.005f;
    
    // PCF
    float shadow = 0.0f;
    float2 texelSize = 1.0f / float2(2048.0f, 2048.0f);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = shadowMap.Sample(shadowSampler, projCoords.xy + float2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 0.0f : 1.0f;
        }
    }
    shadow /= 9.0f;
    
    return shadow;
}

float4 main(PS_INPUT input) : SV_Target {
    // Sample material properties
    float3 albedo = useAlbedoMap ? albedoMap.Sample(defaultSampler, input.texCoord).rgb * albedoFactor : albedoFactor;
    float metallic = useMetallicMap ? metallicMap.Sample(defaultSampler, input.texCoord).r * metallicFactor : metallicFactor;
    float roughness = useRoughnessMap ? roughnessMap.Sample(defaultSampler, input.texCoord).r * roughnessFactor : roughnessFactor;
    float ao = useAOMap ? aoMap.Sample(defaultSampler, input.texCoord).r : 1.0f;
    float3 emissive = useEmissiveMap ? emissiveMap.Sample(defaultSampler, input.texCoord).rgb * emissiveFactor : emissiveFactor;
    
    // Apply vertex color
    albedo *= input.color.rgb;
    
    // Calculate normal
    float3 N = useNormalMap ? getNormalFromMap(input.texCoord, input.worldPos, input.normal) : normalize(input.normal);
    float3 V = normalize(input.viewDir);
    float3 R = reflect(-V, N);
    
    // Calculate F0 (base reflectance)
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    
    // Reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    
    // Direct lighting
    for (int i = 0; i < numLights; ++i) {
        float3 L = normalize(lightPositions[i] - input.worldPos);
        float3 H = normalize(V + L);
        float distance = length(lightPositions[i] - input.worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = lightColors[i] * lightIntensities[i] * attenuation;
        
        // BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);
        
        float3 kS = F;
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        kD *= 1.0f - metallic;
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + EPSILON;
        float3 specular = numerator / denominator;
        
        float NdotL = max(dot(N, L), 0.0f);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Ambient lighting (IBL)
    float3 ambient = float3(0.0f, 0.0f, 0.0f);
    if (useIBL) {
        float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);
        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metallic;
        
        float3 irradiance = irradianceMap.Sample(defaultSampler, N).rgb;
        float3 diffuse = irradiance * albedo;
        
        const float MAX_REFLECTION_LOD = 4.0f;
        float3 prefilteredColor = environmentMap.SampleLevel(defaultSampler, R, roughness * MAX_REFLECTION_LOD).rgb;
        float2 brdf = brdfLUT.Sample(defaultSampler, float2(max(dot(N, V), 0.0f), roughness)).rg;
        float3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        
        ambient = (kD * diffuse + specular) * ao * iblStrength;
    } else {
        ambient = ambientLight * albedo * ao;
    }
    
    float3 color = ambient + Lo + emissive;
    
    // Apply shadow
    float shadowFactor = calculateShadowFactor(input.lightSpacePos);
    color *= shadowFactor;
    
    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    
    // Gamma correction
    color = pow(color, float3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));
    
    return float4(color, 1.0f);
}
