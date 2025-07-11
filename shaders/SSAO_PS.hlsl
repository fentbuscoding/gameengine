// Screen-Space Ambient Occlusion Pixel Shader
struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// G-Buffer textures
Texture2D depthTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D noiseTexture : register(t2);
SamplerState defaultSampler : register(s0);

// SSAO parameters
cbuffer SSAOBuffer : register(b0) {
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 inverseProjectionMatrix;
    float4x4 inverseViewMatrix;
    float3 samples[64];
    float sampleRadius;
    float bias;
    float intensity;
    float2 noiseScale;
    float2 screenSize;
    float nearPlane;
    float farPlane;
    int numSamples;
    float falloff;
};

// Reconstruct world position from depth
float3 reconstructWorldPos(float2 texCoord, float depth) {
    float4 clipSpacePos = float4(texCoord * 2.0f - 1.0f, depth, 1.0f);
    clipSpacePos.y *= -1.0f;
    
    float4 viewSpacePos = mul(clipSpacePos, inverseProjectionMatrix);
    viewSpacePos /= viewSpacePos.w;
    
    float4 worldSpacePos = mul(viewSpacePos, inverseViewMatrix);
    return worldSpacePos.xyz;
}

// Linear depth conversion
float linearizeDepth(float depth) {
    return (2.0f * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
}

float4 main(PS_INPUT input) : SV_Target {
    // Sample depth and normal
    float depth = depthTexture.Sample(defaultSampler, input.texCoord).r;
    float3 normal = normalize(normalTexture.Sample(defaultSampler, input.texCoord).xyz * 2.0f - 1.0f);
    
    // Early exit for background
    if (depth >= 0.999f) {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Reconstruct world position
    float3 worldPos = reconstructWorldPos(input.texCoord, depth);
    
    // Sample noise for randomization
    float2 noiseUV = input.texCoord * noiseScale;
    float3 randomVec = normalize(noiseTexture.Sample(defaultSampler, noiseUV).xyz * 2.0f - 1.0f);
    
    // Create TBN matrix
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    
    // Calculate ambient occlusion
    float occlusion = 0.0f;
    float linearDepth = linearizeDepth(depth);
    
    for (int i = 0; i < numSamples; ++i) {
        // Get sample position
        float3 samplePos = mul(samples[i], TBN);
        samplePos = worldPos + samplePos * sampleRadius;
        
        // Project sample position to screen space
        float4 screenSpacePos = mul(float4(samplePos, 1.0f), mul(viewMatrix, projectionMatrix));
        screenSpacePos /= screenSpacePos.w;
        
        // Convert to texture coordinates
        float2 sampleTexCoord = screenSpacePos.xy * 0.5f + 0.5f;
        sampleTexCoord.y = 1.0f - sampleTexCoord.y;
        
        // Sample depth at this position
        float sampleDepth = depthTexture.Sample(defaultSampler, sampleTexCoord).r;
        float sampleLinearDepth = linearizeDepth(sampleDepth);
        
        // Reconstruct world position of sample
        float3 sampleWorldPos = reconstructWorldPos(sampleTexCoord, sampleDepth);
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0f, 1.0f, sampleRadius / abs(linearDepth - sampleLinearDepth));
        float sampleOcclusion = (sampleLinearDepth <= linearDepth - bias) ? 1.0f : 0.0f;
        
        // Apply falloff
        float distance = length(sampleWorldPos - worldPos);
        float falloffFactor = 1.0f - smoothstep(0.0f, sampleRadius * falloff, distance);
        
        occlusion += sampleOcclusion * rangeCheck * falloffFactor;
    }
    
    occlusion = 1.0f - (occlusion / numSamples);
    occlusion = pow(occlusion, intensity);
    
    return float4(occlusion, occlusion, occlusion, 1.0f);
}
