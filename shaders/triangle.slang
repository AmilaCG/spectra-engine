struct VIn
{
    [[vk::location(0)]] float3 position;
    //[[vk::location(1)]] float3 color;
}

struct VOut
{
    float4 position : SV_Position;
    [[vk::location(0)]] float3 fragColor;
};

[shader("vertex")]
VOut vertexMain(VIn input)
{
    VOut o;
    o.position = float4(input.position, 1.0);
    //o.fragColor = input.color;
    o.fragColor = float3(1.0, 1.0, 0.0);
    return o;
}

struct FIn
{
    [[vk::location(0)]] float3 fragColor;
};

struct FOut
{
    [[vk::location(0)]] float4 outColor;
};

[shader("fragment")]
FOut fragmentMain(FIn i)
{
    FOut o;
    o.outColor = float4(i.fragColor, 1.0);
    return o;
}
