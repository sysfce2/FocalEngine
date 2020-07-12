#pragma once

#include "../ResourceManager/FEResourceManager.h"
#include "../CoreExtensions/FEFreeCamera.h"
#include "../Renderer/FELight.h"

namespace FocalEngine
{
	class FERenderer;
	class FEScene
	{
		friend FERenderer;
	public:
		SINGLETON_PUBLIC_PART(FEScene)

		FEEntity* addEntity(FEGameModel* gameModel, std::string Name = "");
		bool addEntity(FEEntity* newEntity);
		FEEntity* getEntity(std::string name);
		std::vector<std::string> getEntityList();
		void deleteEntity(std::string name);
		bool setEntityName(FEEntity* Entity, std::string EntityName);

		void addLight(FELightType Type, std::string Name);
		FELight* getLight(std::string name);
		std::vector<std::string> getLightsList();

		bool addTerrain(FETerrain* newTerrain);
		std::vector<std::string> getTerrainList();

		void prepareForGameModelDeletion(FEGameModel* gameModel);

		void clear();
	private:
		SINGLETON_PRIVATE_PART(FEScene)

		std::unordered_map<std::string, FEEntity*> entityMap;
		std::unordered_map<std::string, FELight*> lightsMap;
		std::unordered_map<std::string, FETerrain*> terrainMap;
	};
}