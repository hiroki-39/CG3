#define NOMINMAX
#include "GamePlayScene.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"
#include "KHEngine/Graphics/Billboard/Billboard.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include <algorithm>
#include <random>
#include <memory>
#include <cmath>
#include <numbers>
#include "KHEngine/Scene/LevelLoader.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Resource/ResourceLocator.h"
#include "externals/imgui/imgui.h"
#include <filesystem>
#include "KHEngine/Math/CollisionMath.h"
#include "KHEngine/Scene/SceneManager.h"
#include <chrono>

static void CreateObjectFromNode(const LevelObjectData& node, const Object3d* parentObj, std::vector<std::unique_ptr<Object3d>>& instances, std::vector<std::unique_ptr<Rail>>& outRails, Object3dCommon* common, uint32_t skyboxTexIndex, std::list<std::unique_ptr<Enemy>>& enemies, std::list<std::unique_ptr<Obstacle>>& obstacles, std::list<std::unique_ptr<EnhanceRing>>& enhanceRings, std::vector<Enemy*> parentEnemies = {})
{
	const Object3d* currentObj = parentObj;

	if (node.type == "CURVE")
	{


		if (!parentEnemies.empty())
		{
			for (Enemy* e : parentEnemies)
			{
				auto rail = std::make_unique<Rail>();
				rail->Initialize(node.curvePoints);
				e->SetMovePath(std::move(rail));
			}
		}
		else
		{
			auto rail = std::make_unique<Rail>();
			rail->Initialize(node.curvePoints);
			outRails.push_back(std::move(rail));
		}
	}

	std::vector<Enemy*> currentEnemies = parentEnemies;

	bool isObstacle = (node.fileName.find("Obstacle") != std::string::npos) || (node.fileName.find("Invisible") != std::string::npos) || (node.fileName.find("ColliderOnly") != std::string::npos);
	bool isRing = (node.fileName.find("Ring") != std::string::npos) || (node.name.find("Ring") != std::string::npos) || (node.name.find("強化リング") != std::string::npos) || (node.fileName.find("Heal") != std::string::npos) || (node.name.find("Heal") != std::string::npos) || (node.name.find("回復") != std::string::npos);
	bool isEnemy = (node.fileName.find("Fighter") != std::string::npos || node.fileName.find("Asteroid") != std::string::npos || node.fileName.find("Enemy") != std::string::npos);

	if (isRing)
	{
		auto ring = std::make_unique<EnhanceRing>();
        
        RingType type = RingType::POWER_UP;
        if (node.fileName.find("Heal") != std::string::npos || node.name.find("Heal") != std::string::npos || node.name.find("回復") != std::string::npos) {
            type = RingType::HEAL;
        }

		ring->Initialize(common, node.translation, node.scale, node.rotation, node.fileName, type, skyboxTexIndex, node.collider);
		enhanceRings.push_back(std::move(ring));
	}
	else if (isObstacle)
	{

		auto obstacle = std::make_unique<Obstacle>();

		Vector3 rotRad;
		rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
		rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
		rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
		obstacle->Initialize(common, node.translation, node.scale, rotRad, node.fileName, skyboxTexIndex, node.collider, node.isDestructible);
		// オブジェクトのスポーン進行度を設定
		obstacle->SetSpawnProgress(node.spawnProgress);
		if (!node.texturePath.empty())
		{
			obstacle->SetTexturePath(node.texturePath);
		}
		obstacles.push_back(std::move(obstacle));
	}
	else if (isEnemy)
	{
		currentEnemies.clear();
		int count = std::max<int>(1, node.spawnCount);
		for (int i = 0; i < count; ++i)
		{
			auto enemy = std::make_unique<Enemy>();


			Vector3 offset = { 0, 0, 0 };
			if (node.formationType == "LINE")
			{
				offset.z = i * node.formationSpacing;
			}
			else if (node.formationType == "V_SHAPE")
			{
				if (i > 0)
				{
					float side = (i % 2 == 1) ? 1.0f : -1.0f;
					int row = (i + 1) / 2;
					offset.x = side * row * node.formationSpacing;
					offset.z = row * node.formationSpacing;
				}
			}
			else if (node.formationType == "HORIZONTAL")
			{
				if (i > 0)
				{
					float side = (i % 2 == 1) ? 1.0f : -1.0f;
					int row = (i + 1) / 2;
					offset.x = side * row * node.formationSpacing;
				}
			}


			LevelObjectData spawnNode = node;
			spawnNode.translation.x += offset.x;
			spawnNode.translation.y += offset.y;
			spawnNode.translation.z += offset.z;

			enemy->Initialize(common, spawnNode, skyboxTexIndex);
			enemy->SetSpawnProgress(node.spawnProgress);
			enemy->SetSpawnDelay(i * node.spawnInterval);

			if (!node.texturePath.empty())
			{
				enemy->SetTexturePath(node.texturePath);
			}


			currentEnemies.push_back(enemy.get());

			enemies.push_back(std::move(enemy));
		}
	}
	else if (node.type == "MESH")
	{
		auto obj = std::make_unique<Object3d>();
		obj->Initialize(common);

		std::string modelName = node.fileName;
		if (modelName.empty())
		{
			// Blenderの複製連番サフィックス（.001, .002等）がある場合はベース名を抽出
			std::string baseName = node.name;
			size_t dotPos = baseName.rfind('.');
			if (dotPos != std::string::npos && dotPos + 1 < baseName.size())
			{
				bool isNumber = true;
				for (size_t i = dotPos + 1; i < baseName.size(); ++i)
				{
					if (!std::isdigit(static_cast<unsigned char>(baseName[i])))
					{
						isNumber = false;
						break;
					}
				}
				if (isNumber)
				{
					baseName = baseName.substr(0, dotPos);
				}
			}
			std::string baseObjName = baseName;
			if (baseObjName.find(".obj") == std::string::npos) baseObjName += ".obj";

			// ベース名のモデル（例: AITerrain_Background_Mountains.obj, pillar.obj等）が存在すればそれを優先して共通化
			if (!ResourceLocator::Resolve(baseObjName, ResourceLocator::AssetType::Model3D).empty())
			{
				modelName = baseObjName;
			}
			else
			{
				std::string directName = node.name;
				if (directName.find(".obj") == std::string::npos) directName += ".obj";
				modelName = directName;
			}
		}
		else if (modelName.find(".obj") == std::string::npos)
		{
			modelName += ".obj";
		}

		ModelManager::GetInstance()->LoadModel(modelName);
		if (ModelManager::GetInstance()->FindModel(modelName) != nullptr)
		{
			obj->SetModel(modelName);
		}

		obj->SetTranslate(node.translation);


		Vector3 rotRad;
		rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
		rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
		rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
		obj->SetRotation(rotRad);

		obj->SetScale(node.scale);
		obj->SetEnvironmentTextureIndex(skyboxTexIndex);

		if (parentObj)
		{
			obj->SetParent(parentObj);
		}

		currentObj = obj.get();
		instances.push_back(std::move(obj));
	}


	for (const auto& child : node.children)
	{
		CreateObjectFromNode(child, currentObj, instances, outRails, common, skyboxTexIndex, enemies, obstacles, enhanceRings, currentEnemies);
	}
}

static void LoadEnemiesOnlyFromNode(const LevelObjectData& node, Object3dCommon* common, uint32_t skyboxTexIndex, std::list<std::unique_ptr<Enemy>>& enemies, std::list<std::unique_ptr<Obstacle>>& obstacles, std::list<std::unique_ptr<EnhanceRing>>& enhanceRings, std::vector<Enemy*> parentEnemies = {})
{
	if (node.type == "CURVE" && !parentEnemies.empty())
	{
		for (Enemy* e : parentEnemies)
		{
			auto rail = std::make_unique<Rail>();
			rail->Initialize(node.curvePoints);
			e->SetMovePath(std::move(rail));
		}
	}

	std::vector<Enemy*> currentEnemies = parentEnemies;

	bool isObstacle = (node.fileName.find("Obstacle") != std::string::npos) || (node.fileName.find("Invisible") != std::string::npos) || (node.fileName.find("ColliderOnly") != std::string::npos);
	bool isRing = (node.fileName.find("Ring") != std::string::npos) || (node.name.find("Ring") != std::string::npos) || (node.name.find("強化リング") != std::string::npos) || (node.fileName.find("Heal") != std::string::npos) || (node.name.find("Heal") != std::string::npos) || (node.name.find("回復") != std::string::npos);
	bool isEnemy = (node.fileName.find("Fighter") != std::string::npos || node.fileName.find("Asteroid") != std::string::npos || node.fileName.find("Enemy") != std::string::npos);

	if (isRing)
	{
		auto ring = std::make_unique<EnhanceRing>();
        
        RingType type = RingType::POWER_UP;
        if (node.fileName.find("Heal") != std::string::npos || node.name.find("Heal") != std::string::npos || node.name.find("回復") != std::string::npos) {
            type = RingType::HEAL;
        }

		ring->Initialize(common, node.translation, node.scale, node.rotation, node.fileName, type, skyboxTexIndex, node.collider);
		enhanceRings.push_back(std::move(ring));
	}
	else if (isObstacle)
	{
		auto obstacle = std::make_unique<Obstacle>();
		Vector3 rotRad;
		rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
		rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
		rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
		obstacle->Initialize(common, node.translation, node.scale, rotRad, node.fileName, skyboxTexIndex, node.collider, node.isDestructible);
		// オブジェクトのスポーン進行度を設定
		obstacle->SetSpawnProgress(node.spawnProgress);
		if (!node.texturePath.empty())
		{
			obstacle->SetTexturePath(node.texturePath);
		}
		obstacles.push_back(std::move(obstacle));
	}
	else if (isEnemy)
	{
		currentEnemies.clear();
		int count = std::max<int>(1, node.spawnCount);
		for (int i = 0; i < count; ++i)
		{
			auto enemy = std::make_unique<Enemy>();


			Vector3 offset = { 0, 0, 0 };
			if (node.formationType == "LINE")
			{
				offset.z = i * node.formationSpacing;
			}
			else if (node.formationType == "V_SHAPE")
			{
				if (i > 0)
				{
					float side = (i % 2 == 1) ? 1.0f : -1.0f;
					int row = (i + 1) / 2;
					offset.x = side * row * node.formationSpacing;
					offset.z = row * node.formationSpacing;
				}
			}
			else if (node.formationType == "HORIZONTAL")
			{
				if (i > 0)
				{
					float side = (i % 2 == 1) ? 1.0f : -1.0f;
					int row = (i + 1) / 2;
					offset.x = side * row * node.formationSpacing;
				}
			}


			LevelObjectData spawnNode = node;
			spawnNode.translation.x += offset.x;
			spawnNode.translation.y += offset.y;
			spawnNode.translation.z += offset.z;

			enemy->Initialize(common, spawnNode, skyboxTexIndex);
			enemy->SetSpawnProgress(node.spawnProgress);
			enemy->SetSpawnDelay(i * node.spawnInterval);

			if (!node.texturePath.empty())
			{
				enemy->SetTexturePath(node.texturePath);
			}
			currentEnemies.push_back(enemy.get());
			enemies.push_back(std::move(enemy));
		}
	}

	for (const auto& child : node.children)
	{
		LoadEnemiesOnlyFromNode(child, common, skyboxTexIndex, enemies, obstacles, enhanceRings, currentEnemies);
	}
}

