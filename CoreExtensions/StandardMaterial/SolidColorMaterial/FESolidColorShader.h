#pragma once

#include "../../../Renderer/FEShader.h"

static const char* const FESolidColorVS = R"(
#version 400 core

@In_Position@
@In_Normal@

@WorldMatrix@
@ViewMatrix@
@ProjectionMatrix@

out vec3 normal;
out vec3 fragPosition;

void main(void)
{
	normal = normalize(mat3(transpose(inverse(FEWorldMatrix))) * FENormal);
	fragPosition = vec3(FEWorldMatrix * vec4(FEPosition, 1.0));

	gl_Position = FEProjectionMatrix * FEViewMatrix * FEWorldMatrix * vec4(FEPosition, 1.0);
}
)";

static const char* const FESolidColorFS = R"(
#version 400 core

in vec3 normal;
in vec3 fragPosition;

uniform vec3 baseColor;
@CameraPosition@

void main(void)
{
    gl_FragColor = vec4(baseColor, 1.0f);
}
)";

namespace FocalEngine 
{
	class FESolidColorShader : public FEShader
	{
	public:
		FESolidColorShader();
		~FESolidColorShader();
	private:
	};
}