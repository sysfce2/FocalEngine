#include "FERenderer.h"
using namespace FocalEngine;

FERenderer* FERenderer::_instance = nullptr;

FERenderer::FERenderer()
{
}

void FERenderer::standardFBInit(int WindowWidth, int WindowHeight)
{
	sceneToTextureFB = new FEFramebuffer(FE_COLOR_ATTACHMENT | FE_DEPTH_ATTACHMENT, WindowWidth, WindowHeight);
}

void FERenderer::loadStandardParams(FEShader* shader, FEBasicCamera* currentCamera, FEEntity* entity)
{
	static char* name = new char[256];
	FEScene& scene = FEScene::getInstance();

	auto iterator = shader->parameters.begin();
	while (iterator != shader->parameters.end())
	{
		auto parameterName = iterator->first.c_str();

		strcpy_s(name, 256, "FEWorldMatrix");
		if (strcmp(parameterName, name) == 0)
			iterator->second.updateData(entity->transform.getTransformMatrix());

		strcpy_s(name, 256, "FEViewMatrix");
		if (strcmp(parameterName, name) == 0)
			iterator->second.updateData(currentCamera->getViewMatrix());

		strcpy_s(name, 256, "FEProjectionMatrix");
		if (strcmp(parameterName, name) == 0)
			iterator->second.updateData(currentCamera->getProjectionMatrix());

		strcpy_s(name, 256, "FECameraPosition");
		if (strcmp(parameterName, name) == 0)
			iterator->second.updateData(currentCamera->getPosition());

		strcpy_s(name, 256, "FEGamma");
		if (strcmp(parameterName, name) == 0)
			iterator->second.updateData(currentCamera->getGamma());

		strcpy_s(name, 256, "FEExposure");
		if (strcmp(parameterName, name) == 0)
			iterator->second.updateData(currentCamera->getExposure());

		iterator++;
	}
}

void FERenderer::addPostProcess(FEPostProcess* newPostProcess, bool noProcessing)
{
	postProcessEffects.push_back(newPostProcess);

	if (noProcessing)
		return;

	for (size_t i = 0; i < postProcessEffects.back()->stages.size(); i++)
	{
		postProcessEffects.back()->stages[i]->inTexture.resize(postProcessEffects.back()->stages[i]->inTextureSource.size());
		//to-do: change when out texture could be different resolution or/and format.
		postProcessEffects.back()->stages[i]->outTexture = sceneToTextureFB->getColorAttachment()->createSameFormatTexture();
	}
}

void FERenderer::loadUniformBlocks()
{
	FEScene& scene = FEScene::getInstance();
	FEResourceManager& resourceManager = FEResourceManager::getInstance();

	std::vector<FELightShaderInfo> info;
	info.resize(FE_MAX_LIGHTS);

	auto lightIterator = scene.lightsMap.begin();
	int index = 0;
	while (lightIterator != scene.lightsMap.end())
	{
		info[index].typeAndAngles = glm::vec3(lightIterator->second->getType(),
											  glm::cos(glm::radians(lightIterator->second->getSpotAngle())),
											  glm::cos(glm::radians(lightIterator->second->getSpotAngleOuter())));

		info[index].position = glm::vec4(lightIterator->second->transform.getPosition(), 0.0f);
		info[index].color = glm::vec4(lightIterator->second->getColor() * lightIterator->second->getIntensity(), 0.0f);
		info[index].direction = glm::vec4(lightIterator->second->getDirection(), 0.0f);
		info[index].lightSpace = lightIterator->second->shadowProjectionMatrix * lightIterator->second->getViewMatrixForShadowMap();

		index++;
		lightIterator++;
	}

	std::vector<std::string> materialsList = resourceManager.getMaterialList();
	for (size_t i = 0; i < materialsList.size(); i++)
	{
		FEMaterial* material = resourceManager.getMaterial(materialsList[i]);

		auto iteratorBlock = material->shader->blockUniforms.begin();
		while (iteratorBlock != material->shader->blockUniforms.end())
		{
			if (iteratorBlock->first.c_str() == std::string("lightInfo"))
			{
				glBindBuffer(GL_UNIFORM_BUFFER, iteratorBlock->second);
				auto lightIterator = scene.lightsMap.begin();
				int index = 0;
				while (lightIterator != scene.lightsMap.end())
				{
					glBufferSubData(GL_UNIFORM_BUFFER, index * 128 + 0, 16, &info[index].typeAndAngles);
					glBufferSubData(GL_UNIFORM_BUFFER, index * 128 + 16, 16, &info[index].position);
					glBufferSubData(GL_UNIFORM_BUFFER, index * 128 + 32, 16, &info[index].color);
					glBufferSubData(GL_UNIFORM_BUFFER, index * 128 + 48, 16, &info[index].direction);
					glBufferSubData(GL_UNIFORM_BUFFER, index * 128 + 64, 64, &info[index].lightSpace);

					index++;
					lightIterator++;
				}
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}

			iteratorBlock++;
		}
	}
}