void GamePlayScene::Initialize()
{
	auto initStartTime = std::chrono::high_resolution_clock::now();

	auto services = EngineServices::GetInstance();
	auto object3dCommon = services->GetObject3dCommon();
	auto dxCommon = services->GetDirectXCommon();
	auto srvManager = services->GetSrvManager();
	auto spriteCommon = services->GetSpriteCommon();


	camera = std::make_unique<Camera>();
	camera->SetTranslate({ 0.0f, 6.0f, -20.0f });
	camera->SetRotation({ 0.0f, 0.0f, 0.0f });


	debugCamera_ = std::make_unique<Camera>();
	debugCamera_->SetTranslate({ 0.0f, 6.0f, -20.0f });
	debugCamera_->SetRotation({ 0.0f, 0.0f, 0.0f });

	activeCamera_ = camera.get();

	if (object3dCommon)
	{
		object3dCommon->SetDefaultCamera(activeCamera_);
	}


	ParticleManager::GetInstance()->RegisterQuad("quad", "circle2.png");
	ParticleManager::GetInstance()->RegisterRing("ring", "gradationLine.png", 32, 0.5f, 1.0f);
	ParticleManager::GetInstance()->RegisterCylinder("Cylinder", "resources/sprites/gradationLine.png");

	uint32_t instancingSrvIndex = UINT32_MAX;

	auto texManager = TextureManager::GetInstance();
	dxCommon->BeginTextureUploadBatch();


	auto t0 = std::chrono::high_resolution_clock::now();
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon, "resources/skybox.dds");
	auto t1 = std::chrono::high_resolution_clock::now();
	float skyboxMs = std::chrono::duration<float, std::milli>(t1 - t0).count();


	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("player.obj");


	texManager->LoadTexture("uvChecker.png");
	texManager->LoadTexture("monsterBall.png");
	texManager->LoadTexture("checkerBoard.png");
	texManager->LoadTexture("circle.png");
	texManager->LoadTexture("circle2.png");
	texManager->LoadTexture("gradationLine.png");
	texManager->LoadTexture("sprites/white.png");
	texManager->LoadTexture("sprites/prticle_kira.png");
	texManager->LoadTexture("sprites/hart.png");


	uint32_t uvCheckerTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("uvChecker.png");
	uint32_t monsterBallTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("monsterBall.png");
	uint32_t checkerBoardTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("checkerBoard.png");
	uint32_t skyboxTexIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/skybox.dds");




	uint32_t whiteTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("sprites/white.png");
	if (whiteTex == 0)
	{

		whiteTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("white.png");
	}
	whiteTexIndex_ = whiteTex;


	hpBarBgSprite_ = std::make_unique<Sprite>();
	if (hpBarBgSprite_)
	{
		hpBarBgSprite_->Initialize(spriteCommon, whiteTexIndex_);
		hpBarBgSprite_->SetAnchorPoint(Vector2(0.0f, 0.0f));
		hpBarBgSprite_->SetPosition(Vector2(20.0f, 720.0f - 32.0f - 20.0f));
		hpBarBgSprite_->SetSize(Vector2(400.0f, 32.0f));
		hpBarBgSprite_->SetColor(Vector4(0.8f, 0.8f, 0.8f, 0.8f));
		hpBarBgSprite_->Update();
	}


	hpBarSprite_ = std::make_unique<Sprite>();
	if (hpBarSprite_)
	{
		hpBarSprite_->Initialize(spriteCommon, whiteTexIndex_);
		hpBarSprite_->SetAnchorPoint(Vector2(0.0f, 0.0f));
		hpBarSprite_->SetPosition(Vector2(20.0f, 720.0f - 32.0f - 20.0f));
		hpBarSprite_->SetSize(Vector2(400.0f, 32.0f));
		hpBarSprite_->SetColor(Vector4(0.0f, 1.0f, 0.0f, 1.0f));
		hpBarSprite_->Update();
	}

	auto tLevel0 = std::chrono::high_resolution_clock::now();
	ReloadLevel();
	auto tLevel1 = std::chrono::high_resolution_clock::now();
	float levelMs = std::chrono::duration<float, std::milli>(tLevel1 - tLevel0).count();

	// カメラ
	cameraObject_ = std::make_unique<Object3d>();
	cameraObject_->Initialize(object3dCommon);

	player_ = std::make_unique<Player>();
	player_->Initialize(object3dCommon, skybox_->GetCubemapSrvIndex());
	player_->LoadSettings("resources/json/player/player_settings.json");


	player_->GetObject3d()->SetParent(cameraObject_.get());
	if (player_->GetColliderObject())
	{
		player_->GetColliderObject()->SetParent(cameraObject_.get());
	}
	player_->GetReticle()->SetParent(cameraObject_.get());
	if (player_->GetFrontReticle())
	{
		player_->GetFrontReticle()->SetParent(cameraObject_.get());
	}


	railCameraController_ = std::make_unique<RailCameraController>();
	std::vector<Rail*> railsRaw;
	for (auto& r : mainRails_)
	{
		railsRaw.push_back(r.get());
	}
	railCameraController_->Initialize(railsRaw, activeCamera_, cameraObject_.get());

	thrusterEffect_.Initialize(dxCommon, srvManager);
	thrusterEffect_.LoadFromJson("thruster.json");

	explosionEffect_.Initialize(dxCommon, srvManager);
	explosionEffect_.LoadFromJson("explosion.json");

	hitEffect_.Initialize(dxCommon, srvManager);
	hitEffect_.LoadFromJson("hit.json");


	dodgeEffect_.Initialize(dxCommon, srvManager);
	dodgeEffect_.LoadFromJson("dodge.json");

	trailEffect_.Initialize(dxCommon, srvManager);
	trailEffect_.LoadFromJson("trail.json");

	missileSmokeEffect_.Initialize(dxCommon, srvManager);
	missileSmokeEffect_.LoadFromJson("missile_smoke.json");

	ringEffect_.Initialize(dxCommon, srvManager);
	ringEffect_.LoadFromJson("ring.json");

	healRingEffect_.Initialize(dxCommon, srvManager);
	healRingEffect_.LoadFromJson("heal_ring.json");

	windEffect_.Initialize(dxCommon, srvManager);
	windEffect_.LoadFromJson("wind.json");

	auto tUp0 = std::chrono::high_resolution_clock::now();
	texManager->ExecuteUploadCommands();
	texManager->ClearIntermediateResources();
	auto tUp1 = std::chrono::high_resolution_clock::now();
	float uploadMs = std::chrono::duration<float, std::milli>(tUp1 - tUp0).count();

	auto initEndTime = std::chrono::high_resolution_clock::now();
	lastLoadTimeMs_ = std::chrono::duration<float, std::milli>(initEndTime - initStartTime).count();

	char logBuf[512];
	snprintf(logBuf, sizeof(logBuf),
		"\n========================================\n"
		"[Load Profiler Breakdown]\n"
		"  - Skybox Load:         %8.2f ms\n"
		"  - ReloadLevel (Map):   %8.2f ms\n"
		"  - GPU Texture Upload:  %8.2f ms\n"
		"  - TOTAL Initialize:    %8.2f ms (%.3f s)\n"
		"========================================\n\n",
		skyboxMs, levelMs, uploadMs, lastLoadTimeMs_, lastLoadTimeMs_ / 1000.0f);
	OutputDebugStringA(logBuf);
}

void GamePlayScene::ReloadLevel()
{
	auto services = EngineServices::GetInstance();
	auto object3dCommon = services->GetObject3dCommon();


	while (modelInstances.size() > 1)
	{
		modelInstances.pop_back();
	}

	enemies_.clear();
	obstacles_.clear();
	bullets_.clear();
	missiles_.clear();
	railVisualizers_.clear();
	enemyRailVisualizers_.clear();
	mainRails_.clear();
	if (railCameraController_)
	{
		railCameraController_->Reset();
	}


	auto levelData = LevelLoader::Load("resources/json/maps/template/template.json");
	if (levelData)
	{
		for (const auto& objData : levelData->objects)
		{
			CreateObjectFromNode(objData, nullptr, modelInstances, mainRails_, object3dCommon, skybox_->GetCubemapSrvIndex(), enemies_, obstacles_, enhanceRings_);
		}
		OutputDebugStringA("LevelLoader: Successfully reloaded objects.\n");


		if (railCameraController_)
		{
			std::vector<Rail*> railsRaw;
			for (auto& r : mainRails_)
			{
				railsRaw.push_back(r.get());
			}
			railCameraController_->Initialize(railsRaw, activeCamera_, cameraObject_.get());
		}
	}
	else
	{
		OutputDebugStringA("LevelLoader: Failed to reload level.\n");
	}


	railVisualizers_.clear();
	railModels_.clear();
	if (!mainRails_.empty())
	{
		std::vector<Vector4> colors = {
			{1.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 0.5f, 1.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f},
			{0.0f, 1.0f, 1.0f, 1.0f},
			{1.0f, 0.0f, 1.0f, 1.0f},
			{1.0f, 0.5f, 0.0f, 1.0f}
		};

		std::string resolved = ResourceLocator::Resolve("rail.obj", ResourceLocator::AssetType::Model3D);
		std::string directory = "resources/models";
		std::string filename = "rail.obj";
		if (!resolved.empty())
		{
			std::filesystem::path rp(reinterpret_cast<const char8_t*>(resolved.c_str()));
			directory = rp.parent_path().string();
			filename = rp.filename().string();
		}

		int sampleCount = 200;
		int colorIndex = 0;
		for (auto& rail : mainRails_)
		{
			if (!rail || !rail->IsValid()) continue;

			auto railModel = std::make_unique<Model>();
			railModel->Initialize(ModelManager::GetInstance()->GetModelCommon(), directory, filename);
			railModel->SetColor(colors[colorIndex % colors.size()]);

			for (int i = 0; i < sampleCount; ++i)
			{
				float t1 = static_cast<float>(i) / sampleCount;
				float t2 = static_cast<float>(i + 1) / sampleCount;

				Vector3 p1 = rail->GetPosition(t1);
				Vector3 p2 = rail->GetPosition(t2);

				Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
				float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
				if (length < 0.0001f) continue;

				dir.x /= length; dir.y /= length; dir.z /= length;

				Vector3 center = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f };

				auto obj = std::make_unique<Object3d>();
				obj->Initialize(object3dCommon);
				obj->SetModel(railModel.get());
				obj->SetTranslate(center);
				float yaw = std::atan2(dir.x, dir.z);
				float pitch = std::asin(-dir.y);
				obj->SetRotation(Vector3(pitch, yaw, 0.0f));

				obj->SetScale(Vector3(0.02f, 0.02f, length));

				obj->SetEnvironmentCoefficient(0.0f);
				obj->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());

				railVisualizers_.push_back(std::move(obj));
			}

			railModels_.push_back(std::move(railModel));
			colorIndex++;
		}
	}


	enemyRailModel_ = std::make_unique<Model>();
	std::string resolved = ResourceLocator::Resolve("rail.obj", ResourceLocator::AssetType::Model3D);
	if (!resolved.empty())
	{
		std::filesystem::path rp(reinterpret_cast<const char8_t*>(resolved.c_str()));
		enemyRailModel_->Initialize(ModelManager::GetInstance()->GetModelCommon(), rp.parent_path().string(), rp.filename().string());
	}
	else
	{
		enemyRailModel_->Initialize(ModelManager::GetInstance()->GetModelCommon(), "resources/models", "rail.obj");
	}
	enemyRailModel_->SetColor({ 0.5f, 1.0f, 0.0f, 1.0f });

	int sampleCount = 100;
	for (auto& enemy : enemies_)
	{
		const Rail* rail = enemy->GetMovePath();
		if (!rail || !rail->IsValid()) continue;
		for (int i = 0; i < sampleCount; ++i)
		{
			float t1 = static_cast<float>(i) / sampleCount;
			float t2 = static_cast<float>(i + 1) / sampleCount;

			Vector3 p1 = rail->GetPosition(t1);
			Vector3 p2 = rail->GetPosition(t2);

			Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
			float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
			if (length < 0.0001f) continue;

			dir.x /= length; dir.y /= length; dir.z /= length;

			Vector3 center = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f };

			auto obj = std::make_unique<Object3d>();
			obj->Initialize(object3dCommon);
			obj->SetModel(enemyRailModel_.get());
			obj->SetTranslate(center);
			float yaw = std::atan2(dir.x, dir.z);
			float pitch = std::asin(-dir.y);
			obj->SetRotation(Vector3(pitch, yaw, 0.0f));

			obj->SetScale(Vector3(0.015f, 0.015f, length));

			obj->SetEnvironmentCoefficient(0.0f);
			obj->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());

			enemyRailVisualizers_.push_back(std::move(obj));
		}
	}
}

