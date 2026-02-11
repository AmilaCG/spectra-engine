struct VIn
{
    [[vk::location(0)]] float2 pos : POSITION;
    [[vk::location(1)]] float3 color : COLOR;
}

struct VOut
{
    float3 fragColor : LOCATION0;
    float4 position  : SV_Position;
};

[shader("vertex")]
VOut vertexMain(VIn input)
{
    VOut o;
    o.position  = float4(input.pos, 0.0, 1.0);
    o.fragColor = input.color;
    return o;
}

struct FIn
{
    float3 fragColor : LOCATION0;
};

struct FOut
{
    float4 outColor : LOCATION0;
};

[shader("fragment")]
FOut fragmentMain(FIn i)
{
    FOut o;
    o.outColor = float4(i.fragColor, 1.0);
    return o;
}
