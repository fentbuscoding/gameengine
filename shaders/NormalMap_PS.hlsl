// Enhanced Normal Mapping Pixel Shader v2.0
struct PS_INPUT {
    float2 texCoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
    float3 binormal : TEXCOORD4;
    float4 lightSpacePos : TEXCOORD5;
    float4 prevFramePos : TEXCOORD6;
    float  viewDistance : TEXCOORD7;
};

// Textures
Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specularTexture : register(t2);
Texture2D shadowMap : register(t3);
Texture2D emissiveTexture : register(t4);
Texture2D roughnessTexture : register(t5);
Texture2D aoTexture : register(t6);
TextureCube environmentMap : register(t7);

SamplerState defaultSampler : register(s0);
SamplerState shadowSampler : register(s1);

// Lighting parameters
cbuffer LightingBuffer : register(b0) {
    float3 lightDirection;
    float lightIntensity;
    float3 lightColor;
    float ambientStrength;
    float3 eyePosition;
    float fogStart;
    float fogEnd;
    float3 fogColor;
    float time;
    bool useIBL;
    float iblStrength;
    float2 padding;
};

// Material parameters
cbuffer MaterialBuffer : register(b1) {
    bool normalMapEnabled;
    float normalMapStrength;
    bool shadowMapEnabled;
    float specularPower;
    bool emissiveEnabled;
    float emissiveStrength;
    bool roughnessEnabled;
    float roughnessScale;
    bool aoEnabled;
    float aoStrength;
    bool environmentMapEnabled;
    float reflectionStrength;
};

// Advanced shadow mapping with PCF
float CalculateShadowFactor(float4 lightSpacePos) {
    if (!shadowMapEnabled) return 1.0f;
    
    // Perspective divide
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    
    // Transform to [0,1] range
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = projCoords.y * -0.5f + 0.5f;
    
    // Check if position is in shadow
    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f) {
        return 1.0f; // Outside shadow map bounds
    }
    
    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0f;
    float2 texelSize = 1.0f / float2(2048.0f, 2048.0f); // Assume 2048x2048 shadow map
    float bias = 0.005f;
    
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * texelSize;
            float shadowDepth = shadowMap.Sample(shadowSampler, projCoords.xy + offset).r;
            float currentDepth = projCoords.z;
            shadow += (currentDepth - bias > shadowDepth) ? 0.3f : 1.0f;
        }
    }
    return shadow / 9.0f;
}

// Fresnel reflection calculation
float3 FresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// Fog calculation
float3 ApplyFog(float3 color, float distance) {
    float fogFactor = saturate((distance - fogStart) / (fogEnd - fogStart));
    return lerp(color, fogColor, fogFactor);
}

float4 main(PS_INPUT input) : SV_Target {
    // Sample textures
    float4 diffuseColor = diffuseTexture.Sample(defaultSampler, input.texCoord);
    float4 specularColor = specularTexture.Sample(defaultSampler, input.texCoord);
    float roughness = roughnessEnabled ? roughnessTexture.Sample(defaultSampler, input.texCoord).r * roughnessScale : 0.5f;
    float ao = aoEnabled ? aoTexture.Sample(defaultSampler, input.texCoord).r * aoStrength : 1.0f;
    
    // Calculate normal
    float3 normal = normalize(input.normal);
    
    if (normalMapEnabled) {
        // Sample normal map
        float3 normalMap = normalTexture.Sample(defaultSampler, input.texCoord).xyz * 2.0f - 1.0f;
        normalMap.xy *= normalMapStrength;
        
        // Transform to world space
        float3x3 TBN = float3x3(
            normalize(input.tangent),
            normalize(input.binormal),
            normalize(input.normal)
        );
        
        normal = normalize(mul(normalMap, TBN));
    }
    
    // Lighting calculations
    float3 lightDir = normalize(-lightDirection);
    float3 viewDir = normalize(eyePosition - input.worldPos);
    float3 halfDir = normalize(lightDir + viewDir);
    float3 reflectDir = reflect(-viewDir, normal);
    
    // Diffuse lighting
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = diffuseColor.rgb * lightColor * NdotL * lightIntensity;
    
    // Specular lighting with roughness
    float NdotH = max(dot(normal, halfDir), 0.0f);
    float specularExp = (2.0f / (roughness * roughness)) - 2.0f;
    float3 specular = specularColor.rgb * lightColor * pow(NdotH, specularExp) * lightIntensity;
    
    // Fresnel reflection
    float VdotN = max(dot(viewDir, normal), 0.0f);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), diffuseColor.rgb, specularColor.a);
    float3 fresnel = FresnelSchlick(VdotN, F0);
    
    // Environment mapping
    float3 envReflection = float3(0.0f, 0.0f, 0.0f);
    if (environmentMapEnabled && useIBL) {
        envReflection = environmentMap.Sample(defaultSampler, reflectDir).rgb * reflectionStrength * iblStrength;
    }
    
    // Shadow factor
    float shadowFactor = CalculateShadowFactor(input.lightSpacePos);
    
    // Ambient lighting with AO
    float3 ambient = diffuseColor.rgb * ambientStrength * ao;
    
    // Emissive
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    if (emissiveEnabled) {
        emissive = emissiveTexture.Sample(defaultSampler, input.texCoord).rgb * emissiveStrength;
    }
    
    // Combine lighting
    float3 finalColor = ambient + (diffuse + specular) * shadowFactor + envReflection * fresnel + emissive;
    
    // Apply fog
    finalColor = ApplyFog(finalColor, input.viewDistance);
    
    return float4(finalColor, diffuseColor.a);
}
