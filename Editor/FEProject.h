#pragma once

#include "../FEngine.h"
using namespace FocalEngine;

#define ENGINE FEngine::getInstance()

class FEProject
{
public:
	FEProject(std::string Name, std::string ProjectFolder);
	~FEProject();

	std::string getName();
	void setName(std::string newName);

	std::string getProjectFolder();
	void saveScene(std::unordered_map<int, FEEntity*>* excludedEntities);
	void loadScene();

	FETexture* sceneScreenshot;
	bool modified = false;
	void createDummyScreenshot();

	void addFileToDeleteList(std::string fileName);
private:
	std::string name;
	std::string projectFolder;

	void writeTransformToJSON(Json::Value& root, FETransformComponent* transform);
	void readTransformToJSON(Json::Value& root, FETransformComponent* transform);

	std::vector<std::string> filesToDelete;
};