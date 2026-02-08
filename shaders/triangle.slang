struct VIn
{
    [[vk::location(0)]] float2 pos : POSITION;
}

struct VOut
{
    float3 fragColor : LOCATION0;
    float4 position  : SV_Position;
};

[shader("vertex")]
VOut vertexMain(VIn input, uint vertexIndex : SV_VertexID)
{
    float3 colors[3] =
    {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    VOut o;
    o.position  = float4(input.pos, 0.0, 1.0);
    o.fragColor = colors[vertexIndex % 3];
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
