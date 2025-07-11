// Shadow Map Generation Vertex Shader
struct VS_INPUT {
    float3 position : POSITION;
};

struct VS_OUTPUT {
    float4 position : POSITION;
    float depth : TEXCOORD0;
};

float4x4 lightViewProjectionMatrix;
float4x4 worldMatrix;

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    
    // Transform to world space
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    
    // Transform to light space
    output.position = mul(worldPos, lightViewProjectionMatrix);
    
    // Store depth for shadow map
    output.depth = output.position.z / output.position.w;
    
    return output;
}
