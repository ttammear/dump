Shader "Unlit/SpriteTest"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
        _Color ("Color", Color) = (0, 0, 0, 1)
        _StepFrom ("Step from", Range(0.0, 1.0)) = 0.99
	}
	SubShader
	{
		Tags 
        { 
            "RenderType"="Opaque"
            "Queue" = "Transparent+1" 
        }
		LOD 100

		Pass
		{
            Blend SrcAlpha OneMinusSrcAlpha

			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
            #pragma multi_compile DUMMY PIXELSNAP_ON
			

			struct appdata
			{
				float4 vertex : POSITION;
				float3 uv : TEXCOORD0;
                float4 color : COLOR;
			};

			struct v2f
			{
				float3 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
                float4 color : COLOR;
			};

			sampler2D _MainTex;
			float4 _MainTex_ST;
            float4 _Color;
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(float3(v.vertex.xy, 0.0f));
                o.uv = v.uv;
				//o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                o.color = v.color;
				return o;
			}
			
			fixed4 frag (v2f i) : SV_Target
			{
				// sample the texture
                float dist = length(float2(0.5f, 0.5f) - i.uv);
                const float radius = 0.5;
                float duv = ddx(i.uv.x); // uv derivative    
                float blendPrec = 1.0f - 3.0*duv;
                float alpha = smoothstep(radius, blendPrec*radius, dist);
                float colorBlend = smoothstep(0.85*radius, blendPrec*0.85*radius, dist);
                float3 bcol = lerp(float3(0.0, 0.0, 0.0), i.color.xyz, colorBlend);
				fixed4 col = fixed4(bcol.x, bcol.y, bcol.z, alpha);
				return col;
			}
			ENDCG
		}
	}
}
