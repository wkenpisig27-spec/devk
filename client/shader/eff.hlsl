// SM4 stand-in for client/shader/eff.fx techniques t0-t6.
// Pixel formula is tex * (TFACTOR or vertex color). Raster/blend/sampler
// stay in C++ (D3D11 OM). Compiled by lwD3D11MeshCompileEff.
//
// t0 model      — tex * diffuse, z-read, alpha blend, wrap
// t1 opaque     — tex * diffuse, z-write, no blend
// t2 shade      — tex * diffuse, clamp
// t3 particle   — tex * TFACTOR, clamp
// t4 shade2     — tex * diffuse, alpha test
// t5 font       — tex * diffuse, no z, point + clamp
// t6 font com   — tex * diffuse, no z, linear + wrap
//
// CB0 layout must match MeshCB0 in lwD3D11Mesh.cpp.

cbuffer CB0 : register(b0) {
  row_major float4x4 world;
  row_major float4x4 viewProj;
  float4 lightDir;
  float4 ambient;
  float4 diffuse;
  float4 flags;
  float4 extra;
  float4 outlineColor;
  float4 more; /* x=hasColor, y=unlit, z=dual unused, w=tfactor mix */
  float4 tfactor;
  row_major float4x4 uvMat;
  row_major float4x4 uvMat1;
  float4 look;
  float4 hemiSky;
  float4 hemiGnd;
  float4 fog;
  float4 fogMore;
};

Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct PSIn {
  float4 pos : SV_POSITION;
  float3 nrm : NORMAL;
  float2 uv : TEXCOORD0;
  float2 uv1 : TEXCOORD1;
  float4 col : COLOR;
  float3 wpos : TEXCOORD2;
};

float4 PSMain(PSIn i) : SV_TARGET {
  float4 tex = tex0.Sample(samp0, i.uv);
  if (extra.w >= 0.0)
    clip(tex.a - extra.w);
  int mix = (int)(more.w + 0.5);
  float3 tint = (mix & 1) ? tfactor.rgb : i.col.rgb;
  float a = tex.a * ((mix & 2) ? tfactor.a : i.col.a);
  return float4(tex.rgb * tint, a);
}