void FERenderer::render(FEBasicCamera* currentCamera)
{
	FEScene& scene = FEScene::getInstance();

	loadUniformBlocks();
	
	// ********* GENERATE SHADOW MAPS *********
	auto itLight = scene.lightsMap.begin();
	while (itLight != scene.lightsMap.end())
	{
		if (itLight->second->isCastShadows())
		{
			if (itLight->second->getType() == FE_DIRECTIONAL_LIGHT)
			{
				glm::vec3 oldCameraPosition = currentCamera->getPosition();
				glm::mat4 oldViewMatrix = currentCamera->getViewMatrix();
				glm::mat4 oldProjectionMatrix = currentCamera->getProjectionMatrix();
				// put camera to the position of light
				// to-do: should put out of scene bounderies in case of direcctional light.
				currentCamera->projectionMatrix = itLight->second->shadowProjectionMatrix;
				currentCamera->viewMatrix = itLight->second->getViewMatrixForShadowMap();

				glViewport(0, 0, itLight->second->shadowMap->getWidth(), itLight->second->shadowMap->getHeight());

				itLight->second->shadowMap->bind();
				FE_GL_ERROR(glClear(GL_DEPTH_BUFFER_BIT));

				auto it = scene.entityMap.begin();
				while (it != scene.entityMap.end())
				{
					auto entity = it->second;
					if (!entity->isCastShadows())
					{
						it++;
						continue;
					}
					
					FEMaterial* originamlMaterial = entity->material;
					entity->material = shadowMapMaterial;
					//#fix do it only if albedoHasAlpha
					shadowMapMaterial->albedoMap = originamlMaterial->albedoMap;

					renderEntity(entity, currentCamera);

					shadowMapMaterial->albedoMap = nullptr;
					entity->material = originamlMaterial;

					it++;
				}

				itLight->second->shadowMap->unBind();

				currentCamera->setPosition(oldCameraPosition);
				currentCamera->viewMatrix = oldViewMatrix;
				currentCamera->projectionMatrix = oldProjectionMatrix;

				glViewport(0, 0, sceneToTextureFB->getWidth(), sceneToTextureFB->getHeight());

				// in current version only shadows from one directional light is supported.
				shadowMap = itLight->second->shadowMap->getDepthAttachment();
				break;
			}
		}

		itLight++;
	}
	// ********* GENERATE SHADOW MAPS END *********

	// in current version only shadows from one directional light is supported.
	if (shadowMap) shadowMap->bind(FE_SHADOW_MAP_UNIT);

	// ********* RENDER SCENE *********
	sceneToTextureFB->bind();
	FE_GL_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	auto it = scene.entityMap.begin();
	while (it != scene.entityMap.end())
	{
		auto entity = it->second;
		renderEntity(entity, currentCamera);
		it++;
	}
	
	sceneToTextureFB->unBind();
	// ********* RENDER SCENE END *********

	// ********* POST_PROCESS EFFECTS *********
	FETexture* prevStageTex = sceneToTextureFB->getColorAttachment();

	for (size_t i = 0; i < postProcessEffects.size(); i++)
	{
		FEPostProcess& effect = *postProcessEffects[i];
		for (size_t j = 0; j < effect.stages.size(); j++)
		{
			effect.stages[j]->shader->start();
			loadStandardParams(effect.stages[j]->shader, currentCamera, nullptr);
			effect.stages[j]->shader->loadDataToGPU();

			for (size_t k = 0; k < effect.stages[j]->inTextureSource.size(); k++)
			{
				if (effect.stages[j]->inTextureSource[k] == FEPP_PREVIOUS_STAGE_RESULT0)
				{
					effect.stages[j]->inTexture[k] = prevStageTex;
					effect.stages[j]->inTexture[k]->bind(k);
				}
				else if (effect.stages[j]->inTextureSource[k] == FEPP_SCENE_HDR_COLOR)
				{
					effect.stages[j]->inTexture[k] = sceneToTextureFB->getColorAttachment();
					effect.stages[j]->inTexture[k]->bind(k);
				}
				else if (effect.stages[j]->inTextureSource[k] == FEPP_SCENE_DEPTH)
				{
					effect.stages[j]->inTexture[k] = sceneToTextureFB->getDepthAttachment();
					effect.stages[j]->inTexture[k]->bind(k);
				}
				else if (effect.stages[j]->inTextureSource[k] == FEPP_OWN_TEXTURE)
				{
					effect.stages[j]->inTexture[k]->bind(k);
				}
			}

			effect.intermediateFramebuffer->setColorAttachment(effect.stages[j]->outTexture);
			effect.intermediateFramebuffer->bind();

			FE_GL_ERROR(glBindVertexArray(effect.screenQuad->getVaoID()));
			FE_GL_ERROR(glEnableVertexAttribArray(0));
			FE_GL_ERROR(glDrawElements(GL_TRIANGLES, effect.screenQuad->getVertexCount(), GL_UNSIGNED_INT, 0));
			FE_GL_ERROR(glDisableVertexAttribArray(0));
			FE_GL_ERROR(glBindVertexArray(0));

			effect.intermediateFramebuffer->unBind();

			for (size_t k = 0; k < effect.stages[j]->inTextureSource.size(); k++)
			{
				effect.stages[j]->inTexture[k]->unBind();
			}
			effect.stages[j]->shader->stop();

			prevStageTex = effect.stages[j]->outTexture;
		}
	}

	for (int i = postProcessEffects.size() - 1; i >= 0; i--)
	{
		FEPostProcess& effect = *postProcessEffects[i];
		
		if (effect.active)
		{
			effect.renderResult();
			break;
		}
	}
	// ********* SCREEN SPACE EFFECTS END *********
}

