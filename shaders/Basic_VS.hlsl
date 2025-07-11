cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 Color;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    
    // Transform vertex position
    float4 worldPos = mul(float4(input.position, 1.0f), World);
    float4 viewPos = mul(worldPos, View);
    output.position = mul(viewPos, Projection);
    
    // Transform normal
    output.normal = mul(input.normal, (float3x3)World);
    output.normal = normalize(output.normal);
    
    // Pass through color
    output.color = Color;
    
    return output;
}