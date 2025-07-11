// Enhanced Normal Mapping Vertex Shader v2.0
struct VS_INPUT {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
    float4 instancePos : TEXCOORD1;  // For instanced rendering
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 tangent : TEXCOORD3;
    float3 binormal : TEXCOORD4;
    float4 lightSpacePos : TEXCOORD5;
    float4 prevFramePos : TEXCOORD6;  // For motion blur
    float  viewDistance : TEXCOORD7;  // For fog/LOD
};

// Transformation matrices
cbuffer TransformBuffer : register(b0) {
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldViewProjectionMatrix;
    float4x4 lightViewProjectionMatrix;
    float4x4 prevFrameWVP;
    float3 cameraPosition;
    float time;
};

// Animation buffer
cbuffer AnimationBuffer : register(b1) {
    float4x4 boneMatrices[128];
    bool hasAnimation;
    float animationTime;
    float2 padding;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    
    // Apply instancing offset if available
    float3 worldPos = input.position + input.instancePos.xyz;
    
    // Apply bone animation if enabled
    if (hasAnimation) {
        int boneIndex = (int)input.instancePos.w;
        if (boneIndex >= 0 && boneIndex < 128) {
            worldPos = mul(float4(worldPos, 1.0f), boneMatrices[boneIndex]).xyz;
        }
    }
    
    // Transform position
    output.position = mul(float4(worldPos, 1.0f), worldViewProjectionMatrix);
    output.worldPos = mul(float4(worldPos, 1.0f), worldMatrix).xyz;
    
    // Transform normal, tangent for normal mapping
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
    output.tangent = normalize(mul(input.tangent, (float3x3)worldMatrix));
    output.binormal = cross(output.normal, output.tangent);
    
    // Pass through texture coordinates with time-based animation
    output.texCoord = input.texCoord;
    
    // Calculate light space position for shadow mapping
    output.lightSpacePos = mul(float4(output.worldPos, 1.0f), lightViewProjectionMatrix);
    
    // Previous frame position for motion blur
    output.prevFramePos = mul(float4(worldPos, 1.0f), prevFrameWVP);
    
    // Calculate view distance for fog/LOD
    output.viewDistance = length(cameraPosition - output.worldPos);
    
    return output;
}
