#pragma once

#include "FEGammaAndHDRCorrection.h"

static const char* const FEFXAAVS = R"(
#version 400 core

@In_Position@
out vec2 textureCoords;

void main(void)
{
	gl_Position = vec4(FEPosition, 1.0);
	textureCoords = vec2((FEPosition.x + 1.0) / 2.0, 1 - (-FEPosition.y + 1.0) / 2.0);
}
)";

static const char* FEFXAAFS = R"(
#version 400 core

// take this simplified version of FXAA from :
// https://github.com/BennyQBD/3DEngineCpp/blob/054c2dcd7c52adcf8c9da335a2baee78850504b8/res/shaders/filter-fxaa.fs
// original paper :
// http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf

in vec2 textureCoords;
uniform float FXAASpanMax;
uniform float FXAAReduceMin;
uniform float FXAAReduceMul;
uniform vec2 FXAATextuxelSize;
@Texture@ inputTexture;

void main(void)
{
    //vec2 texCoordOffset = R_inverseFilterTextureSize.xy;
	vec2 texCoordOffset = FXAATextuxelSize;//vec2(1.0/1920.0, 1.0/1080.0);
	
	vec3 luma = vec3(0.299, 0.587, 0.114);	
	float lumaTL = dot(luma, texture2D(inputTexture, textureCoords.xy + (vec2(-1.0, -1.0) * texCoordOffset)).xyz);
	float lumaTR = dot(luma, texture2D(inputTexture, textureCoords.xy + (vec2(1.0, -1.0) * texCoordOffset)).xyz);
	float lumaBL = dot(luma, texture2D(inputTexture, textureCoords.xy + (vec2(-1.0, 1.0) * texCoordOffset)).xyz);
	float lumaBR = dot(luma, texture2D(inputTexture, textureCoords.xy + (vec2(1.0, 1.0) * texCoordOffset)).xyz);
	float lumaM  = dot(luma, texture2D(inputTexture, textureCoords.xy).xyz);

	vec2 dir;
	dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
	dir.y = ((lumaTL + lumaBL) - (lumaTR + lumaBR));
	
	float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (FXAAReduceMul * 0.25), FXAAReduceMin);
	float inverseDirAdjustment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = min(vec2(FXAASpanMax, FXAASpanMax), 
		max(vec2(-FXAASpanMax, -FXAASpanMax), dir * inverseDirAdjustment)) * texCoordOffset;

	vec3 result1 = (1.0/2.0) * (
		texture2D(inputTexture, textureCoords.xy + (dir * vec2(1.0/3.0 - 0.5))).xyz +
		texture2D(inputTexture, textureCoords.xy + (dir * vec2(2.0/3.0 - 0.5))).xyz);

	vec3 result2 = result1 * (1.0/2.0) + (1.0/4.0) * (
		texture2D(inputTexture, textureCoords.xy + (dir * vec2(0.0/3.0 - 0.5))).xyz +
		texture2D(inputTexture, textureCoords.xy + (dir * vec2(3.0/3.0 - 0.5))).xyz);

	float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
	float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));
	float lumaResult2 = dot(luma, result2);
	
	if(lumaResult2 < lumaMin || lumaResult2 > lumaMax)
		gl_FragColor = vec4(result1, 1.0);
	else
		gl_FragColor = vec4(result2, 1.0);
}
)";

namespace FocalEngine
{
	class FEFXAA : public FEPostProcess
	{
	public:
		FEFXAA(FEMesh* ScreenQuad, int ScreenWidth, int ScreenHeight);
		~FEFXAA();

	private:

	};
}