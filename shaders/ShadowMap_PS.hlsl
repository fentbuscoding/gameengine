// Shadow Map Generation Pixel Shader
struct PS_INPUT {
    float depth : TEXCOORD0;
};

float4 main(PS_INPUT input) : COLOR {
    // Output depth to shadow map
    return float4(input.depth, input.depth, input.depth, 1.0f);
}
