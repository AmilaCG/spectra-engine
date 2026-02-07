struct VOut
{
    float3 fragColor : LOCATION0;
    float4 position  : SV_Position;
};

[shader("vertex")]
VOut vertexMain(uint vertexIndex : SV_VertexID)
{
    float2 positions[3] =
    {
        float2( 0.0, -0.5),
        float2( 0.5,  0.5),
        float2(-0.5,  0.5)
    };

    float3 colors[3] =
    {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    VOut o;
    o.position  = float4(positions[vertexIndex], 0.0, 1.0);
    o.fragColor = colors[vertexIndex];
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
