// Physically-Based Rendering Vertex Shader
struct VS_INPUT {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 bitangent : TEXCOORD3;
    float2 texCoord : TEXCOORD4;
    float4 color : TEXCOORD5;
    float3 viewDir : TEXCOORD6;
    float4 lightSpacePos : TEXCOORD7;
};

cbuffer TransformBuffer : register(b0) {
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldViewProjectionMatrix;
    float4x4 lightViewProjectionMatrix;
    float3 cameraPosition;
    float padding;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    
    // Transform position
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(worldPos, mul(viewMatrix, projectionMatrix));
    output.worldPos = worldPos.xyz;
    
    // Transform normal vectors
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
    output.tangent = normalize(mul(input.tangent, (float3x3)worldMatrix));
    output.bitangent = cross(output.normal, output.tangent);
    
    // Pass through texture coordinates and vertex color
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    // Calculate view direction
    output.viewDir = normalize(cameraPosition - output.worldPos);
    
    // Light space position for shadow mapping
    output.lightSpacePos = mul(worldPos, lightViewProjectionMatrix);
    
    return output;
}
