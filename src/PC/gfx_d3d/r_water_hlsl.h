#ifndef R_WATER_HLSL_H
#define R_WATER_HLSL_H

/* r_water 1: our text for the game's materials/shaders/lib/water.hlsl, which the water techset's four
   fog variants include. 1.3's samples its height map at fixed coordinates, so its water never moves,
   and takes the reflection by the cube map's alpha alone. This one moves two layers of the same map
   with gameTime (code constant: sin, cos, fraction, seconds), mixes the water's color with the
   reflection of the map's own sky (skySampler; the material's cube keeps its alpha as reflectivity)
   by Schlick's fresnel and sharpens the sun's highlight. Where the scene's depth is there
   (floatZSampler, an INTZ texture of ours), the water thins out and foams at the shore: with
   1.3's infinite far plane a depth d is 1 - near / z, so each water pixel finds near from its own
   depth and turns the scene's behind it into a distance, and shows the scene under it (sceneColorSampler,
   copied before the water, rb_shade.c) bent by the waves. Without it floatZ reads 0 and that is off. Same inputs, same pixel shader 2.0
   budget, same material arguments (waterColor, cubeMapSampler, normalMapSampler). */
static const char r_waterShaderText[] =
    "#include \"globals.hlsl\"\n"
    "#define FOGVAR fog\n"
    "struct VertexInput {\n"
    "  float4 position : POSITION; float4 color : COLOR; float2 texCoords : TEXCOORD0;\n"
    "  float3 tangent : TEXCOORD2; float3 binormal : TEXCOORD3; float3 normal : NORMAL;\n"
    "};\n"
    "struct PixelInput {\n"
    "  float4 position : POSITION; float4 color : COLOR0; float2 texCoords : TEXCOORD0;\n"
    "  float3 eyeVector : TEXCOORD1;\n"
    "  float4 clipPos : TEXCOORD2;\n"
    "#if USE_FOG\n"
    "  float fog : FOG_SEMANTIC;\n"
    "#endif\n"
    "};\n"
    "struct PixelOutput { float4 color : COLOR; };\n"
    "float4 waterColor;\n"
    "sampler skySampler;\n"
    "sampler sceneColorSampler;\n"
    "#include \"transform.hlsl\"\n"
    "#include \"fog.hlsl\"\n"
    "PixelInput vs_main( const VertexInput vertex )\n"
    "{\n"
    "  PixelInput pixel = (PixelInput) 0;\n"
    "  pixel.position = Transform_ObjectToClip( vertex.position );\n"
    "  pixel.clipPos = pixel.position;\n"
    "  pixel.color = vertex.color;\n"
    "  pixel.texCoords = vertex.texCoords;\n"
    "  pixel.eyeVector = Transform_ObjectToWorld( vertex.position ) - eyePosition;\n"
    "  Fog_VertexCalc( vertex, pixel );\n"
    "  return pixel;\n"
    "}\n"
    /* two layers drifting apart; an offset that wraps at 1 keeps the tiling seamless */
    "float waveHeight( float2 uv, float2 driftA, float2 driftB )\n"
    "{\n"
    "  return tex2D( normalMapSampler, uv + driftA ).r + 0.5 * tex2D( normalMapSampler, uv * 2.3 + driftB ).r;\n"
    "}\n"
    "PixelOutput ps_main( const PixelInput pixel )\n"
    "{\n"
    "  PixelOutput fragment;\n"
    "  float2 driftA = frac( gameTime.w * float2( 0.012, 0.007 ) );\n"
    "  float2 driftB = frac( gameTime.w * float2( -0.02, 0.025 ) );\n"
    "  float h = waveHeight( pixel.texCoords, driftA, driftB );\n"
    "  float hx = waveHeight( pixel.texCoords + float2( 1.0 / 128, 0 ), driftA, driftB );\n"
    "  float hy = waveHeight( pixel.texCoords + float2( 0, 1.0 / 128 ), driftA, driftB );\n"
    "  float3 normal = normalize( float3( ( h - hx ) * 2, ( h - hy ) * 2, 1 ) );\n"
    "  float3 eyeDir = normalize( pixel.eyeVector );\n"
    "  float3 reflected = reflect( eyeDir, normal );\n"
    "  float sun = pow( max( 0, dot( reflected, lightPosition0.xyz ) ), 128 );\n"
    "  reflected.z = abs( reflected.z );\n"
    "  half4 env = texCUBE( cubeMapSampler, reflected );\n"
    "  env.rgb = texCUBE( skySampler, reflected ).rgb;\n"
    "  float fresnel = saturate( 0.04 + 0.96 * pow( 1 - saturate( -dot( eyeDir, normal ) ), 5 ) + 0.3 * env.a );\n"
    "  float2 screenUV = pixel.clipPos.xy / pixel.clipPos.w * float2( 0.5, -0.5 ) + 0.5;\n"
    "  float waterD = pixel.clipPos.z / pixel.clipPos.w;\n"
    "  float sceneD = tex2D( floatZSampler, screenUV ).r;\n"
    "  float nearZ = pixel.clipPos.w * ( 1 - waterD );\n"
    "  float depth = sceneD > 0 ? nearZ / max( 1 - sceneD, 0.00001 ) - pixel.clipPos.w : 10000;\n"
    /* the scene under the water, bent by the waves; where the bent sample lands on something in front
       of the water it takes the straight one, so nothing above the water is drawn below it */
    "  float2 refractUV = screenUV + normal.xy * 0.03 * saturate( depth / 40 );\n"
    "  refractUV = tex2D( floatZSampler, refractUV ).r < waterD ? screenUV : refractUV;\n"
    "  float clarity = sceneD > 0 ? 1 - saturate( depth / 160 ) : 0;\n"
    "  float3 below = lerp( waterColor.rgb * ( 0.7 + 0.3 * normal.z ), tex2D( sceneColorSampler, refractUV ).rgb, clarity * clarity );\n"
    "  fragment.color.rgb = lerp( below, env.rgb, fresnel ) + sun * 1.5 * lightColor0.rgb;\n"
    "  float foam = max( saturate( 1 - depth / 20 ) * ( 0.6 + 0.4 * saturate( h ) ), saturate( 1 - depth / 60 ) * saturate( ( h - 0.3 ) * 3 ) );\n"
    "  fragment.color.rgb = lerp( fragment.color.rgb, float3( 0.95, 0.97, 1.0 ) * ( 0.5 + 0.5 * lightColor0.rgb ), foam );\n"
    /* with the scene's depth the shader shows what is below itself: opaque, as the vertex color says */
    "  fragment.color.a = sceneD > 0 ? pixel.color.a : pixel.color.a * max( lerp( 0.25, 1, saturate( depth / 24 ) ), foam );\n"
    "  Fog_PixelCalc( pixel, fragment );\n"
    "  return fragment;\n"
    "}\n";

#endif