FEPostProcess* FERenderer::getPostProcessEffect(std::string name)
{
	for (size_t i = 0; i < postProcessEffects.size(); i++)
	{
		if (postProcessEffects[i]->getName() == name)
			return postProcessEffects[i];
	}

	return nullptr;
}

std::vector<std::string> FERenderer::getPostProcessList()
{
	std::vector<std::string> result;
	for (size_t i = 0; i < postProcessEffects.size(); i++)
		result.push_back(postProcessEffects[i]->getName());
		
	return result;
}

void FERenderer::takeScreenshot(const char* fileName, int width, int height)
{
	char* pixels = new char[4 * width * height];
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, postProcessEffects.back()->stages.back()->outTexture->getTextureID());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	//with glGetTexImage we get upside down image data, so we flip it.
	for (size_t j = 0; j < size_t(height / 2); j++)
	{
		for (size_t i = 0; i < size_t(4 * width); i+=4)
		{
			std::swap(pixels[i + (j * 4 * width)], pixels[4 * width * height - (i + (j * 4 * width)) - 4]);
			std::swap(pixels[i + 1 + (j * 4 * width)], pixels[4 * width * height - (i - 1 + (j * 4 * width)) - 4]);
			std::swap(pixels[i + 2 + (j * 4 * width)], pixels[4 * width * height - (i - 2 + (j * 4 * width)) - 4]);
			std::swap(pixels[i + 3 + (j * 4 * width)], pixels[4 * width * height - (i - 3 + (j * 4 * width)) - 4]);
		}
	}
	
	FEResourceManager::getInstance().saveFETexture(fileName, pixels, width, height);
	delete[] pixels;
}

void FERenderer::renderEntity(FEEntity* entity, FEBasicCamera* currentCamera)
{
	entity->material->bind();
	loadStandardParams(entity->material->shader, currentCamera, entity);
	entity->material->shader->loadDataToGPU();
	entity->render();
	entity->material->unBind();
}