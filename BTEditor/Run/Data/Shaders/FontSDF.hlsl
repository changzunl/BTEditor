struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color         : COLOR;
	float2 uv            : TEXCOORD;
};

struct v2p_t
{
	float4 position : SV_Position;
	float4 color    : COLOR;
	float2 uv       : TEXCOORD;
};

Texture2D       diffuseTexture  : register(t0);
SamplerState    diffuseSampler  : register(s0);

cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4   TintColor;
}

cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
}

v2p_t VertexMain(vs_input_t input)
{
    float4 localPos = float4(input.localPosition, 1);
    float4 worldPos = mul(ModelMatrix, localPos);

	v2p_t v2p;
	v2p.position = mul(ProjectionMatrix, mul(ViewMatrix, worldPos));
	v2p.color = input.color;
	v2p.uv = input.uv;
	
	return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float FONT_ASPECT = 0.666;
	float FONT_SMOOTH_EDGE = 0.7;
	float FONT_SDF_BIAS = 0.742;

	float4 diffuse = diffuseTexture.Sample(diffuseSampler, input.uv);
	
    float sigDist = diffuse.r;
	float ddxx = abs(ddx(sigDist));
	float ddyy = abs(ddy(sigDist)) * FONT_ASPECT;
    float width = (ddxx + ddyy) * 0.5 * FONT_SMOOTH_EDGE; // smoothing edge
	float bias = FONT_SDF_BIAS; // thickness bias on font size
    float opacity = smoothstep(clamp(bias - width, 0, 1), clamp(bias + width, 0, 1), sigDist);
	
    if (opacity <= 0)
        discard;
	
	float4 color = TintColor * input.color;
	color.a *= opacity;
	
    return color;
}
