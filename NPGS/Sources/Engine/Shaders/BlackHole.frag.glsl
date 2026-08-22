#version 450
#pragma shader_stage(fragment)
#extension GL_EXT_samplerless_texture_functions : enable
#include "Common/CoordConverter.glsl"
#include "Common/NumericConstants.glsl"

// =============================================================================
// SECTION 1: 输入输出与 Uniform 定义
// =============================================================================

layout(location = 0) out vec4 FragColor;
layout(location = 0) in  vec2 TexCoordFromVert;

#include "BlackHole_common.glsl"

void main()
{
    vec2 Uv = gl_FragCoord.xy / iResolution.xy;

    vec2 Jitter = vec2(RandomStep(Uv, fract(iTime * 1.0 + 0.5)), 
                       RandomStep(Uv, fract(iTime * 1.0))) / iResolution;

    float CurrentStatus;
    vec3  CurrentDir;
    float CurrentShift;
    TraceResult res = TraceRay(Uv + 0.5 * Jitter, iResolution);

        vec4 FinalColor    = res.AccumColor;
        CurrentStatus = res.Status;
        CurrentDir    = res.EscapeDir;
        CurrentShift  = res.FreqShift;

    if ( CurrentStatus > 0.5 && CurrentStatus < 2.5) 
    {
        // 屏幕空间偏导数计算立体角与放大率（与 BlackHole_composite.frag.glsl 一致）
        vec3 dDdx = dFdx(CurrentDir);
        vec3 dDdy = dFdy(CurrentDir);
        float LensedSolidAngle = length(cross(dDdx, dDdy));

        float Fov = tan(iFovRadians / 2.0);
        vec3 ViewDirLocal = FragUvToDir(Uv, Fov, iResolution.xy);
        vec3 dVdx = dFdx(ViewDirLocal);
        vec3 dVdy = dFdy(ViewDirLocal);
        float ScreenSolidAngle = length(cross(dVdx, dVdy));

        float Magnification = ScreenSolidAngle / max(LensedSolidAngle, 1e-12);

        vec4 Bg = SampleBackground(CurrentDir, CurrentShift, CurrentStatus, Magnification, ScreenSolidAngle);
        FinalColor += 0.9999 * Bg * vec4(pow((1.0 - FinalColor.a),1.0+0.3*(1.0-1.0)),pow((1.0 - FinalColor.a),1.0+0.3*(3.0-1.0)),pow((1.0 - FinalColor.a),1.0+0.3*(6.0-1.0)),1.0);
    }


    FinalColor = ApplyToneMapping(FinalColor,CurrentShift);

    vec4 PrevColor = texelFetch(iHistoryTex, ivec2(gl_FragCoord.xy), 0);
    FragColor      = (iBlendWeight) * FinalColor + (1.0 - iBlendWeight) * PrevColor;
}