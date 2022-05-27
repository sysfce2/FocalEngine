#include "../Editor/FEEditor.h"

static FETexture* tempTexture = nullptr;
static void addTransparencyToTextureCallBack(std::vector<FEObject*> selectionsResult)
{
	if (tempTexture == nullptr)
		return;

	if (selectionsResult.size() == 1 && selectionsResult[0]->getType() == FE_TEXTURE)
	{
		FETexture* originalTexture = reinterpret_cast<FETexture*>(tempTexture);

		FETexture* newTexture = RESOURCE_MANAGER.createTextureWithTransparency(originalTexture, reinterpret_cast<FETexture*>(selectionsResult[0]));
		if (newTexture == nullptr)
		{
			tempTexture = nullptr;
			return;
		}

		unsigned char* newRawData = RESOURCE_MANAGER.getFETextureRawData(newTexture);
		int maxDimention = std::max(originalTexture->getWidth(), originalTexture->getHeight());
		size_t mipCount = size_t(floor(log2(maxDimention)) + 1);
		RESOURCE_MANAGER.updateFETextureRawData(originalTexture, newRawData, mipCount);
		FE_GL_ERROR(glGenerateMipmap(GL_TEXTURE_2D));
		FE_GL_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f));
		FE_GL_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));
		PROJECT_MANAGER.getCurrent()->addUnSavedObject(originalTexture);

		delete[] newRawData;
		RESOURCE_MANAGER.deleteFETexture(newTexture);
	}

	tempTexture = nullptr;
}

static void createNewPrefabCallBack(std::vector<FEObject*> selectionsResult)
{
	if (selectionsResult.size() > 0 && selectionsResult[0]->getType() == FE_GAMEMODEL)
	{
		FEPrefab* newPrefab = RESOURCE_MANAGER.createPrefab(reinterpret_cast<FEGameModel*>(selectionsResult[0]));

		if (selectionsResult.size() > 1)
		{
			for (int i = 1; i < selectionsResult.size(); i++)
			{
				if (selectionsResult[i]->getType() == FE_GAMEMODEL)
					newPrefab->addComponent(reinterpret_cast<FEGameModel*>(selectionsResult[i]));
			}
		}
		
		PROJECT_MANAGER.getCurrent()->setModified(true);
		VIRTUAL_FILE_SYSTEM.createFile(newPrefab, VIRTUAL_FILE_SYSTEM.getCurrentPath());
	}
}