void GamePlayScene::ReloadEnemiesOnly()
{
	auto services = EngineServices::GetInstance();
	auto object3dCommon = services->GetObject3dCommon();

	enemies_.clear();
	obstacles_.clear();
	enemyRailVisualizers_.clear();

	auto levelData = LevelLoader::Load("resources/json/maps/template/template.json");
	if (levelData)
	{
		for (const auto& objData : levelData->objects)
		{
			LoadEnemiesOnlyFromNode(objData, object3dCommon, skybox_->GetCubemapSrvIndex(), enemies_, obstacles_, enhanceRings_);
		}
		OutputDebugStringA("LevelLoader: Successfully respawned enemies.\n");


		if (enemyRailModel_)
		{
			int sampleCount = 100;
			for (auto& enemy : enemies_)
			{
				const Rail* rail = enemy->GetMovePath();
				if (!rail || !rail->IsValid()) continue;
				for (int i = 0; i < sampleCount; ++i)
				{
					float t1 = static_cast<float>(i) / sampleCount;
					float t2 = static_cast<float>(i + 1) / sampleCount;
					Vector3 p1 = rail->GetPosition(t1);
					Vector3 p2 = rail->GetPosition(t2);
					Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
					float length = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
					if (length < 0.0001f) continue;
					dir.x /= length; dir.y /= length; dir.z /= length;
					Vector3 center = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f };
					auto obj = std::make_unique<Object3d>();
					obj->Initialize(object3dCommon);
					obj->SetModel(enemyRailModel_.get());
					obj->SetTranslate(center);
					float yaw = std::atan2(dir.x, dir.z);
					float pitch = std::asin(-dir.y);
					obj->SetRotation(Vector3(pitch, yaw, 0.0f));
					obj->SetScale(Vector3(0.015f, 0.015f, length));
					obj->SetEnvironmentCoefficient(0.0f);
					obj->SetEnvironmentTextureIndex(skybox_->GetCubemapSrvIndex());
					enemyRailVisualizers_.push_back(std::move(obj));
				}
			}
		}
	}
	else
	{
		OutputDebugStringA("LevelLoader: Failed to respawn enemies.\n");
	}
}


void GamePlayScene::Finalize()
{

	sprites.clear();
	modelInstances.clear();

	if (auto pp = EngineServices::GetInstance()->GetPostProcess())
	{
		pp->SetEffectActive("Grayscale", false);
		pp->SetEffectActive("Vignetting", false);
	}

	skybox_.reset();
	camera.reset();
	debugCamera_.reset();
	activeCamera_ = nullptr;
}

namespace
{
	float LerpAngle(float a, float b, float t)
	{
		float diff = b - a;
		while (diff < -3.14159265f) diff += 6.2831853f;
		while (diff > 3.14159265f) diff -= 6.2831853f;
		return a + diff * t;
	}
}

