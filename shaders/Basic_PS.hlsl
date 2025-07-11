struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

float4 main(PixelInput input) : SV_Target
{
    // Simple lighting calculation
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float lighting = saturate(dot(input.normal, -lightDir));
    
    // Apply lighting to color
    float4 finalColor = input.color;
    finalColor.rgb *= (0.3f + 0.7f * lighting); // Ambient + diffuse
    
    return finalColor;
}