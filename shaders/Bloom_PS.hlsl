// Enhanced Bloom Post-Processing Pixel Shader v2.0
struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// Textures
Texture2D sceneTexture : register(t0);
Texture2D bloomTexture : register(t1);  // For multi-pass bloom
SamplerState defaultSampler : register(s0);

// Bloom parameters
cbuffer BloomBuffer : register(b0) {
    float bloomThreshold;
    float bloomIntensity;
    float bloomRadius;
    float bloomSoftness;
    float2 texelSize;
    int bloomPasses;
    float exposure;
    float gamma;
    float saturation;
    float2 padding;
};

// Gaussian blur weights for high-quality bloom
static const float gaussianWeights[9] = {
    0.0947416f, 0.118318f, 0.0947416f,
    0.118318f,  0.147761f, 0.118318f,
    0.0947416f, 0.118318f, 0.0947416f
};

// Tone mapping functions
float3 ReinhardToneMapping(float3 color) {
    return color / (1.0f + color);
}

float3 ACESFilmic(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(PS_INPUT input) : SV_Target {
    float4 sceneColor = sceneTexture.Sample(defaultSampler, input.texCoord);
    
    // Calculate luminance using more accurate weights
    float luminance = dot(sceneColor.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    
    // Apply soft bloom threshold
    float bloomFactor = smoothstep(bloomThreshold - bloomSoftness, bloomThreshold + bloomSoftness, luminance);
    
    // Sample surrounding pixels for gaussian blur
    float3 bloomColor = float3(0.0f, 0.0f, 0.0f);
    int index = 0;
    
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * texelSize * bloomRadius;
            float3 sample = sceneTexture.Sample(defaultSampler, input.texCoord + offset).rgb;
            
            // Apply threshold to each sample
            float sampleLuminance = dot(sample, float3(0.2126f, 0.7152f, 0.0722f));
            float sampleBloomFactor = smoothstep(bloomThreshold - bloomSoftness, bloomThreshold + bloomSoftness, sampleLuminance);
            
            bloomColor += sample * sampleBloomFactor * gaussianWeights[index];
            index++;
        }
    }
    
    // Apply bloom intensity and exposure
    bloomColor *= bloomIntensity * exposure;
    
    // Enhance saturation for more vibrant bloom
    float3 bloomLuma = dot(bloomColor, float3(0.2126f, 0.7152f, 0.0722f));
    bloomColor = lerp(bloomLuma, bloomColor, saturation);
    
    // Apply tone mapping
    bloomColor = ACESFilmic(bloomColor);
    
    // Apply gamma correction
    bloomColor = pow(bloomColor, 1.0f / gamma);
    
    return float4(bloomColor, 1.0f);
}