void GamePlayScene::Update()
{
	auto services = EngineServices::GetInstance();
	auto input = services->GetInput();
	float dt = services->GetDeltaTime();


	if (input && input->TriggerKey(DIK_0))
	{
		services->SetEditorMode(true);
	}


	if (input && input->TriggerKey(DIK_F5))
	{
		ReloadLevel();
	}


	if (isPlaying_)
	{
		activeCamera_ = camera.get();
	}
	else
	{
		activeCamera_ = debugCamera_.get();
	}
	if (auto objCommon = services->GetObject3dCommon())
	{
		objCommon->SetDefaultCamera(activeCamera_);
	}

	float unscaledGameSpeed = baseGameSpeed_;





	if (input && activeCamera_)
	{

		const float kRotateSpeed = 0.005f;
		const float kMoveSpeed = 8.0f;
		const float kZoomSpeed = 0.0015f;

		LONG dx = input->GetMouseMoveX();
		LONG dy = input->GetMouseMoveY();
		LONG wheel = input->GetMouseWheel();


		if (!isPlaying_)
		{
			if (input->PushMouseButton(1))
			{
				Vector3 rot = activeCamera_->GetRotation();

				rot.y += static_cast<float>(dx) * kRotateSpeed;
				rot.x += static_cast<float>(dy) * kRotateSpeed;


				const float kMaxPitch = 1.5f;
				const float kMinPitch = -1.5f;
				rot.x = std::clamp(rot.x, kMinPitch, kMaxPitch);

				activeCamera_->SetRotation(rot);
			}


			float currentMoveSpeed = kMoveSpeed;
			if (input->PushKey(DIK_LSHIFT) || input->PushKey(DIK_RSHIFT))
			{
				currentMoveSpeed *= 25.0f;
			}
			float moveStep = currentMoveSpeed * dt;
			Vector3 pos = activeCamera_->GetTranslate();
			Vector3 rot = activeCamera_->GetRotation();
			float yaw = rot.y;


			Vector3 forward = { std::sinf(yaw), 0.0f, std::cosf(yaw) };
			Vector3 right = { std::cosf(yaw), 0.0f, -std::sinf(yaw) };

			auto normalize = [](Vector3 v)
				{
					float len = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
					if (len > 1e-6f) { v.x /= len; v.y /= len; v.z /= len; }
					return v;
				};

			forward = normalize(forward);
			right = normalize(right);

			if (input->PushKey(DIK_W))
			{
				pos.x += forward.x * moveStep;
				pos.z += forward.z * moveStep;
			}
			if (input->PushKey(DIK_S))
			{
				pos.x -= forward.x * moveStep;
				pos.z -= forward.z * moveStep;
			}
			if (input->PushKey(DIK_D))
			{
				pos.x += right.x * moveStep;
				pos.z += right.z * moveStep;
			}
			if (input->PushKey(DIK_A))
			{
				pos.x -= right.x * moveStep;
				pos.z -= right.z * moveStep;
			}
			if (input->PushKey(DIK_E))
			{
				pos.y += moveStep;
			}
			if (input->PushKey(DIK_Q))
			{
				pos.y -= moveStep;
			}

			activeCamera_->SetTranslate(pos);



		}


		if (isPlaying_)
		{

			auto pp = EngineServices::GetInstance()->GetPostProcess();
			float unscaledGameSpeed = baseGameSpeed_;
			if (player_ && player_->IsBoosting())
			{
				unscaledGameSpeed = baseGameSpeed_ * 1.5f;
				thrusterEffect_.SetBaseColor({ 1.0f, 0.2f, 0.0f, 1.0f });
				if (pp)
				{
					pp->SetEffectActive("RadialBlur", true);
					pp->GetData()->radialBlurIntensity = 0.08f;
				}
				if (railCameraController_)
				{
					railCameraController_->SetSpeedMultiplier(1.5f);
				}
			}
			else
			{
				unscaledGameSpeed = baseGameSpeed_;
				thrusterEffect_.SetBaseColor({ 1.0f, 1.0f, 1.0f, 1.0f });
				if (pp)
				{
					pp->SetEffectActive("RadialBlur", false);
				}
				if (railCameraController_)
				{
					railCameraController_->SetSpeedMultiplier(1.0f);
				}
			}

			// ブースト時の動的FOV拡大演出（通常66度 → ブースト時76度へ滑らかに補間）
			if (activeCamera_)
			{
				float targetFov = (player_ && player_->IsBoosting()) ? 1.32f : 1.15f;
				float currentFov = activeCamera_->GetFovY();
				currentFov += (targetFov - currentFov) * 0.15f;
				activeCamera_->SetFovY(currentFov);
			}


			if (isJustDodgeActive_)
			{
				justDodgeTimer_ -= 1.0f;


				if (pp)
				{
					float t = justDodgeTimer_ / justDodgeMaxTime_;

					pp->GetData()->vignetteIntensity = std::sinf(t * 3.141592f) * 0.6f;
				}

				if (justDodgeTimer_ <= 0.0f)
				{
					isJustDodgeActive_ = false;


					if (pp)
					{
						pp->SetEffectActive("Grayscale", false);
						pp->SetEffectActive("Vignetting", false);
					}
				}
			}

			gameSpeed_ = unscaledGameSpeed;
			if (isJustDodgeActive_)
			{
				gameSpeed_ *= justDodgeSlowSpeed_;
			}

			if (railCameraController_)
			{
				railCameraController_->Update(gameSpeed_, player_->GetTranslate());
			}
		}
	}


	if (player_)
	{
		if (isPlaying_)
		{

			bool wasBanking = player_->IsBanking();
			Vector3 prevLeftWing = player_->GetLeftWingPosition();
			Vector3 prevRightWing = player_->GetRightWingPosition();


			Enemy* nearestEnemy = nullptr;
			float minDistanceSq = 10000.0f;
			float assistRadius = 10.0f;
			Vector3 reticlePos = player_->GetReticleWorldPosition();


			const Matrix4x4& wMat = player_->GetObject3d()->GetmatWorld();
			Vector3 playerPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };

			for (auto& enemy : enemies_)
			{
				if (enemy->IsDead()) continue;
				Vector3 enemyPos = enemy->GetColliderCenter();

				float dz = enemyPos.z - playerPos.z;

				if (dz > 0.0f && dz < 300.0f)
				{

					float dx = enemyPos.x - reticlePos.x;
					float dy = enemyPos.y - reticlePos.y;
					float distSqXY = dx * dx + dy * dy;

					if (distSqXY < (assistRadius * assistRadius) && distSqXY < minDistanceSq)
					{
						minDistanceSq = distSqXY;
						nearestEnemy = enemy.get();
					}
				}
			}
			player_->SetAssistTarget(nearestEnemy);

			// レール進行度に応じた動的移動制限の適用
			if (!mainRails_.empty() && mainRails_[0]->IsValid() && railCameraController_)
			{
				float p = railCameraController_->GetProgress();
				float limitX, limitYMin, limitYMax;
				mainRails_[0]->GetMoveLimits(p, limitX, limitYMin, limitYMax);
				player_->SetTargetMoveLimits(limitX, limitYMin, limitYMax);
			}

			player_->Update(bullets_, missiles_, enemies_, cameraObject_.get(), unscaledGameSpeed);


			if (player_->ConsumeDodgeTrigger())
			{
				const Matrix4x4& wMat = player_->GetObject3d()->GetmatWorld();

				Vector3 pPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };


				Vector3 forward = { wMat.m[2][0], wMat.m[2][1], wMat.m[2][2] };
				float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
				if (len > 0.0001f)
				{
					forward.x /= len; forward.y /= len; forward.z /= len;
				}


				Vector3 effectPos = { pPos.x - forward.x * 2.0f, pPos.y - forward.y * 2.0f, pPos.z - forward.z * 2.0f };

				dodgeEffect_.SetPosition(effectPos);
				dodgeEffect_.Play();
			}


			if (player_->IsBanking())
			{
				Vector3 leftWing = player_->GetLeftWingPosition();
				Vector3 rightWing = player_->GetRightWingPosition();

				if (wasBanking)
				{

					int emitCount = 8;
					for (int i = 1; i <= emitCount; ++i)
					{
						float t = (float)i / emitCount;
						Vector3 lPos = {
							prevLeftWing.x + (leftWing.x - prevLeftWing.x) * t,
							prevLeftWing.y + (leftWing.y - prevLeftWing.y) * t,
							prevLeftWing.z + (leftWing.z - prevLeftWing.z) * t
						};
						Vector3 rPos = {
							prevRightWing.x + (rightWing.x - prevRightWing.x) * t,
							prevRightWing.y + (rightWing.y - prevRightWing.y) * t,
							prevRightWing.z + (rightWing.z - prevRightWing.z) * t
						};

						trailEffect_.SetPosition(lPos);
						trailEffect_.Play();

						trailEffect_.SetPosition(rPos);
						trailEffect_.Play();
					}
				}
				else
				{
					trailEffect_.SetPosition(leftWing);
					trailEffect_.Play();

					trailEffect_.SetPosition(rightWing);
					trailEffect_.Play();
				}
			}
		}
		else
		{

			player_->Update3DObjectOnly();
		}
	}


	if (isPlaying_)
	{
		for (auto it = bullets_.begin(); it != bullets_.end(); )
		{
			(*it)->Update(gameSpeed_);
			if ((*it)->IsDead())
			{
				it = bullets_.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = missiles_.begin(); it != missiles_.end(); )
		{
			(*it)->Update(gameSpeed_);

			if ((*it)->GetCurrentPhase() == PlayerMissile::Phase::FLIGHT)
			{
				thrusterEffect_.SetPosition((*it)->GetPosition());
				thrusterEffect_.Play();

				missileSmokeEffect_.SetPosition((*it)->GetPosition());
				missileSmokeEffect_.Play();
			}

			if ((*it)->IsDead())
			{
				it = missiles_.erase(it);
			}
			else
			{
				++it;
			}
		}

		Vector3 cameraPos = activeCamera_->GetTranslate();
		Vector3 rot = activeCamera_->GetRotation();
		Vector3 cameraForward = { std::sinf(rot.y), 0.0f, std::cosf(rot.y) };


		for (auto it = enemies_.begin(); it != enemies_.end();)
		{
			(*it)->Update(cameraPos, cameraForward, player_.get(), enemyBullets_, gameSpeed_);


			for (auto& bullet : bullets_)
			{
				if (bullet->IsDead()) continue;

				Sphere bulletSphere = { bullet->GetPosition(), 1.0f };
				bool isHit = false;

				isHit = (*it)->CheckCollision(bulletSphere);


				if (!isHit)
				{
					Vector3 prev = bullet->GetPreviousPosition();
					Vector3 curr = bullet->GetPosition();
					Vector3 diff = { curr.x - prev.x, curr.y - prev.y, curr.z - prev.z };
					float moveLen = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					if (moveLen > 0.0001f)
					{
						Ray moveRay = { prev, { diff.x / moveLen, diff.y / moveLen, diff.z / moveLen } };
						float hitDist = 0.0f;
						if ((*it)->CheckRaycast(moveRay, &hitDist))
						{
							if (hitDist <= moveLen + 1.0f)
							{
								isHit = true;
							}
						}
					}
				}

				if (isHit)
				{
					bullet->OnCollision();
					(*it)->OnCollision();


					hitEffect_.SetPosition(bulletSphere.center);
					hitEffect_.Play();
				}
			}


			for (auto& missile : missiles_)
			{
				if (missile->IsDead()) continue;

				Sphere missileSphere = { missile->GetPosition(), 1.0f };
				bool isHit = false;

				isHit = (*it)->CheckCollision(missileSphere);


				if (!isHit)
				{
					Vector3 prev = missile->GetPreviousPosition();
					Vector3 curr = missile->GetPosition();
					Vector3 diff = { curr.x - prev.x, curr.y - prev.y, curr.z - prev.z };
					float moveLen = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					if (moveLen > 0.0001f)
					{
						Ray moveRay = { prev, { diff.x / moveLen, diff.y / moveLen, diff.z / moveLen } };
						float hitDist = 0.0f;
						if ((*it)->CheckRaycast(moveRay, &hitDist))
						{
							if (hitDist <= moveLen + 1.0f)
							{
								isHit = true;
							}
						}
					}
				}

				if (isHit)
				{
					missile->OnCollision();
					// 取得されたリングを消滅させる
					(*it)->Kill();


					hitEffect_.SetPosition(missileSphere.center);
					hitEffect_.Play();

					explosionEffect_.SetPosition(missileSphere.center);
					explosionEffect_.Play();
				}
			}

			if ((*it)->IsDead())
			{

				explosionEffect_.SetPosition((*it)->GetPosition());
				explosionEffect_.Play();
				it = enemies_.erase(it);
			}
			else
			{
				++it;
			}
		}


		for (auto it = enemyBullets_.begin(); it != enemyBullets_.end();)
		{
			(*it)->Update(gameSpeed_);

			if (!(*it)->IsDead() && player_ && !player_->IsDead())
			{
				Sphere bulletSphere = { (*it)->GetPosition(), 1.0f };


				OBB playerOBB = player_->GetWorldOBB();

				if (CollisionMath::IsCollision(bulletSphere, playerOBB))
				{
					if (player_->IsRolling() && player_->GetRollTimer() < 15.0f)
					{

						isJustDodgeActive_ = true;
						justDodgeTimer_ = justDodgeMaxTime_;


						auto pp = EngineServices::GetInstance()->GetPostProcess();
						if (pp)
						{
							pp->SetEffectActive("Grayscale", true);
							pp->SetEffectActive("Vignetting", true);
						}

						dodgeEffect_.SetPosition((*it)->GetPosition());
						dodgeEffect_.Play();

						(*it)->OnCollision();
					}
					else
					{

						(*it)->OnCollision();
						hitEffect_.SetPosition((*it)->GetPosition());
						hitEffect_.Play();

						player_->OnCollision(); cameraShakeTimer_ = 20.0f;
					}
				}
			}

			if ((*it)->IsDead())
			{
				it = enemyBullets_.erase(it);
			}
			else
			{
				++it;
			}
		}


		for (auto it = enhanceRings_.begin(); it != enhanceRings_.end();)
		{
			(*it)->Update();

			if (!(*it)->IsDead() && player_ && !player_->IsDead())
			{
				if ((*it)->CheckCollision(player_.get()))
				{
					if ((*it)->GetType() == RingType::POWER_UP) {
						player_->PowerUp();
					} else if ((*it)->GetType() == RingType::HEAL) {
						player_->Heal(3000);
					}
					
					if ((*it)->GetType() == RingType::POWER_UP) {
						ringEffect_.SetPosition((*it)->GetPosition());
						ringEffect_.Play();
					} else {
						healRingEffect_.SetPosition((*it)->GetPosition());
						healRingEffect_.Play();
					}

					(*it)->StartShrink(player_.get());
				}
			}

			if ((*it)->IsDead())
			{
				it = enhanceRings_.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = obstacles_.begin(); it != obstacles_.end();)
		{
			(*it)->Update();


			for (auto& bullet : bullets_)
			{
				if (bullet->IsDead()) continue;

				Sphere bulletSphere = { bullet->GetPosition(), 1.0f };
				bool isHit = false;

				isHit = (*it)->CheckCollision(bulletSphere);


				if (!isHit)
				{
					Vector3 prev = bullet->GetPreviousPosition();
					Vector3 curr = bullet->GetPosition();
					Vector3 diff = { curr.x - prev.x, curr.y - prev.y, curr.z - prev.z };
					float moveLen = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					if (moveLen > 0.0001f)
					{
						Ray moveRay = { prev, { diff.x / moveLen, diff.y / moveLen, diff.z / moveLen } };
						float hitDist = 0.0f;
						if ((*it)->CheckRaycast(moveRay, &hitDist))
						{
							if (hitDist <= moveLen + 1.0f)
							{
								isHit = true;
							}
						}
					}
				}

				if (isHit)
				{
					bullet->OnCollision();
					(*it)->OnCollision();


					hitEffect_.SetPosition(bulletSphere.center);
					hitEffect_.Play();
				}
			}


			for (auto& missile : missiles_)
			{
				if (missile->IsDead()) continue;

				Sphere missileSphere = { missile->GetPosition(), 1.0f };
				bool isHit = false;

				isHit = (*it)->CheckCollision(missileSphere);


				if (!isHit)
				{
					Vector3 prev = missile->GetPreviousPosition();
					Vector3 curr = missile->GetPosition();
					Vector3 diff = { curr.x - prev.x, curr.y - prev.y, curr.z - prev.z };
					float moveLen = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
					if (moveLen > 0.0001f)
					{
						Ray moveRay = { prev, { diff.x / moveLen, diff.y / moveLen, diff.z / moveLen } };
						float hitDist = 0.0f;
						if ((*it)->CheckRaycast(moveRay, &hitDist))
						{
							if (hitDist <= moveLen + 1.0f)
							{
								isHit = true;
							}
						}
					}
				}

				if (isHit)
				{
					missile->OnCollision();
					// 取得されたリングを消滅させる
					(*it)->Kill();


					hitEffect_.SetPosition(missileSphere.center);
					hitEffect_.Play();
				}
			}

			// プレイヤーとのメッシュ／障害物衝突判定（接触・めり込み・はじかれ）
			if (player_ && !player_->IsDead())
			{
				OBB playerOBB = player_->GetWorldOBB();

				CollisionResult colRes;
				if ((*it)->CheckCollisionWithOBB(playerOBB, &colRes))
				{
					bool causedDamage = player_->OnTerrainCollision(colRes.normal, colRes.penetrationDepth, cameraObject_.get());
					hitEffect_.SetPosition(colRes.hitPoint);
					hitEffect_.Play();
					if (causedDamage)
					{
						cameraShakeTimer_ = 20.0f;
					}
				}
			}

			if ((*it)->IsDead())
			{

				explosionEffect_.SetPosition((*it)->GetPosition());
				explosionEffect_.Play();
				it = obstacles_.erase(it);
			}
			else
			{
				++it;
			}
		}

		// --- 地形・背景メッシュ（modelInstances）とのメッシュ衝突判定 ---
		if (player_ && !player_->IsDead())
		{
			OBB playerOBB = player_->GetWorldOBB();

			for (auto& modelObj : modelInstances)
			{
				if (!modelObj) continue;
				CollisionResult colRes;
				if (modelObj->CheckCollisionWithOBB(playerOBB, &colRes))
				{
					bool causedDamage = player_->OnTerrainCollision(colRes.normal, colRes.penetrationDepth, cameraObject_.get());
					hitEffect_.SetPosition(colRes.hitPoint);
					hitEffect_.Play();
					if (causedDamage)
					{
						cameraShakeTimer_ = 20.0f;
					}
				}
			}
		}

		// プレイヤー弾と地形メッシュ（modelInstances）の衝突判定
		for (auto& bullet : bullets_)
		{
			if (bullet->IsDead()) continue;
			Sphere bulletSphere = { bullet->GetPosition(), 1.0f };
			for (auto& modelObj : modelInstances)
			{
				if (!modelObj) continue;
				if (modelObj->CheckCollisionWithSphere(bulletSphere, nullptr))
				{
					bullet->OnCollision();
					hitEffect_.SetPosition(bulletSphere.center);
					hitEffect_.Play();
					break;
				}
			}
		}

		bool isLockOn = false;
		if (player_ && player_->GetObject3d() && player_->GetReticle() && activeCamera_)
		{
			Vector3 cameraPos = activeCamera_->GetTranslate();
			Vector3 reticlePos = player_->GetReticleWorldPosition();


			Vector3 rayDir = {
				reticlePos.x - cameraPos.x,
				reticlePos.y - cameraPos.y,
				reticlePos.z - cameraPos.z
			};
			float length = std::sqrtf(rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z);

			Vector3 lockOnPos = { 0, 0, 0 };
			Enemy* lockOnEnemy = nullptr;

			if (length > 0.0001f)
			{
				rayDir.x /= length;
				rayDir.y /= length;
				rayDir.z /= length;

				Ray ray = { cameraPos, rayDir };

				float bestScore = 1000000.0f;


				for (auto& enemy : enemies_)
				{
					if (enemy->IsDead()) continue;

					float dist = 0.0f;
					bool hit = enemy->CheckRaycast(ray, &dist);

					if (hit)
					{

						Vector3 enemyPos = enemy->GetPosition();
						Vector3 toEnemy = { enemyPos.x - cameraPos.x, enemyPos.y - cameraPos.y, enemyPos.z - cameraPos.z };
						float toEnemyLen = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);
						if (toEnemyLen > 0.0f)
						{
							toEnemy.x /= toEnemyLen;
							toEnemy.y /= toEnemyLen;
							toEnemy.z /= toEnemyLen;
						}


						float dot = ray.direction.x * toEnemy.x + ray.direction.y * toEnemy.y + ray.direction.z * toEnemy.z;
						float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));


						float score = angle * 100.0f + dist * 0.1f;

						if (score < bestScore)
						{
							bestScore = score;
							lockOnPos = {
								ray.origin.x + ray.direction.x * dist,
								ray.origin.y + ray.direction.y * dist,
								ray.origin.z + ray.direction.z * dist
							};
							isLockOn = true;
							lockOnEnemy = enemy.get();
						}
					}
				}
			}


			float minEnemyDist = 1000000.0f;
			for (auto& enemy : enemies_)
			{
				if (enemy->IsDead()) continue;
				Vector3 ePos = enemy->GetPosition();
				float d = std::sqrt((ePos.x - cameraPos.x) * (ePos.x - cameraPos.x) +
					(ePos.y - cameraPos.y) * (ePos.y - cameraPos.y) +
					(ePos.z - cameraPos.z) * (ePos.z - cameraPos.z));
				if (d < minEnemyDist)
				{
					minEnemyDist = d;
				}
			}


			if (isLockOn)
			{
				player_->SetReticleColor({ 1.0f, 0.0f, 0.0f, 1.0f });
				player_->SetLockOn(true, lockOnPos, lockOnEnemy);
			}
			else
			{
				if (minEnemyDist < 100.0f)
				{

					player_->SetReticleColor({ 1.0f, 0.6f, 0.0f, 1.0f });
				}
				else
				{
					player_->SetReticleColor({ 1.0f, 1.0f, 1.0f, 1.0f });
				}
				player_->SetLockOn(false);
			}
		}


		for (auto it = bullets_.begin(); it != bullets_.end(); )
		{
			if ((*it)->IsDead())
			{
				it = bullets_.erase(it);
			}
			else
			{
				++it;
			}
		}


		for (auto it = missiles_.begin(); it != missiles_.end(); )
		{
			if ((*it)->IsDead())
			{
				it = missiles_.erase(it);
			}
			else
			{
				++it;
			}
		}

	}
	else
	{
		for (auto& bullet : bullets_)
		{
			bullet->Update3DObjectOnly();
		}
		for (auto& missile : missiles_)
		{
			missile->Update3DObjectOnly();
		}
		for (auto& enemy : enemies_)
		{
			enemy->Update3DObjectOnly();
		}
	}


	if (input && input->TriggerKey(DIK_ESCAPE))
	{
		auto dxCommon = services->GetDirectXCommon();
		if (dxCommon)
		{
			WinApp* winApp = dxCommon->GetWinApp();
			if (winApp)
			{
				::PostMessage(winApp->GetHwnd(), WM_CLOSE, 0, 0);
			}
		}
	}


	if (activeCamera_)
	{ // カメラシェイク処琁E
		if (cameraShakeTimer_ > 0.0f) { cameraShakeTimer_ -= 1.0f; float p = cameraShakeTimer_ / 20.0f; float shakePower = 2.0f * p; Vector3 pos = activeCamera_->GetTranslate(); pos.x += ((rand() % 100) / 50.0f - 1.0f) * shakePower; pos.y += ((rand() % 100) / 50.0f - 1.0f) * shakePower; activeCamera_->SetTranslate(pos); } activeCamera_->Update();
	}
	for (auto& model : modelInstances) if (model) model->Update();
	if (isDrawRail_)
	{
		for (auto& vis : railVisualizers_) if (vis) vis->Update();
		for (auto& vis : enemyRailVisualizers_) if (vis) vis->Update();
	}


	if (!isPlaying_)
	{
		for (auto& enemy : enemies_) if (enemy) enemy->Update3DObjectOnly();
		for (auto& obstacle : obstacles_) if (obstacle) obstacle->Update3DObjectOnly();
		for (auto& ring : enhanceRings_) if (ring) ring->Update();
	}


	Matrix4x4 cameraMatrix = Matrix4x4::Identity();
	Matrix4x4 viewMatrix = Matrix4x4::Identity();
	Matrix4x4 projectionMatrix = Matrix4x4::Identity();
	Matrix4x4 billboardMatrix = Matrix4x4::Identity();

	if (activeCamera_)
	{
		cameraMatrix = activeCamera_->GetWorldMatrix();
		viewMatrix = activeCamera_->GetViewMatrix();
		projectionMatrix = activeCamera_->GetProjectionMatrix();
		billboardMatrix = Billboard::CreateFromCamera(activeCamera_, true);
	}

	if (skybox_)
	{
		skybox_->SetCamera(activeCamera_);
		skybox_->Update();
	}

	if (isPlaying_)
	{

	}


	if (player_ && player_->GetObject3d())
	{
		const Matrix4x4& wMat = player_->GetObject3d()->GetmatWorld();
		Vector3 worldPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };


		Vector3 backward = { -wMat.m[2][0], -wMat.m[2][1], -wMat.m[2][2] };
		float length = std::sqrt(backward.x * backward.x + backward.y * backward.y + backward.z * backward.z);
		if (length > 0.0f)
		{
			backward.x /= length; backward.y /= length; backward.z /= length;
		}
		float offsetDistance = 1.8f;
		worldPos.x += backward.x * offsetDistance;
		worldPos.y += backward.y * offsetDistance;
		worldPos.z += backward.z * offsetDistance;

		thrusterEffect_.SetPosition(worldPos);
		thrusterEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
		explosionEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
		hitEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
		dodgeEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
		trailEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
		missileSmokeEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
		ringEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
		healRingEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);

		// 気流線（風の筋）エフェクトの発生（自機前方から奥へと通り過ぎる白い空気抵抗パーティクル）
		if (isPlaying_)
		{
			bool isBoost = player_ && player_->IsBoosting();
			int spawnCount = isBoost ? 4 : 2;

			// 機体の飛行進路周辺に分散させて発生（照準を遮らないよう自然な範囲に分散）
			std::uniform_real_distribution<float> distOffsetX(-8.0f, 8.0f);
			std::uniform_real_distribution<float> distOffsetY(-3.0f, 4.5f);
			std::uniform_real_distribution<float> distOffsetZ(20.0f, 45.0f);

			for (int i = 0; i < spawnCount; ++i)
			{
				Vector3 windSpawnPos = {
					worldPos.x + distOffsetX(randomEngine),
					worldPos.y + distOffsetY(randomEngine),
					worldPos.z + distOffsetZ(randomEngine)
				};
				windEffect_.SetPosition(windSpawnPos);
				windEffect_.Play();
			}
		}
		windEffect_.Update(dt, viewMatrix, projectionMatrix, billboardMatrix);
	}

	// --- ゲームループ・UI更新 (HPバー・シーン終了判定) ---
	if (player_)
	{
		if (hpBarSprite_)
		{
			float hpRate = (float)player_->GetHp() / player_->GetMaxHp();
			if (hpRate < 0.0f) hpRate = 0.0f;
			hpBarSprite_->SetSize(Vector2(400.0f * hpRate, 32.0f));

			// HPに応じて色を変更
			if (hpRate > 0.5f)
			{
				hpBarSprite_->SetColor(Vector4(0.0f, 1.0f, 0.0f, 1.0f)); // 緑
			}
			else if (hpRate > 0.2f)
			{
				hpBarSprite_->SetColor(Vector4(1.0f, 1.0f, 0.0f, 1.0f)); // 黄色
			}
			else
			{
				hpBarSprite_->SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤
			}
			hpBarSprite_->Update();
		}

		if (isPlaying_)
		{
			if (!enemies_.empty())
			{
				hasEnemySpawned_ = true;
			}

			if (player_->GetHp() <= 0)
			{
				auto sceneManager = GetSceneManager();
				if (sceneManager)
				{
					sceneManager->ChangeScene("GAMEOVER");
					return;
				}
			}

			if (hasEnemySpawned_ && enemies_.empty())
			{
				auto sceneManager = GetSceneManager();
				if (sceneManager)
				{
					sceneManager->ChangeScene("GAMECLEAR");
					return;
				}
			}
		}
	}

