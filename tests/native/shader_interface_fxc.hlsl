struct In { float4 p : POSITION; float3 n : NORMAL; float2 uv : TEXCOORD0; };
struct Out { float4 p : SV_Position; float4 uv : TEXCOORD0; };
Out vs_read(In i) { Out o; o.p=i.p; o.uv=float4(i.uv,i.n.x,i.n.y); return o; }
Out vs_unread(In i) { Out o; o.p=i.p; o.uv=float4(i.uv,0,0); return o; }
float4 ps(float4 p:SV_Position, float4 uv:TEXCOORD0, float4 missing:TEXCOORD1):SV_Target { return uv + missing; }
struct Packed { float4 p:SV_Position; float2 a:TEXCOORD0; float2 b:TEXCOORD1; };
Packed vs_packed(In i) { Packed o; o.p=i.p; o.a=i.uv; o.b=i.n.xy; return o; }
float4 ps_packed(float4 p:SV_Position,float2 a:TEXCOORD0,float2 b:TEXCOORD1):SV_Target { return float4(a,b); }
