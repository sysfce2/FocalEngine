#pragma once

#include "..\SubSystems\FECoreIncludes.h"

namespace FocalEngine
{
	enum FE_TEXTURE_MAG_FILTER
	{
		FE_NEAREST = 0,
		FE_LINEAR = 1,
	};

	class FEResourceManager;
	class FEScreenSpaceEffect;

	class FETexture
	{
		friend FEResourceManager;
		friend FEScreenSpaceEffect;
	public:
		FETexture();
		FETexture(int Width, int Height);
		FETexture(GLint InternalFormat, GLenum Format, int Width, int Height);
		~FETexture();

		GLuint getTextureID();

		std::string getName();
		void setName(std::string newName);

		virtual void bind(const unsigned int textureUnit = 0);
		virtual void unBind();
	private:
		GLuint textureID = -1;
		std::string name = "DefaultName";
		bool hdr = false;

		int width = 0;
		int height = 0;
		GLint internalFormat;
		GLenum format;
		GLuint defaultTextureUnit = -1;

		FE_TEXTURE_MAG_FILTER magFilter = FE_LINEAR;
		bool mipEnabled = true;
	};
}