#ifdef USE_IMGUI
	// =========================================================================
	// 統合エディタUI: [左] メニュー  [右] インスペクター  [下] タイムライン
	// =========================================================================
	static int currentNavIndex = 0;
	const char* navItems[] = {
		"ゲーム・進行",
		"プレイヤー",
		"演出・シェーダー",
		"シーン・照明"
	};

	// -------------------------------------------------------------
	// 1. [左ドック用] メニュー (Category Selector & Performance)
	// -------------------------------------------------------------
	if (ImGui::Begin("メニュー"))
	{
		ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "CATEGORY");
		ImGui::Separator();

		for (int i = 0; i < IM_ARRAYSIZE(navItems); ++i)
		{
			bool isSelected = (currentNavIndex == i);
			if (ImGui::Selectable(navItems[i], isSelected, 0, ImVec2(0, 30)))
			{
				currentNavIndex = i;
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("F1: UI表示切替");
		if (ImGui::Button("全画面プレイ (F1)", ImVec2(-1, 28)))
		{
			EngineServices::GetInstance()->SetEditorMode(false);
		}

		// 最下部にパフォーマンス情報を常時表示
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.5f, 1.0f), "PERFORMANCE");

		float fps = ImGui::GetIO().Framerate;
		float frameTime = 1000.0f / (fps > 0.0f ? fps : 1.0f);

		ImGui::Text("FPS: %.1f (%.2f ms)", fps, frameTime);
		if (lastLoadTimeMs_ > 0.0f)
		{
			ImGui::TextDisabled("Load: %.1f ms", lastLoadTimeMs_);
		}

		static float fpsHistory[60] = {};
		static int historyOffset = 0;
		fpsHistory[historyOffset] = fps;
		historyOffset = (historyOffset + 1) % IM_ARRAYSIZE(fpsHistory);

		ImGui::PlotLines("##FPSMiniGraph", fpsHistory, IM_ARRAYSIZE(fpsHistory), historyOffset, nullptr, 0.0f, 120.0f, ImVec2(-1, 42));
	}
	ImGui::End();

	// -------------------------------------------------------------
	// 2. [右ドック用] インスペクター (Inspector Content)
	// -------------------------------------------------------------
	if (ImGui::Begin("インスペクター"))
	{
		// 1. ゲーム・進行
		if (currentNavIndex == 0)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ ゲーム・進行・レール制御");
			ImGui::Separator();

			if (ImGui::CollapsingHeader("シーン切り替え (Scene Selector)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (auto sm = GetSceneManager())
				{
					sm->DrawSceneSelectorUI();
				}
			}

			ImGui::Spacing();
			if (ImGui::CollapsingHeader("ゲームプレイ制御 (Playback Control)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool doReset = false;

				if (isPlaying_)
				{
					if (ImGui::Button("一時停止 (Pause)", ImVec2(130, 36)))
					{
						isPlaying_ = false;
					}
					ImGui::SameLine();
					if (ImGui::Button("リセット (Reset)", ImVec2(110, 36)))
					{
						doReset = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("全画面 (F1で復帰)", ImVec2(140, 36)))
					{
						EngineServices::GetInstance()->SetEditorMode(false);
					}
					ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "状態: プレイ中 (PLAYING)");
				}
				else
				{
					if (ImGui::Button("開始 (Play)", ImVec2(130, 36)))
					{
						isPlaying_ = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("全画面プレイ", ImVec2(140, 36)))
					{
						isPlaying_ = true;
						EngineServices::GetInstance()->SetEditorMode(false);
					}
					ImGui::SameLine();
					if (ImGui::Button("リセット", ImVec2(100, 36)))
					{
						doReset = true;
					}
					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "状態: 停止中 (STOPPED - フリーカメラ可能)");
					ImGui::TextDisabled("※フリーカメラ: WASD/QE移動, 右クリックドラッグ回転");
				}

				ImGui::Separator();
				if (ImGui::Button("レベル再読込 (F5)", ImVec2(160, 32)))
				{
					ReloadLevel();
				}
				ImGui::SameLine();
				if (ImGui::Button("敵を全リスポーン", ImVec2(160, 32)))
				{
					ReloadEnemiesOnly();
				}

				ImGui::Separator();
				if (railCameraController_)
				{
					float p = railCameraController_->GetProgress();
					if (ImGui::SliderFloat("レール進行度 (Progress)", &p, 0.0f, 1.0f, "%.3f"))
					{
						railCameraController_->SetProgress(p);
					}
				}
				ImGui::SliderFloat("ゲーム速度 (Game Speed)", &baseGameSpeed_, 0.0f, 5.0f, "%.2fx");

				// リセット処理
				if (doReset)
				{
					isPlaying_ = false;
					if (railCameraController_)
					{
						railCameraController_->Reset();
					}
					if (!mainRails_.empty() && mainRails_[0]->IsValid())
					{
						Vector3 railForward = mainRails_[0]->GetForward(0.0f);
						float yaw = std::atan2(railForward.x, railForward.z);
						float pitch = std::asin(-railForward.y);
						float railTilt = mainRails_[0]->GetTilt(0.0f);
						currentCameraRot_ = { pitch, yaw, 0.0f };
						lastCameraYaw_ = yaw;
						currentCameraBank_ = railTilt;
					}
				}

				ImGui::Separator();
				ImGui::Checkbox("レール軌跡を表示 (Draw Rail)", &isDrawRail_);
				ImGui::SameLine();
				ImGui::Checkbox("コライダーを表示 (Draw Collider)", &isDrawCollider_);
			}

			if (railCameraController_)
			{
				ImGui::Spacing();
				if (ImGui::CollapsingHeader("レールカメラ設定 (Rail Camera)", ImGuiTreeNodeFlags_DefaultOpen))
				{
					railCameraController_->DrawImGuiContent();
				}
			}
		}
		// 2. プレイヤー
		else if (currentNavIndex == 1)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ プレイヤー設定・チート");
			ImGui::Separator();

			if (player_)
			{
				player_->DrawImGuiContent();
			}
			else
			{
				ImGui::TextDisabled("プレイヤーが存在しません。");
			}
		}
		// 3. 演出・シェーダー
		else if (currentNavIndex == 2)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ 演出・トランジション・シェーダー");
			ImGui::Separator();

			if (ImGui::CollapsingHeader("画面遷移 (Transition Settings)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (auto sm = GetSceneManager())
				{
					sm->DrawTransitionSettingsUI();
				}
			}

			ImGui::Spacing();
			if (ImGui::CollapsingHeader("ポストプロセス (Post Process)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (auto pp = EngineServices::GetInstance()->GetPostProcess())
				{
					pp->DrawImGuiContent();
				}
			}

			ImGui::Spacing();
			if (ImGui::CollapsingHeader("パーティクルエフェクト (Particles)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* items[] = { "Thruster (バーニア)", "Explosion (爆破)", "Hit (命中)", "Wind (風・スピード線)", "Trail (軌跡)" };
				ImGui::Combo("対象エフェクト", &currentEditEffectIndex_, items, IM_ARRAYSIZE(items));

				ImGui::Separator();
				if (currentEditEffectIndex_ == 0) thrusterEffect_.DrawImGui();
				else if (currentEditEffectIndex_ == 1) explosionEffect_.DrawImGui();
				else if (currentEditEffectIndex_ == 2) hitEffect_.DrawImGui();
				else if (currentEditEffectIndex_ == 3) windEffect_.DrawImGui();
				else if (currentEditEffectIndex_ == 4) trailEffect_.DrawImGui();
			}
		}
		// 4. シーン・照明
		else if (currentNavIndex == 3)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "■ カメラ・照明・配置モデル");
			ImGui::Separator();

			// カメラ設定
			if (ImGui::CollapsingHeader("カメラ詳細 (Active Camera)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (camera)
				{
					Vector3& camPosRef = camera->GetTranslate();
					float camPosArr[3] = { camPosRef.x, camPosRef.y, camPosRef.z };
					if (ImGui::DragFloat3("座標 (Translate)", camPosArr, 0.1f))
					{
						camera->SetTranslate(Vector3(camPosArr[0], camPosArr[1], camPosArr[2]));
					}

					Vector3& camRotRef = camera->GetRotation();
					float camRotArr[3] = { camRotRef.x, camRotRef.y, camRotRef.z };
					if (ImGui::DragFloat3("回転 (Rotation)", camRotArr, 0.1f))
					{
						camera->SetRotation(Vector3(camRotArr[0], camRotArr[1], camRotArr[2]));
					}

					float fov = camera->GetFovY();
					if (ImGui::DragFloat("画角 (FOV Y)", &fov, 0.01f, 0.01f, 3.14f))
					{
						camera->SetFovY(fov);
					}

					float aspect = camera->GetAspectRatio();
					if (ImGui::DragFloat("アスペクト比", &aspect, 0.01f, 0.1f, 10.0f))
					{
						camera->SetAspectRatio(aspect);
					}

					float nearC = camera->GetNearClip();
					if (ImGui::DragFloat("近クリップ", &nearC, 0.001f, 0.001f, 100.0f))
					{
						camera->SetNearClip(nearC);
					}

					float farC = camera->GetFarClip();
					if (ImGui::DragFloat("遠クリップ", &farC, 0.1f, 1.0f, 100000.0f))
					{
						camera->SetFarClip(farC);
					}
				}
			}

			// モデルとライトの準備
			std::vector<Object3d*> allModels;
			std::vector<std::string> modelNames;
			int index = 0;
			for (auto& obj : modelInstances)
			{
				if (obj)
				{
					allModels.push_back(obj.get());
					modelNames.push_back("Model " + std::to_string(index));
				}
				index++;
			}
			if (player_)
			{
				if (player_->GetObject3d())
				{
					allModels.push_back(player_->GetObject3d());
					modelNames.push_back("Player");
				}
				if (player_->GetReticle())
				{
					allModels.push_back(player_->GetReticle());
					modelNames.push_back("Player Reticle");
				}
			}

			// ライト設定
			ImGui::Spacing();
			if (ImGui::CollapsingHeader("グローバルライト設定 (Global Lights)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (!allModels.empty())
				{
					Object3d* firstObj = allModels[0];
					Model* sampleModel = firstObj ? firstObj->GetModel() : nullptr;
					int lightingMode = sampleModel ? sampleModel->GetSelectLightings() : 0;

					auto usesDirectional = [](int mode) { return mode >= 1 && mode <= 4; };
					auto usesPoint = [](int mode) { return mode == 4; };
					auto usesSpot = [](int mode) { return mode == 5; };

					// 平行光源
					if (usesDirectional(lightingMode))
					{
						ImGui::Text("平行光源 (Directional Light)");
						static bool dirInit = false;
						static Vector4 dirColor = { 1.0f, 1.0f, 1.0f, 1.0f };
						static Vector3 dirDirection = { -1.0f, -0.5f, 0.5f };
						static float dirIntensity = 1.0f;
						static bool dirEnabledGlobal = true;
						static float dirPrevIntensityGlobal = 1.0f;

						if (!dirInit)
						{
							dirColor = firstObj->GetDirectionalLightColor();
							dirDirection = firstObj->GetDirectionalLightDirection();
							dirIntensity = firstObj->GetDirectionalLightIntensity();
							dirPrevIntensityGlobal = dirIntensity;
							dirInit = true;
						}

						float dcArr[4] = { dirColor.x, dirColor.y, dirColor.z, dirColor.w };
						if (ImGui::ColorEdit4("色##Dir", dcArr))
						{
							dirColor = Vector4(dcArr[0], dcArr[1], dcArr[2], dcArr[3]);
							for (auto& m : allModels) m->SetDirectionalLightColor(dirColor);
						}

						float ddArr[3] = { dirDirection.x, dirDirection.y, dirDirection.z };
						if (ImGui::DragFloat3("方向##Dir", ddArr, 0.01f, -10.0f, 10.0f))
						{
							dirDirection = Vector3(ddArr[0], ddArr[1], ddArr[2]);
							for (auto& m : allModels) m->SetDirectionalLightDirection(dirDirection);
						}

						if (ImGui::DragFloat("強度##Dir", &dirIntensity, 0.01f, 0.0f, 100.0f))
						{
							if (dirEnabledGlobal)
							{
								for (auto& m : allModels) m->SetDirectionalLightIntensity(dirIntensity);
								dirPrevIntensityGlobal = dirIntensity;
							}
							else
							{
								dirPrevIntensityGlobal = dirIntensity;
							}
						}

						if (ImGui::Checkbox("平行光源を有効化##Dir", &dirEnabledGlobal))
						{
							if (!dirEnabledGlobal)
							{
								for (auto& m : allModels) m->SetDirectionalLightIntensity(0.0f);
							}
							else
							{
								for (auto& m : allModels) m->SetDirectionalLightIntensity(dirPrevIntensityGlobal);
							}
						}
					}

					// 点光源
					if (usesPoint(lightingMode))
					{
						ImGui::Separator();
						ImGui::Text("点光源 (Point Light)");
						static bool pointInit = false;
						static Vector4 pointColor = { 1.0f, 1.0f, 1.0f, 1.0f };
						static Vector3 pointPosition = { 0.0f, 1.0f, -8.0f };
						static float pointIntensity = 1.0f;
						static float prevPointIntensity = 1.0f;
						static float pointRadius = 15.0f;
						static float pointRange = 1.0f;
						static bool pointLightEnabled = true;

						if (!pointInit && firstObj)
						{
							pointColor = firstObj->GetPointLightColor();
							pointPosition = firstObj->GetPointLightPosition();
							pointIntensity = firstObj->GetPointLightIntensity();
							prevPointIntensity = pointIntensity;
							pointInit = true;
						}

						if (ImGui::Checkbox("点光源を有効化##Point", &pointLightEnabled))
						{
							if (!pointLightEnabled)
								for (auto& m : allModels) m->SetPointLightIntensity(0.0f);
							else
								for (auto& m : allModels) m->SetPointLightIntensity(prevPointIntensity);
						}

						ImGui::BeginDisabled(!pointLightEnabled);
						float pcArr[4] = { pointColor.x, pointColor.y, pointColor.z, pointColor.w };
						if (ImGui::ColorEdit4("色##Point", pcArr))
						{
							pointColor = Vector4(pcArr[0], pcArr[1], pcArr[2], pcArr[3]);
							for (auto& m : allModels) m->SetPointLightColor(pointColor);
						}
						float ppArr[3] = { pointPosition.x, pointPosition.y, pointPosition.z };
						if (ImGui::DragFloat3("位置##Point", ppArr, 0.05f, -100.0f, 100.0f))
						{
							pointPosition = Vector3(ppArr[0], ppArr[1], ppArr[2]);
							for (auto& m : allModels) m->SetPointLightPosition(pointPosition);
						}
						if (ImGui::DragFloat("強度##Point", &pointIntensity, 0.01f, 0.0f, 100.0f))
						{
							if (pointLightEnabled)
							{
								for (auto& m : allModels) m->SetPointLightIntensity(pointIntensity);
								prevPointIntensity = pointIntensity;
							}
							else
							{
								prevPointIntensity = pointIntensity;
							}
						}
						if (ImGui::DragFloat("半径##Point", &pointRadius, 0.01f, 0.1f, 100.0f))
						{
							for (auto& m : allModels) m->SetPointLightRadius(pointRadius);
						}
						if (ImGui::DragFloat("減衰範囲##Point", &pointRange, 0.01f, 0.1f, 50.0f))
						{
							for (auto& m : allModels) m->SetPointLightDecry(pointRange);
						}
						ImGui::EndDisabled();
					}

					// スポットライト
					if (usesSpot(lightingMode))
					{
						ImGui::Separator();
						ImGui::Text("スポットライト (Spot Light)");
						static bool spotInit = false;
						static Vector4 spotColor = { 1.0f, 1.0f, 1.0f, 1.0f };
						static Vector3 spotPosition = { 0.0f, 5.0f, 0.0f };
						static Vector3 spotDirection = { 0.0f, -1.0f, 0.0f };
						static float spotIntensity = 1.0f;
						static float prevSpotIntensity = 1.0f;
						static float spotDistance = 10.0f;
						static float spotDecay = 1.0f;
						static float spotAngleDeg = 45.0f;
						static bool spotLightEnabled = true;

						if (!spotInit && firstObj)
						{
							spotColor = firstObj->GetSpotLightColor();
							spotPosition = firstObj->GetSpotLightPosition();
							spotDirection = firstObj->GetSpotLightDirection();
							spotIntensity = firstObj->GetSpotLightIntensity();
							prevSpotIntensity = spotIntensity;
							spotDistance = firstObj->GetSpotLightDistance();
							spotDecay = firstObj->GetSpotLightDecay();
							spotAngleDeg = firstObj->GetSpotLightAngleDeg();
							spotInit = true;
						}

						if (ImGui::Checkbox("スポットライトを有効化##Spot", &spotLightEnabled))
						{
							if (!spotLightEnabled)
								for (auto& m : allModels) m->SetSpotLightIntensity(0.0f);
							else
								for (auto& m : allModels) m->SetSpotLightIntensity(prevSpotIntensity);
						}

						ImGui::BeginDisabled(!spotLightEnabled);
						float scArr[4] = { spotColor.x, spotColor.y, spotColor.z, spotColor.w };
						if (ImGui::ColorEdit4("色##Spot", scArr))
						{
							spotColor = Vector4(scArr[0], scArr[1], scArr[2], spotColor.w);
							for (auto& m : allModels) m->SetSpotLightColor(spotColor);
						}
						float spArr[3] = { spotPosition.x, spotPosition.y, spotPosition.z };
						if (ImGui::DragFloat3("位置##Spot", spArr, 0.05f, -100.0f, 100.0f))
						{
							spotPosition = Vector3(spArr[0], spArr[1], spArr[2]);
							for (auto& m : allModels) m->SetSpotLightPosition(spotPosition);
						}
						float sdArr[3] = { spotDirection.x, spotDirection.y, spotDirection.z };
						if (ImGui::DragFloat3("方向##Spot", sdArr, 0.01f, -10.0f, 10.0f))
						{
							spotDirection = Vector3(sdArr[0], sdArr[1], sdArr[2]);
							for (auto& m : allModels) m->SetSpotLightDirection(spotDirection);
						}
						if (ImGui::DragFloat("強度##Spot", &spotIntensity, 0.01f, 0.0f, 100.0f))
						{
							if (spotLightEnabled)
							{
								for (auto& m : allModels) m->SetSpotLightIntensity(spotIntensity);
								prevSpotIntensity = spotIntensity;
							}
							else
							{
								prevSpotIntensity = spotIntensity;
							}
						}
						if (ImGui::DragFloat("距離##Spot", &spotDistance, 0.1f, 0.0f, 10000.0f))
						{
							for (auto& m : allModels) m->SetSpotLightDistance(spotDistance);
						}
						if (ImGui::DragFloat("減衰率##Spot", &spotDecay, 0.01f, 0.0f, 10.0f))
						{
							for (auto& m : allModels) m->SetSpotLightDecay(spotDecay);
						}
						if (ImGui::SliderFloat("照射角度##Spot", &spotAngleDeg, 1.0f, 90.0f))
						{
							for (auto& m : allModels) m->SetSpotLightAngleDeg(spotAngleDeg);
						}
						ImGui::EndDisabled();
					}

					if (!usesDirectional(lightingMode) && !usesPoint(lightingMode) && !usesSpot(lightingMode))
					{
						ImGui::TextWrapped("現在のライティングモードでは、編集可能なライトがありません。");
					}
				}
			}

			// モデル一覧・トランスフォーム調整
			ImGui::Spacing();
			if (ImGui::CollapsingHeader("3Dモデル一覧・調整 (Model Inspector)"))
			{
				if (!allModels.empty())
				{
					static int currentModelIndex = 0;
					if (currentModelIndex >= (int)allModels.size()) currentModelIndex = 0;

					std::vector<const char*> namePtrs;
					for (const auto& name : modelNames)
					{
						namePtrs.push_back(name.c_str());
					}

					ImGui::Combo("対象モデル", &currentModelIndex, namePtrs.data(), (int)namePtrs.size());

					Object3d* obj = allModels[currentModelIndex];
					if (obj)
					{
						Vector3 t = obj->GetTranslate();
						float tArr[3] = { t.x, t.y, t.z };
						if (ImGui::DragFloat3("座標 (Translate)", tArr, 0.05f))
						{
							obj->SetTranslate(Vector3(tArr[0], tArr[1], tArr[2]));
						}

						Vector3 r = obj->GetRotation();
						float rArr[3] = { r.x, r.y, r.z };
						if (ImGui::DragFloat3("回転 (Rotation)", rArr, 0.5f))
						{
							obj->SetRotation(Vector3(rArr[0], rArr[1], rArr[2]));
						}

						Vector3 s = obj->GetScale();
						float sArr[3] = { s.x, s.y, s.z };
						if (ImGui::DragFloat3("スケール (Scale)", sArr, 0.01f, 0.001f, 100000.0f))
						{
							obj->SetScale(Vector3(sArr[0], sArr[1], sArr[2]));
						}

						if (obj->GetModel())
						{
							float envCoeff = obj->GetModel()->GetEnvironmentCoefficient();
							if (ImGui::DragFloat("環境反射係数", &envCoeff, 0.01f, 0.0f, 1.0f))
							{
								obj->SetEnvironmentCoefficient(envCoeff);
							}
						}
					}

					ImGui::Separator();
					Model* sampleModel = allModels[0]->GetModel();
					if (sampleModel)
					{
						int currentSelect = sampleModel->GetSelectLightings();
						const char* lightingNames[] = {
							"0: テクスチャのみ (TextureOnly)",
							"1: 平行光源・ディフューズ (Directional Diffuse)",
							"2: 平行光源・ソフト (Directional Soft)",
							"3: 平行光源・スペキュラ (Dir Diffuse+Specular)",
							"4: 平行光源 + 点光源 (Dir + Point)",
							"5: スポットライト (Spot)"
						};
						if (ImGui::Combo("ライティングモード (一括変更)", &currentSelect, lightingNames, IM_ARRAYSIZE(lightingNames)))
						{
							for (auto& mObj : allModels)
							{
								Model* m = mObj->GetModel();
								if (m) m->SetSelectLightings(currentSelect);
							}
						}
					}
				}
			}
		}
	}
	ImGui::End();

	// -------------------------------------------------------------
	// 3. [下ドック用] タイムライン (Timeline Editor)
	// -------------------------------------------------------------
	if (ImGui::Begin("タイムライン"))
	{
		float currentProgress = railCameraController_ ? railCameraController_->GetProgress() : 0.0f;
		float totalRailLength = (!mainRails_.empty() && mainRails_[0]->IsValid()) ? mainRails_[0]->GetTotalLength() : 1000.0f;
		// 想定総時間（秒）: 基準速度での概算時間
		float estimatedTotalTime = (totalRailLength > 0.0f) ? (totalRailLength / 40.0f) : 60.0f;
		float currentEstimatedTime = currentProgress * estimatedTotalTime;

		// 制御バー: 再生コントロール
		ImGui::BeginGroup();
		{
			// 先頭へ
			if (ImGui::Button("|< 0%##TL", ImVec2(55, 26)))
			{
				if (railCameraController_) railCameraController_->SetProgress(0.0f);
			}
			ImGui::SameLine();
			// -5%
			if (ImGui::Button("<< -5%##TL", ImVec2(65, 26)))
			{
				float newP = (std::max)(0.0f, currentProgress - 0.05f);
				if (railCameraController_) railCameraController_->SetProgress(newP);
			}
			ImGui::SameLine();
			// 再生 / 一時停止
			if (isPlaying_)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
				if (ImGui::Button("一時停止 (Pause)##TL", ImVec2(125, 26)))
				{
					isPlaying_ = false;
				}
				ImGui::PopStyleColor(2);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.65f, 0.35f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.78f, 0.42f, 1.0f));
				if (ImGui::Button("再生 (Play)##TL", ImVec2(125, 26)))
				{
					isPlaying_ = true;
				}
				ImGui::PopStyleColor(2);
			}
			ImGui::SameLine();
			// +5%
			if (ImGui::Button("+5% >>##TL", ImVec2(65, 26)))
			{
				float newP = (std::min)(1.0f, currentProgress + 0.05f);
				if (railCameraController_) railCameraController_->SetProgress(newP);
			}
			ImGui::SameLine();
			// 末尾へ
			if (ImGui::Button("100% >|##TL", ImVec2(65, 26)))
			{
				if (railCameraController_) railCameraController_->SetProgress(1.0f);
			}
			ImGui::SameLine();
			if (ImGui::Button("リセット##TL", ImVec2(65, 26)))
			{
				isPlaying_ = false;
				if (railCameraController_) railCameraController_->Reset();
				ReloadEnemiesOnly();
			}
			ImGui::SameLine();
			if (ImGui::Button("敵リスポーン##TL", ImVec2(95, 26)))
			{
				ReloadEnemiesOnly();
			}

			ImGui::SameLine();
			ImGui::TextDisabled("|");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(110.0f);
			ImGui::SliderFloat("速度##TL", &baseGameSpeed_, 0.1f, 3.0f, "%.2fx");
			ImGui::SameLine();
			if (ImGui::SmallButton("1.0x##TL")) baseGameSpeed_ = 1.0f;
			ImGui::SameLine();
			if (ImGui::SmallButton("2.0x##TL")) baseGameSpeed_ = 2.0f;
		}
		ImGui::EndGroup();

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "  進行度: %5.1f%% (%.1fs / %.1fs)", currentProgress * 100.0f, currentEstimatedTime, estimatedTotalTime);

		ImGui::Spacing();

		// タイムライン描画 (DrawList によるカスタムシーケンサーバー)
		ImVec2 canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, 34.0f);
		if (canvasSize.x < 100.0f) canvasSize.x = 100.0f;
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// 背景トラック
		drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(32, 34, 40, 255), 4.0f);
		drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(60, 65, 75, 255), 4.0f);

		// 狭小ゾーン (Narrow Zones) の帯を描画
		if (!mainRails_.empty() && mainRails_[0]->IsValid())
		{
			const auto& zones = mainRails_[0]->GetNarrowZones();
			for (const auto& zone : zones)
			{
				float x0 = canvasPos.x + zone.startT * canvasSize.x;
				float x1 = canvasPos.x + zone.endT * canvasSize.x;
				drawList->AddRectFilled(ImVec2(x0, canvasPos.y + 2.0f), ImVec2(x1, canvasPos.y + canvasSize.y - 2.0f), IM_COL32(230, 140, 40, 100), 2.0f);
				drawList->AddRect(ImVec2(x0, canvasPos.y + 2.0f), ImVec2(x1, canvasPos.y + canvasSize.y - 2.0f), IM_COL32(250, 170, 60, 200), 2.0f);
			}
		}

		// エネミーマーカーの収集と描画
		struct EnemyMarkerItem {
			float progress;
			std::string type;
			bool isDead;
			bool isActive;
		};
		std::vector<EnemyMarkerItem> enemyMarkers;

		const Rail* mainRail = (!mainRails_.empty() && mainRails_[0]->IsValid()) ? mainRails_[0].get() : nullptr;

		for (const auto& enemy : enemies_)
		{
			if (!enemy) continue;
			float sp = enemy->GetRailProgress();
			// 未設定(0.0f以下)の場合は、敵の配置座標またはパス始点から最寄りレール進行度を自動逆算
			if ((sp <= 0.0f || sp > 1.0f) && mainRail)
			{
				Vector3 basePos = enemy->GetSpawnPos();
				if (enemy->GetMovePath() && enemy->GetMovePath()->IsValid())
				{
					basePos = enemy->GetMovePath()->GetPosition(0.0f);
				}
				sp = mainRail->GetClosestProgress(basePos);
				// タイムライン用キャッシュに反映（敵のスポーン設定は変更しない）
				const_cast<Enemy*>(enemy.get())->SetRailProgress(sp);
			}

			if (sp >= 0.0f && sp <= 1.0f)
			{
				enemyMarkers.push_back({ sp, enemy->GetTypeName(), enemy->IsDead(), enemy->IsActive() });

				float ex = canvasPos.x + sp * canvasSize.x;
				// 状態に応じた色分け（待機中: 紫, 出現中: 黄金, 撃破済: 暗いグレー）
				ImU32 col = IM_COL32(185, 95, 255, 230);
				if (enemy->IsDead()) col = IM_COL32(110, 110, 120, 140);
				else if (enemy->IsActive()) col = IM_COL32(255, 215, 0, 255);

				drawList->AddLine(ImVec2(ex, canvasPos.y + 2.0f), ImVec2(ex, canvasPos.y + canvasSize.y - 2.0f), col, 1.5f);
				drawList->AddTriangleFilled(ImVec2(ex - 3.5f, canvasPos.y + 2.0f), ImVec2(ex + 3.5f, canvasPos.y + 2.0f), ImVec2(ex, canvasPos.y + 8.0f), col);
			}
		}

		// 目盛り (10% 刻み)
		for (int i = 1; i < 10; ++i)
		{
			float tx = canvasPos.x + (i * 0.1f) * canvasSize.x;
			drawList->AddLine(ImVec2(tx, canvasPos.y + canvasSize.y - 6.0f), ImVec2(tx, canvasPos.y + canvasSize.y - 1.0f), IM_COL32(100, 105, 120, 180), 1.0f);
		}

		// 現在の再生ヘッド（Playhead）
		float playheadX = canvasPos.x + currentProgress * canvasSize.x;
		drawList->AddLine(ImVec2(playheadX, canvasPos.y), ImVec2(playheadX, canvasPos.y + canvasSize.y), IM_COL32(255, 60, 60, 255), 2.5f);
		drawList->AddTriangleFilled(ImVec2(playheadX - 5.0f, canvasPos.y), ImVec2(playheadX + 5.0f, canvasPos.y), ImVec2(playheadX, canvasPos.y + 8.0f), IM_COL32(255, 80, 80, 255));

		// 見えないボタンを被せてマウスクリック＆ドラッグによるスクラブ操作を実現
		ImGui::InvisibleButton("##TimelineTrack", canvasSize);
		if (ImGui::IsItemActive() || ImGui::IsItemClicked())
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			float newT = (mousePos.x - canvasPos.x) / canvasSize.x;
			newT = (std::max)(0.0f, (std::min)(1.0f, newT));
			if (railCameraController_)
			{
				railCameraController_->SetProgress(newT);
			}
		}

		// ホバー時にツールチップ表示
		if (ImGui::IsItemHovered())
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			float hoverT = (mousePos.x - canvasPos.x) / canvasSize.x;
			hoverT = (std::max)(0.0f, (std::min)(1.0f, hoverT));

			std::string extraInfo = "";
			float minDiff = 0.035f; // 3.5%の範囲内の敵を検出
			int nearbyCount = 0;
			std::string nearestType = "";
			for (const auto& m : enemyMarkers)
			{
				if (std::abs(m.progress - hoverT) < minDiff)
				{
					nearbyCount++;
					if (nearestType.empty()) nearestType = m.type;
				}
			}
			if (nearbyCount > 0)
			{
				extraInfo = "\n[エネミー配置: " + nearestType + (nearbyCount > 1 ? (" ほか計" + std::to_string(nearbyCount) + "体") : "") + "]";
			}

			ImGui::SetTooltip("位置: %.1f%% (%.1fs)%s\nクリックでジャンプ", hoverT * 100.0f, hoverT * estimatedTotalTime, extraInfo.c_str());
		}

		// イベントマーカー凡例・クイックジャンプ
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "■ 狭小ゾーン (Orange)");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.75f, 0.45f, 1.0f, 1.0f), "  ■ エネミー配置 (Purple:待機 / Yellow:出現中 / Gray:撃破)");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  | 再生位置 (Red)");
		ImGui::SameLine();
		ImGui::TextDisabled(" [敵総数: %zu体]", enemyMarkers.size());

		if (!mainRails_.empty() && mainRails_[0]->IsValid())
		{
			const auto& zones = mainRails_[0]->GetNarrowZones();
			ImGui::TextDisabled("ジャンプ: ");

			if (!zones.empty())
			{
				for (size_t zIdx = 0; zIdx < zones.size(); ++zIdx)
				{
					ImGui::SameLine();
					char btnLabel[32];
					snprintf(btnLabel, sizeof(btnLabel), "ゾーン%zu (%.0f%%)##Jump", zIdx + 1, zones[zIdx].startT * 100.0f);
					if (ImGui::SmallButton(btnLabel))
					{
						if (railCameraController_) railCameraController_->SetProgress(zones[zIdx].startT);
					}
				}
			}

			// 敵の出現ポイントへのジャンプ（代表的なポイントを重複排除して最大5箇所）
			if (!enemyMarkers.empty())
			{
				std::vector<float> enemyJumpPoints;
				for (const auto& em : enemyMarkers)
				{
					bool isTooClose = false;
					for (float jp : enemyJumpPoints)
					{
						if (std::abs(jp - em.progress) < 0.05f) { isTooClose = true; break; }
					}
					if (!isTooClose)
					{
						enemyJumpPoints.push_back(em.progress);
						if (enemyJumpPoints.size() >= 5) break;
					}
				}
				std::sort(enemyJumpPoints.begin(), enemyJumpPoints.end());

				for (size_t eIdx = 0; eIdx < enemyJumpPoints.size(); ++eIdx)
				{
					ImGui::SameLine();
					char btnLabel[32];
					snprintf(btnLabel, sizeof(btnLabel), "敵群%zu (%.0f%%)##EJump", eIdx + 1, enemyJumpPoints[eIdx] * 100.0f);
					if (ImGui::SmallButton(btnLabel))
					{
						if (railCameraController_) railCameraController_->SetProgress(enemyJumpPoints[eIdx]);
					}
				}
			}
		}
	}
	ImGui::End();