static int itemIconSide = 128 + 8;
void FEEditor::displayContentBrowser()
{
	if (!contentBrowserVisible)
		return;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
	ImGui::Begin("Content Browser", nullptr, ImGuiWindowFlags_None);

	displayContentBrowserItems();

	bool open_context_menu = false;
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
		open_context_menu = true;

	if (open_context_menu)
		ImGui::OpenPopup("##context_menu");

	isOpenContextMenuInContentBrowser = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
	if (ImGui::BeginPopup("##context_menu"))
	{
		isOpenContextMenuInContentBrowser = true;

		if (contentBrowserItemUnderMouse == -1)
		{
			if (ImGui::MenuItem("Import Asset..."))
			{
				std::string filePath = "";
				FILE_SYSTEM.showFileOpenDialog(filePath, allImportLoadFilter, 3);
				if (filePath != "")
				{
					std::vector<FEObject*> loadedObjects = RESOURCE_MANAGER.importAsset(filePath.c_str());
					for (size_t i = 0; i < loadedObjects.size(); i++)
					{
						if (loadedObjects[i] != nullptr)
						{
							VIRTUAL_FILE_SYSTEM.createFile(reinterpret_cast<FEMesh*>(loadedObjects[i]), VIRTUAL_FILE_SYSTEM.getCurrentPath());
							PROJECT_MANAGER.getCurrent()->setModified(true);
							PROJECT_MANAGER.getCurrent()->addUnSavedObject(loadedObjects[i]);
						}
					}
				}
			}

			if (ImGui::MenuItem("*DEPRECATED*Import Tree..."))
			{
				std::string filePath = "";
				FILE_SYSTEM.showFileOpenDialog(filePath, OBJLoadFilter, 1);
				if (filePath != "")
				{
					std::vector<FEObject*> loadedObjects = RESOURCE_MANAGER.importOBJ(filePath.c_str(), true);
					for (size_t i = 0; i < loadedObjects.size(); i++)
					{
						if (loadedObjects[i] != nullptr)
						{
							VIRTUAL_FILE_SYSTEM.createFile(reinterpret_cast<FEMesh*>(loadedObjects[i]), VIRTUAL_FILE_SYSTEM.getCurrentPath());
							PROJECT_MANAGER.getCurrent()->setModified(true);
							PROJECT_MANAGER.getCurrent()->addUnSavedObject(loadedObjects[i]);
						}
					}
				}
			}

			if (ImGui::BeginMenu("Add"))
			{
				if (ImGui::MenuItem("Add folder"))
				{
					std::string newDirectoryName = VIRTUAL_FILE_SYSTEM.createDirectory(VIRTUAL_FILE_SYSTEM.getCurrentPath());
					updateDirectoryDragAndDropTargets();

					for (size_t i = 0; i < filteredResourcesContentBrowser.size(); i++)
					{
						if (filteredResourcesContentBrowser[i]->getName() == newDirectoryName)
						{
							contentBrowserRenameIndex = int(i);
							strcpy_s(contentBrowserRename, filteredResourcesContentBrowser[i]->getName().size() + 1, filteredResourcesContentBrowser[i]->getName().c_str());
							lastFrameRenameEditWasVisiable = false;
							break;
						}
					}
				}

				if (ImGui::BeginMenu("Texture"))
				{
					if (ImGui::MenuItem("Combine channels..."))
						CombineChannelsToTexturePopUp::getInstance().show("");

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Create new material"))
				{
					FEMaterial* newMat = RESOURCE_MANAGER.createMaterial("");
					if (newMat)
					{
						PROJECT_MANAGER.getCurrent()->setModified(true);
						newMat->shader = RESOURCE_MANAGER.getShader("0800253C242B05321A332D09"/*"FEPBRShader"*/);

						newMat->setAlbedoMap(RESOURCE_MANAGER.noTexture);

						VIRTUAL_FILE_SYSTEM.createFile(newMat, VIRTUAL_FILE_SYSTEM.getCurrentPath());
					}
				}

				if (ImGui::MenuItem("Create new game model"))
				{
					FEGameModel* newGameModel = RESOURCE_MANAGER.createGameModel();
					PROJECT_MANAGER.getCurrent()->setModified(true);
					VIRTUAL_FILE_SYSTEM.createFile(newGameModel, VIRTUAL_FILE_SYSTEM.getCurrentPath());
				}

				if (ImGui::MenuItem("Create new prefab"))
				{
					selectFEObjectPopUp::getInstance().show(FE_GAMEMODEL, createNewPrefabCallBack);
				}

				ImGui::EndMenu();
			}
		}
		else
		{
			std::string fullPath = VIRTUAL_FILE_SYSTEM.getCurrentPath();
			if (fullPath.back() != '/')
				fullPath += '/';
			fullPath += filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getName();

			bool readOnlyItem = VIRTUAL_FILE_SYSTEM.isReadOnly(filteredResourcesContentBrowser[contentBrowserItemUnderMouse], fullPath);

			if (readOnlyItem)
				ImGui::MenuItem("Read Only");
			
			if (!readOnlyItem)
			{
				if (ImGui::MenuItem("Rename"))
				{
					contentBrowserRenameIndex = contentBrowserItemUnderMouse;

					strcpy_s(contentBrowserRename, filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getName().size() + 1, filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getName().c_str());
					lastFrameRenameEditWasVisiable = false;
				}
			}

			if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_MATERIAL)
			{
				if (ImGui::MenuItem("Edit"))
				{
					editMaterialPopup::getInstance().show(RESOURCE_MANAGER.getMaterial(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
				}
			}

			if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_GAMEMODEL)
			{
				if (ImGui::MenuItem("Edit"))
				{
					editGameModelPopup::getInstance().show(RESOURCE_MANAGER.getGameModel(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
				}

				if (ImGui::MenuItem("Create Prefab out of this Game Model"))
				{
					FEPrefab* newPrefab = RESOURCE_MANAGER.createPrefab(RESOURCE_MANAGER.getGameModel(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
					PROJECT_MANAGER.getCurrent()->setModified(true);
					VIRTUAL_FILE_SYSTEM.createFile(newPrefab, VIRTUAL_FILE_SYSTEM.getCurrentPath());
				}
			}

			if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_PREFAB)
			{
				if (ImGui::MenuItem("Edit"))
				{
					prefabEditorWindow::getInstance().show(RESOURCE_MANAGER.getPrefab(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
				}
			}

			if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_SHADER)
			{
				if (ImGui::MenuItem("Edit"))
				{
					shaderEditorWindow::getInstance().show(RESOURCE_MANAGER.getShader(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
				}
			}

			if (!readOnlyItem)
			{
				if (ImGui::MenuItem("Delete"))
				{
					if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_NULL)
					{
						deleteDirectoryPopup::getInstance().show(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getName());
					}
					else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_MESH)
					{
						deleteMeshPopup::getInstance().show(RESOURCE_MANAGER.getMesh(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
					}
					else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_TEXTURE)
					{
						deleteTexturePopup::getInstance().show(RESOURCE_MANAGER.getTexture(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
					}
					else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_MATERIAL)
					{
						deleteMaterialPopup::getInstance().show(RESOURCE_MANAGER.getMaterial(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
					}
					else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_GAMEMODEL)
					{
						deleteGameModelPopup::getInstance().show(RESOURCE_MANAGER.getGameModel(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
					}
					else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_PREFAB)
					{
						deletePrefabPopup::getInstance().show(RESOURCE_MANAGER.getPrefab(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
					}
				}
			}

			if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_TEXTURE)
			{
				if (ImGui::BeginMenu("Convert"))
				{
					if (ImGui::MenuItem("Texture channels to individual textures"))
					{
						std::vector<FETexture*> newTextures = RESOURCE_MANAGER.channelsToFETextures(RESOURCE_MANAGER.getTexture(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));

						PROJECT_MANAGER.getCurrent()->addUnSavedObject(newTextures[0]);
						PROJECT_MANAGER.getCurrent()->addUnSavedObject(newTextures[1]);
						PROJECT_MANAGER.getCurrent()->addUnSavedObject(newTextures[2]);
						PROJECT_MANAGER.getCurrent()->addUnSavedObject(newTextures[3]);

						PROJECT_MANAGER.getCurrent()->setModified(true);

						VIRTUAL_FILE_SYSTEM.createFile(newTextures[0], VIRTUAL_FILE_SYSTEM.getCurrentPath());
						VIRTUAL_FILE_SYSTEM.createFile(newTextures[1], VIRTUAL_FILE_SYSTEM.getCurrentPath());
						VIRTUAL_FILE_SYSTEM.createFile(newTextures[2], VIRTUAL_FILE_SYSTEM.getCurrentPath());
						VIRTUAL_FILE_SYSTEM.createFile(newTextures[3], VIRTUAL_FILE_SYSTEM.getCurrentPath());
					}

					if (ImGui::MenuItem("Resize"))
					{
						FETexture* textureToResize = RESOURCE_MANAGER.getTexture(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID());
						resizeTexturePopup::getInstance().show(textureToResize);
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Add transparency"))
				{
					if (ImGui::MenuItem("Choose transparency mask"))
					{
						tempTexture = reinterpret_cast<FETexture*>(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]);
						selectFEObjectPopUp::getInstance().show(FE_TEXTURE, addTransparencyToTextureCallBack);
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Export"))
				{
					FETexture* textureToExport = RESOURCE_MANAGER.getTexture(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID());

					if (ImGui::MenuItem("as PNG"))
					{
						std::string filePath = "";
						FILE_SYSTEM.showFileSaveDialog(filePath, textureLoadFilter, 1);

						if (filePath != "")
						{
							filePath += ".png";
							bool result = RESOURCE_MANAGER.exportFETextureToPNG(textureToExport, filePath.c_str());
						}
					}

					ImGui::EndMenu();
				}
			}
		}

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::End();
}

void FEEditor::displayContentBrowserItems()
{
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::ImColor(0.95f, 0.90f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::ImColor(0.1f, 1.0f, 0.1f, 1.0f));

	float currentY = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY(currentY);
	if (ImGui::ImageButton((void*)(intptr_t)VFSBackIcon->getTextureID(), ImVec2(64, 64), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 8, ImColor(0.0f, 0.0f, 0.0f, 0.0f), ImColor(1.0f, 1.0f, 1.0f, 1.0f)))
	{
		VIRTUAL_FILE_SYSTEM.setCurrentPath(VIRTUAL_FILE_SYSTEM.getDirectoryParent(VIRTUAL_FILE_SYSTEM.getCurrentPath()));
		strcpy_s(filterForResourcesContentBrowser, "");
	}
	VFSBackButtonTarget->stickToItem();

	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();

	static std::string lastFramePath = "";
	allResourcesContentBrowser.clear();
	allResourcesContentBrowser = VIRTUAL_FILE_SYSTEM.getDirectoryContent(VIRTUAL_FILE_SYSTEM.getCurrentPath());

	ImGui::SetCursorPosX(100);
	ImGui::SetCursorPosY(currentY + 5);
	ImGui::Text("Filter by type: ");

	ImGui::SetCursorPosX(120 + 140);
	ImGui::SetCursorPosY(currentY - 5);
	objTypeFilterForResourcesContentBrowser == "" ? setSelectedStyle(filterAllTypesButton) : setDefaultStyle(filterAllTypesButton);
	filterAllTypesButton->render();
	if (filterAllTypesButton->getWasClicked())
		objTypeFilterForResourcesContentBrowser = "";

	ImGui::SetCursorPosX(120 + 140 + 48);
	ImGui::SetCursorPosY(currentY - 5);
	objTypeFilterForResourcesContentBrowser == FEObjectTypeToString(FE_TEXTURE) ? setSelectedStyle(filterTextureTypeButton) : setDefaultStyle(filterTextureTypeButton);
	filterTextureTypeButton->render();
	if (filterTextureTypeButton->getWasClicked())
		objTypeFilterForResourcesContentBrowser = FEObjectTypeToString(FE_TEXTURE);

	ImGui::SetCursorPosX(120 + 140 + 48 + 48);
	ImGui::SetCursorPosY(currentY - 5);
	objTypeFilterForResourcesContentBrowser == FEObjectTypeToString(FE_MESH) ? setSelectedStyle(filterMeshTypeButton) : setDefaultStyle(filterMeshTypeButton);
	filterMeshTypeButton->render();
	if (filterMeshTypeButton->getWasClicked())
		objTypeFilterForResourcesContentBrowser = FEObjectTypeToString(FE_MESH);

	ImGui::SetCursorPosX(120 + 140 + 48 + 48 + 48);
	ImGui::SetCursorPosY(currentY - 5);
	objTypeFilterForResourcesContentBrowser == FEObjectTypeToString(FE_MATERIAL) ? setSelectedStyle(filterMaterialTypeButton) : setDefaultStyle(filterMaterialTypeButton);
	filterMaterialTypeButton->render();
	if (filterMaterialTypeButton->getWasClicked())
		objTypeFilterForResourcesContentBrowser = FEObjectTypeToString(FE_MATERIAL);

	ImGui::SetCursorPosX(120 + 140 + 48 + 48 + 48 + 48);
	ImGui::SetCursorPosY(currentY - 5);
	objTypeFilterForResourcesContentBrowser == FEObjectTypeToString(FE_GAMEMODEL) ? setSelectedStyle(filterGameModelTypeButton) : setDefaultStyle(filterGameModelTypeButton);
	filterGameModelTypeButton->render();
	if (filterGameModelTypeButton->getWasClicked())
		objTypeFilterForResourcesContentBrowser = FEObjectTypeToString(FE_GAMEMODEL);

	ImGui::SetCursorPosX(120 + 140 + 48 + 48 + 48 + 48 + 48);
	ImGui::SetCursorPosY(currentY - 5);
	objTypeFilterForResourcesContentBrowser == FEObjectTypeToString(FE_PREFAB) ? setSelectedStyle(filterPrefabTypeButton) : setDefaultStyle(filterPrefabTypeButton);
	filterPrefabTypeButton->render();
	if (filterPrefabTypeButton->getWasClicked())
		objTypeFilterForResourcesContentBrowser = FEObjectTypeToString(FE_PREFAB);

	ImGui::SetCursorPosX(100);
	ImGui::SetCursorPosY(currentY + 50);
	ImGui::Text("Filter by name: ");

	ImGui::SetCursorPosX(120 + 140);
	ImGui::SetCursorPosY(currentY + 47);
	ImGui::InputText("##filter", filterForResourcesContentBrowser, IM_ARRAYSIZE(filterForResourcesContentBrowser));

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 25);
	ImGui::Separator();

	updateFilterForResourcesContentBrowser();
	updateDirectoryDragAndDropTargets();

	// ************** Drag&Drop **************
	if (VIRTUAL_FILE_SYSTEM.getCurrentPath() != lastFramePath)
	{
		lastFramePath = VIRTUAL_FILE_SYSTEM.getCurrentPath();
		updateDirectoryDragAndDropTargets();
	}
	int directoryIndex = 0;
	// ************** Drag&Drop END **************

	int iconsPerWindowWidth = (int)ImGui::GetCurrentContext()->CurrentWindow->Rect().GetWidth() / (itemIconSide + 32);
	// Possibly window is minimized anyway ImGui::Columns can't take 0 as columns count!
	if (iconsPerWindowWidth == 0)
		return;

	if (!isOpenContextMenuInContentBrowser) contentBrowserItemUnderMouse = -1;
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
	ImGui::Columns(iconsPerWindowWidth, "mycolumns3", false);

	for (size_t i = 0; i < filteredResourcesContentBrowser.size(); i++)
	{
		ImGui::PushID(int(std::hash<std::string>{}(filteredResourcesContentBrowser[i]->getObjectID())));
		
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::ImColor(0.95f, 0.90f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::ImColor(0.1f, 1.0f, 0.1f, 1.0f));
		
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);

		FETexture* previewTexture = nullptr;
		FETexture* smallAdditionTypeIcon = nullptr;

		if (filteredResourcesContentBrowser[i]->getType() == FE_NULL)
		{
			uv0 = ImVec2(0.0f, 0.0f);
			uv1 = ImVec2(1.0f, 1.0f);

			previewTexture = folderIcon;

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::ImColor(0.95f, 0.90f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::ImColor(0.1f, 1.0f, 0.1f, 1.0f));
		}
		if (filteredResourcesContentBrowser[i]->getType() == FE_SHADER)
		{
			uv0 = ImVec2(0.0f, 0.0f);
			uv1 = ImVec2(1.0f, 1.0f);

			previewTexture = shaderIcon;

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.5f, 0.5f, 0.5f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::ImColor(0.95f, 0.90f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::ImColor(0.1f, 1.0f, 0.1f, 1.0f));
		}
		else if (filteredResourcesContentBrowser[i]->getType() == FE_MESH)
		{
			uv0 = ImVec2(0.0f, 1.0f);
			uv1 = ImVec2(1.0f, 0.0f);

			previewTexture = PREVIEW_MANAGER.getMeshPreview(filteredResourcesContentBrowser[i]->getObjectID());
			smallAdditionTypeIcon = meshContentBrowserIcon;
		}
		else if (filteredResourcesContentBrowser[i]->getType() == FE_TEXTURE)
		{
			previewTexture = RESOURCE_MANAGER.getTexture(filteredResourcesContentBrowser[i]->getObjectID());
			smallAdditionTypeIcon = textureContentBrowserIcon;
		}
		else if (filteredResourcesContentBrowser[i]->getType() == FE_MATERIAL)
		{
			uv0 = ImVec2(0.0f, 1.0f);
			uv1 = ImVec2(1.0f, 0.0f);

			previewTexture = PREVIEW_MANAGER.getMaterialPreview(filteredResourcesContentBrowser[i]->getObjectID());
			smallAdditionTypeIcon = materialContentBrowserIcon;
		}
		else if (filteredResourcesContentBrowser[i]->getType() == FE_GAMEMODEL)
		{
			uv0 = ImVec2(0.0f, 1.0f);
			uv1 = ImVec2(1.0f, 0.0f);

			previewTexture = PREVIEW_MANAGER.getGameModelPreview(filteredResourcesContentBrowser[i]->getObjectID());
			smallAdditionTypeIcon = gameModelContentBrowserIcon;
		}
		else if (filteredResourcesContentBrowser[i]->getType() == FE_PREFAB)
		{
			uv0 = ImVec2(0.0f, 1.0f);
			uv1 = ImVec2(1.0f, 0.0f);

			previewTexture = PREVIEW_MANAGER.getPrefabPreview(filteredResourcesContentBrowser[i]->getObjectID());
			smallAdditionTypeIcon = prefabContentBrowserIcon;
		}

		if (previewTexture != nullptr)
			ImGui::ImageButton((void*)(intptr_t)previewTexture->getTextureID(), ImVec2(128, 128), uv0, uv1, 8, ImColor(0.0f, 0.0f, 0.0f, 0.0f), ImColor(1.0f, 1.0f, 1.0f, 1.0f));
		
		if (filteredResourcesContentBrowser[i]->getType() == FE_NULL && contentBrowserDirectoriesTargets.size() > (size_t)directoryIndex)
			contentBrowserDirectoriesTargets[directoryIndex++]->stickToItem();

		if (ImGui::IsItemHovered())
		{
			if (!isOpenContextMenuInContentBrowser && !DRAG_AND_DROP_MANAGER.objectIsDraged())
			{
				std::string additionalTypeInfo = "";
				if (filteredResourcesContentBrowser[i]->getType() == FE_TEXTURE)
				{
					additionalTypeInfo += "\nTexture type: ";
					additionalTypeInfo += FETexture::textureInternalFormatToString(RESOURCE_MANAGER.getTexture(filteredResourcesContentBrowser[i]->getObjectID())->getInternalFormat());
				}

				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(("Name: " + filteredResourcesContentBrowser[i]->getName() +
										"\nType: " + FEObjectTypeToString(filteredResourcesContentBrowser[i]->getType()) +
										additionalTypeInfo +
										"\nPath: " + VIRTUAL_FILE_SYSTEM.getCurrentPath()
										).c_str());
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();

				contentBrowserItemUnderMouse = int(i);

				if (ImGui::IsMouseDragging(0))
					DRAG_AND_DROP_MANAGER.setObject(filteredResourcesContentBrowser[i], previewTexture, uv0, uv1);
			}
		}

		if (smallAdditionTypeIcon != nullptr)
		{
			ImVec2 cursorPosBefore = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cursorPosBefore.x + 10, cursorPosBefore.y - 48));
			ImGui::Image((void*)(intptr_t)smallAdditionTypeIcon->getTextureID(), ImVec2(32, 32));
			ImGui::SetCursorPos(cursorPosBefore);
		}
			

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		ImGui::PopID();
		
		if (contentBrowserRenameIndex == i)
		{
			if (!lastFrameRenameEditWasVisiable)
			{
				ImGui::SetKeyboardFocusHere(0);
				ImGui::SetFocusID(ImGui::GetID("##newNameEditor"), ImGui::GetCurrentWindow());
				ImGui::SetItemDefaultFocus();
				lastFrameRenameEditWasVisiable = true;
			}

			ImGui::SetNextItemWidth(itemIconSide + 8.0f);
			if (ImGui::InputText("##newNameEditor", contentBrowserRename, IM_ARRAYSIZE(contentBrowserRename), ImGuiInputTextFlags_EnterReturnsTrue) ||
				ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered() || ImGui::GetFocusID() != ImGui::GetID("##newNameEditor"))
			{
				if (filteredResourcesContentBrowser[contentBrowserRenameIndex]->getType() == FE_NULL)
				{
					std::string pathToDirectory = VIRTUAL_FILE_SYSTEM.getCurrentPath();
					if (pathToDirectory.back() != '/')
						pathToDirectory += '/';

					pathToDirectory += filteredResourcesContentBrowser[contentBrowserRenameIndex]->getName();
					VIRTUAL_FILE_SYSTEM.renameDirectory(contentBrowserRename, pathToDirectory);

					updateDirectoryDragAndDropTargets();
				}
				else
				{
					filteredResourcesContentBrowser[contentBrowserRenameIndex]->setDirtyFlag(true);
					PROJECT_MANAGER.getCurrent()->setModified(true);
					filteredResourcesContentBrowser[contentBrowserRenameIndex]->setName(contentBrowserRename);
				}
				
				contentBrowserRenameIndex = -1;
			}
		}
		else
		{
			ImVec2 textSize = ImGui::CalcTextSize(filteredResourcesContentBrowser[i]->getName().c_str());
			if (textSize.x < itemIconSide + 8)
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (itemIconSide + 8.0f) / 2.0f - textSize.x / 2.0f);
				ImGui::Text(filteredResourcesContentBrowser[i]->getName().c_str());
			}
			else
			{
				ImGui::Text(filteredResourcesContentBrowser[i]->getName().c_str());
			}
		}
		
		ImGui::NextColumn();
	}

	if (ImGui::IsMouseDoubleClicked(0) && contentBrowserItemUnderMouse != -1)
	{
		if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_NULL)
		{
			std::string currentPath = VIRTUAL_FILE_SYSTEM.getCurrentPath();
			if (currentPath.back() != '/')
				currentPath += '/';

			currentPath += filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getName();
			VIRTUAL_FILE_SYSTEM.setCurrentPath(currentPath);
		}
		else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_MESH)
		{
			std::string meshInfo = "Vertex count: ";
			meshInfo += std::to_string(RESOURCE_MANAGER.getMesh(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID())->getVertexCount());
			meshInfo += "\n";
			meshInfo += "Sub material socket: ";
			meshInfo += RESOURCE_MANAGER.getMesh(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID())->getMaterialCount() == 2 ? "Yes" : "No";
			messagePopUp::getInstance().show("Mesh info", meshInfo.c_str());
		}
		else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_MATERIAL)
		{
			editMaterialPopup::getInstance().show(RESOURCE_MANAGER.getMaterial(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
		}
		else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_GAMEMODEL)
		{
			if (!isOpenContextMenuInContentBrowser && !editGameModelPopup::getInstance().isVisible())
			{
				editGameModelPopup::getInstance().show(RESOURCE_MANAGER.getGameModel(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
			}
		}
		else if (filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getType() == FE_PREFAB)
		{
			if (!isOpenContextMenuInContentBrowser && !prefabEditorWindow::getInstance().isVisible())
			{
				prefabEditorWindow::getInstance().show(RESOURCE_MANAGER.getPrefab(filteredResourcesContentBrowser[contentBrowserItemUnderMouse]->getObjectID()));
			}
		}
	}

	ImGui::Columns(1);
}

void FEEditor::updateDirectoryDragAndDropTargets()
{
	contentBrowserDirectoriesTargets.clear();
	directoryDragAndDropInfo.clear();
	allResourcesContentBrowser.clear();
	allResourcesContentBrowser = VIRTUAL_FILE_SYSTEM.getDirectoryContent(VIRTUAL_FILE_SYSTEM.getCurrentPath());

	updateFilterForResourcesContentBrowser();

	directoryDragAndDropInfo.resize(VIRTUAL_FILE_SYSTEM.subDirectoriesCount(VIRTUAL_FILE_SYSTEM.getCurrentPath()));
	int subDirectoryIndex = 0;
	for (size_t i = 0; i < filteredResourcesContentBrowser.size(); i++)
	{
		if (filteredResourcesContentBrowser[i]->getType() == FE_NULL)
		{
			directoryDragAndDropCallbackInfo info;

			info.directoryPath = VIRTUAL_FILE_SYSTEM.getCurrentPath();
			if (VIRTUAL_FILE_SYSTEM.getCurrentPath().back() != '/')
				info.directoryPath += "/";

			info.directoryPath += filteredResourcesContentBrowser[i]->getName() + "/";
			directoryDragAndDropInfo[subDirectoryIndex] = info;

			contentBrowserDirectoriesTargets.push_back(DRAG_AND_DROP_MANAGER.addTarget(std::vector<FEObjectType> { FE_NULL, FE_SHADER, FE_TEXTURE, FE_MESH, FE_MATERIAL, FE_GAMEMODEL, FE_PREFAB },
				directoryDragAndDropCallback, reinterpret_cast<void**>(&directoryDragAndDropInfo[subDirectoryIndex]),
				std::vector<std::string> { "Drop to move to folder", "Drop to move to folder", "Drop to move to folder", "Drop to move to folder", "Drop to move to folder", "Drop to move to folder", "Drop to move to folder" }));
			subDirectoryIndex++;
		}
	}

	if (VFSBackButtonTarget == nullptr)
	{
		VFSBackButtoninfo.directoryPath = VIRTUAL_FILE_SYSTEM.getDirectoryParent(VIRTUAL_FILE_SYSTEM.getCurrentPath());
		VFSBackButtonTarget = DRAG_AND_DROP_MANAGER.addTarget(std::vector<FEObjectType> { FE_NULL, FE_SHADER, FE_TEXTURE, FE_MESH, FE_MATERIAL, FE_GAMEMODEL, FE_PREFAB },
			directoryDragAndDropCallback, reinterpret_cast<void**>(&VFSBackButtoninfo),
			std::vector<std::string> { "Drop to move to parent folder", "Drop to move to parent folder", "Drop to move to parent folder", "Drop to move to parent folder", "Drop to move to parent folder", "Drop to move to parent folder", "Drop to move to parent folder" });
	}
	else
	{
		VFSBackButtoninfo.directoryPath = VIRTUAL_FILE_SYSTEM.getDirectoryParent(VIRTUAL_FILE_SYSTEM.getCurrentPath());
		//VFSBackButton->setNewUserData();
	}
}

void FEEditor::updateFilterForResourcesContentBrowser()
{
	filteredResourcesContentBrowser.clear();

	// Filter by name.
	if (strlen(filterForResourcesContentBrowser) == 0)
	{
		filteredResourcesContentBrowser = allResourcesContentBrowser;
	}
	else
	{
		for (size_t i = 0; i < allResourcesContentBrowser.size(); i++)
		{
			if (allResourcesContentBrowser[i]->getName().find(filterForResourcesContentBrowser) != -1)
			{
				filteredResourcesContentBrowser.push_back(allResourcesContentBrowser[i]);
			}
		}
	}

	// Filter by type.
	if (objTypeFilterForResourcesContentBrowser != "")
	{
		std::vector<FEObject*> finalFilteredList;
		for (size_t i = 0; i < filteredResourcesContentBrowser.size(); i++)
		{
			if (FEObjectTypeToString(filteredResourcesContentBrowser[i]->getType()) == objTypeFilterForResourcesContentBrowser ||
				// Add folders
				filteredResourcesContentBrowser[i]->getType() == FE_NULL)
			{
				finalFilteredList.push_back(filteredResourcesContentBrowser[i]);
			}
		}

		filteredResourcesContentBrowser = finalFilteredList;
	}
}