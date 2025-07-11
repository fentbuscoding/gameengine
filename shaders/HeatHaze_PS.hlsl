// Enhanced Heat Haze Distortion Pixel Shader v2.0
struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// Textures
Texture2D sceneTexture : register(t0);
Texture2D noiseTexture : register(t1);
Texture2D depthTexture : register(t2);
Texture2D heatMask : register(t3);
SamplerState defaultSampler : register(s0);

// Heat haze parameters
cbuffer HeatHazeBuffer : register(b0) {
    float heatHazeStrength;
    float heatHazeSpeed;
    float time;
    float heatHazeScale;
    float2 windDirection;
    float distortionFrequency;
    float heatIntensity;
    float temperatureThreshold;
    float fadeDistance;
    float2 viewportSize;
    float nearPlane;
    float farPlane;
};

// Noise functions for more realistic heat distortion
float noise(float2 p) {
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float fbm(float2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    
    return value;
}

float4 main(PS_INPUT input) : SV_Target {
    // Sample depth for depth-based attenuation
    float depth = depthTexture.Sample(defaultSampler, input.texCoord).r;
    float linearDepth = (2.0f * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
    
    // Sample heat mask to determine where heat distortion should occur
    float heatMaskValue = heatMask.Sample(defaultSampler, input.texCoord).r;
    
    // Calculate time-based animation with wind
    float2 animatedUV = input.texCoord * heatHazeScale;
    animatedUV += windDirection * time * heatHazeSpeed;
    
    // Multi-octave noise for more realistic heat distortion
    float2 noiseUV1 = animatedUV + float2(time * 0.1f, time * 0.05f);
    float2 noiseUV2 = animatedUV * 2.0f + float2(time * 0.15f, time * 0.1f);
    
    float noise1 = fbm(noiseUV1);
    float noise2 = fbm(noiseUV2);
    
    // Combine noise samples
    float2 distortionNoise = float2(noise1, noise2) - 0.5f;
    
    // Add turbulence for more realistic heat shimmer
    float2 turbulence = float2(
        sin(input.texCoord.y * distortionFrequency + time * 2.0f),
        cos(input.texCoord.x * distortionFrequency + time * 1.5f)
    ) * 0.1f;
    
    distortionNoise += turbulence;
    
    // Apply heat intensity and distance attenuation
    float distanceAttenuation = 1.0f - saturate(linearDepth / fadeDistance);
    float finalStrength = heatHazeStrength * heatIntensity * heatMaskValue * distanceAttenuation;
    
    // Calculate final displacement
    float2 displacement = distortionNoise * finalStrength;
    
    // Apply chromatic aberration for more realistic heat effect
    float2 redUV = input.texCoord + displacement * 1.1f;
    float2 greenUV = input.texCoord + displacement;
    float2 blueUV = input.texCoord + displacement * 0.9f;
    
    // Sample RGB channels separately for chromatic aberration
    float red = sceneTexture.Sample(defaultSampler, redUV).r;
    float green = sceneTexture.Sample(defaultSampler, greenUV).g;
    float blue = sceneTexture.Sample(defaultSampler, blueUV).b;
    
    // Combine channels
    float3 finalColor = float3(red, green, blue);
    
    // Add subtle heat shimmer overlay
    float shimmer = sin(time * 10.0f + input.texCoord.x * 50.0f) * 0.02f + 1.0f;
    finalColor *= shimmer;
    
    return float4(finalColor, 1.0f);
}