#endif // USE_IMGUI

}

void GamePlayScene::Draw()
{
	auto services = EngineServices::GetInstance();
	auto object3dCommon = services->GetObject3dCommon();
	auto spriteCommon = services->GetSpriteCommon();

	if (skybox_)
	{
		skybox_->Draw();
	}

	// 視錐台（Frustum）カリング用の情報取得
	Frustum frustum{};
	bool enableCulling = false;
	/*
	if (activeCamera_)
	{
		Matrix4x4 vp = activeCamera_->GetViewMatrix() * activeCamera_->GetProjectionMatrix();
		frustum = Frustum::CreateFromViewProjection(vp);
		enableCulling = true;
	}
	*/

	if (object3dCommon) object3dCommon->SetCommonDrawSetting();

	for (auto& model : modelInstances)
	{
		if (!model) continue;

		if (enableCulling)
		{
			Vector3 center;
			float radius = 0.0f;
			if (model->GetBoundingSphere(center, radius))
			{
				// 半径が極端に巨大なオブジェクト（広域背景など）以外は視錐台カリング
				if (radius < 300.0f && !frustum.ContainsSphere(center, radius))
				{
					continue; // 画面外のため描画スキップ
				}
			}
		}

		model->Draw();
	}

	if (object3dCommon) object3dCommon->SetCommonDrawSetting();
	for (auto& enemy : enemies_)
	{
		if (enableCulling)
		{
			Vector3 ePos = enemy->GetPosition();
			if (!frustum.ContainsSphere(ePos, 20.0f))
			{
				continue; // 画面外スキップ
			}
		}
		enemy->Draw();
	}
	for (auto& obstacle : obstacles_)
	{
		if (enableCulling)
		{
			Vector3 oPos = obstacle->GetPosition();
			if (!frustum.ContainsSphere(oPos, 25.0f))
			{
				continue; // 画面外スキップ
			}
		}
		obstacle->Draw();
	}
	for (auto& ring : enhanceRings_)
	{
		if (enableCulling)
		{
			Vector3 rPos = ring->GetPosition();
			if (!frustum.ContainsSphere(rPos, 20.0f))
			{
				continue; // 画面外スキップ
			}
		}
		ring->Draw();
	}


	if (isDrawCollider_)
	{
		if (object3dCommon) object3dCommon->SetWireframeDrawSetting();
		for (auto& enemy : enemies_)
		{
			enemy->DrawCollider();
		}
		for (auto& obstacle : obstacles_)
		{
			obstacle->DrawCollider();
		}
		for (auto& ring : enhanceRings_)
		{
			ring->DrawCollider();
		}
		for (auto& bullet : bullets_)
		{
			bullet->DrawCollider();
		}
		for (auto& missile : missiles_)
		{
			missile->DrawCollider();
		}
		for (auto& bullet : enemyBullets_)
		{
			bullet->DrawCollider();
		}
		if (player_)
		{
			player_->DrawCollider();
		}

		if (object3dCommon) object3dCommon->SetCommonDrawSetting();
	}


	for (auto& bullet : bullets_)
	{
		bullet->Draw();
	}
	for (auto& missile : missiles_)
	{
		missile->Draw();
	}
	for (auto& bullet : enemyBullets_)
	{
		bullet->Draw();
	}


	if (player_)
	{
		if (object3dCommon) object3dCommon->SetCommonDrawSetting();
		player_->Draw();
	}


	if (isDrawRail_)
	{
		if (object3dCommon) object3dCommon->SetCommonDrawSetting();
		for (auto& vis : railVisualizers_)
		{
			if (vis) vis->Draw();
		}
		for (auto& vis : enemyRailVisualizers_)
		{
			if (vis) vis->Draw();
		}
	}

	thrusterEffect_.Draw();
	explosionEffect_.Draw();
	hitEffect_.Draw();
	dodgeEffect_.Draw();
	trailEffect_.Draw();
	missileSmokeEffect_.Draw();
	ringEffect_.Draw();
	healRingEffect_.Draw();
	windEffect_.Draw();
}

void GamePlayScene::DrawUI()
{
	auto services = EngineServices::GetInstance();
	auto spriteCommon = services->GetSpriteCommon();
	if (spriteCommon) spriteCommon->SetCommonDrawSetting();
	auto srvManager = services->GetSrvManager();
	if (srvManager) srvManager->PreDraw();
	if (hpBarBgSprite_) { hpBarBgSprite_->Update(); hpBarBgSprite_->Draw(); }
	if (hpBarSprite_) { hpBarSprite_->Update(); hpBarSprite_->Draw(); }
	if (isDisplaySprite) { for (auto& sprite : sprites) if (sprite) { sprite->Update(); sprite->Draw(); } }


}