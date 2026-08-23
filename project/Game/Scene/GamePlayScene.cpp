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

static void CreateObjectFromNode(const LevelObjectData& node, const Object3d* parentObj, std::vector<std::unique_ptr<Object3d>>& instances, std::vector<std::unique_ptr<Rail>>& outRails, Object3dCommon* common, uint32_t skyboxTexIndex, std::list<std::unique_ptr<Enemy>>& enemies, std::list<std::unique_ptr<Obstacle>>& obstacles, std::vector<Enemy*> parentEnemies = {})
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

	bool isObstacle = (node.fileName.find("Obstacle") != std::string::npos) || (node.fileName.find("Invisible") != std::string::npos) || (node.fileName.find("ColliderOnly") != std::string::npos) || (node.fileName.find("Ring") != std::string::npos);
	bool isEnemy = (node.fileName.find("Fighter") != std::string::npos || node.fileName.find("Asteroid") != std::string::npos || node.fileName.find("Enemy") != std::string::npos);

	if (isObstacle)
	{

		auto obstacle = std::make_unique<Obstacle>();

		Vector3 rotRad;
		rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
		rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
		rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
		obstacle->Initialize(common, node.translation, node.scale, rotRad, node.fileName, skyboxTexIndex, node.collider, node.isDestructible);
		// オブジェクトのスポーン進行度を設定
        obstacle->SetSpawnProgress(node.spawnProgress);
		// ファイル名に "Ring" が含まれている場合は、強化リングとしてフラグを立てる
        obstacle->SetIsRing(node.fileName.find("Ring") != std::string::npos);
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
			modelName = node.name + ".obj";
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
		CreateObjectFromNode(child, currentObj, instances, outRails, common, skyboxTexIndex, enemies, obstacles, currentEnemies);
	}
}

static void LoadEnemiesOnlyFromNode(const LevelObjectData& node, Object3dCommon* common, uint32_t skyboxTexIndex, std::list<std::unique_ptr<Enemy>>& enemies, std::list<std::unique_ptr<Obstacle>>& obstacles, std::vector<Enemy*> parentEnemies = {})
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

	bool isObstacle = (node.fileName.find("Obstacle") != std::string::npos) || (node.fileName.find("Invisible") != std::string::npos) || (node.fileName.find("ColliderOnly") != std::string::npos) || (node.fileName.find("Ring") != std::string::npos);
	bool isEnemy = (node.fileName.find("Fighter") != std::string::npos || node.fileName.find("Asteroid") != std::string::npos || node.fileName.find("Enemy") != std::string::npos);

	if (isObstacle)
	{
		auto obstacle = std::make_unique<Obstacle>();
		Vector3 rotRad;
		rotRad.x = node.rotation.x * (std::numbers::pi_v<float> / 180.0f);
		rotRad.y = node.rotation.y * (std::numbers::pi_v<float> / 180.0f);
		rotRad.z = node.rotation.z * (std::numbers::pi_v<float> / 180.0f);
		obstacle->Initialize(common, node.translation, node.scale, rotRad, node.fileName, skyboxTexIndex, node.collider, node.isDestructible);
		// オブジェクトのスポーン進行度を設定
        obstacle->SetSpawnProgress(node.spawnProgress);
		// ファイル名に "Ring" が含まれている場合は、強化リングとしてフラグを立てる
        obstacle->SetIsRing(node.fileName.find("Ring") != std::string::npos);
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
		LoadEnemiesOnlyFromNode(child, common, skyboxTexIndex, enemies, obstacles, currentEnemies);
	}
}

void GamePlayScene::Initialize()
{

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


	ParticleManager::GetInstance()->RegisterQuad("quad", "resources/circle.png");
	ParticleManager::GetInstance()->RegisterRing("ring", "gradationLine.png", 32, 0.5f, 1.0f);
	ParticleManager::GetInstance()->RegisterCylinder("Cylinder", "resources/sprites/gradationLine.png");

	uint32_t instancingSrvIndex = UINT32_MAX;

	auto texManager = TextureManager::GetInstance();
	dxCommon->BeginTextureUploadBatch();


	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon, "resources/skybox.dds");


	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("Cube.obj");
	ModelManager::GetInstance()->LoadModel("cube.obj");
	ModelManager::GetInstance()->LoadModel("monsterBall.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");
	ModelManager::GetInstance()->LoadModel("player.obj");
	ModelManager::GetInstance()->LoadModel("suzanne.obj");


	texManager->LoadTexture("uvChecker.png");
	texManager->LoadTexture("monsterBall.png");
	texManager->LoadTexture("checkerBoard.png");
	texManager->LoadTexture("resources/skybox.dds");
	texManager->LoadTexture("circle.png");
	texManager->LoadTexture("circle2.png");
	texManager->LoadTexture("gradationLine.png");
	texManager->LoadTexture("sprites/white.png");


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

	ReloadLevel();

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


	texManager->ExecuteUploadCommands();
	texManager->ClearIntermediateResources();
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
			CreateObjectFromNode(objData, nullptr, modelInstances, mainRails_, object3dCommon, skybox_->GetCubemapSrvIndex(), enemies_, obstacles_);
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
			LoadEnemiesOnlyFromNode(objData, object3dCommon, skybox_->GetCubemapSrvIndex(), enemies_, obstacles_);
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
					pp->GetData()->radialBlurIntensity = 0.1f;
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


				const Matrix4x4& wMat = player_->GetColliderObject()->GetmatWorld();
				Vector3 pWorldPos = { wMat.m[3][0], wMat.m[3][1], wMat.m[3][2] };
				Vector3 playerBoxSize = player_->GetColliderSize();


				Matrix4x4 rotMat = wMat;
				for (int i = 0; i < 3; ++i)
				{
					float len = std::sqrt(rotMat.m[i][0] * rotMat.m[i][0] + rotMat.m[i][1] * rotMat.m[i][1] + rotMat.m[i][2] * rotMat.m[i][2]);
					if (len > 0.0001f)
					{
						rotMat.m[i][0] /= len;
						rotMat.m[i][1] /= len;
						rotMat.m[i][2] /= len;
					}
				}

				OBB playerOBB = CollisionMath::CreateOBB(pWorldPos, playerBoxSize, rotMat);

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


		for (auto it = obstacles_.begin(); it != obstacles_.end();)
		{
			(*it)->Update();

			if ((*it)->GetIsRing() && !(*it)->IsDead() && player_ && !player_->IsDead())
			{
				Sphere playerSphere = { player_->GetTranslate(), player_->GetColliderSize().x };
				if ((*it)->CheckCollision(playerSphere))
				{
					// プレイヤーのHPを回復させる
                    player_->Heal(3000);
					// プレイヤーの弾を2連装にパワーアップさせる
                    player_->PowerUp();
					// 取得されたリングを消滅させる
                    (*it)->Kill();
				}
			}


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
	}

#ifdef USE_IMGUI

	ImGui::Begin("Game Control");

	bool doReset = false;


	if (isPlaying_)
	{
		if (ImGui::Button("Stop (Pause)", ImVec2(120, 40)))
		{
			isPlaying_ = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset", ImVec2(120, 40)))
		{
			doReset = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Full Screen (Hide UI)", ImVec2(160, 40)))
		{
			EngineServices::GetInstance()->SetEditorMode(false);
		}
		ImGui::Text("Status: PLAYING");
	}
	else
	{
		if (ImGui::Button("Play (Start)", ImVec2(120, 40)))
		{
			isPlaying_ = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Play Full Screen", ImVec2(160, 40)))
		{
			isPlaying_ = true;
			EngineServices::GetInstance()->SetEditorMode(false);
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset", ImVec2(120, 40)))
		{
			doReset = true;
		}
		ImGui::Text("Status: STOPPED (Free Camera Mode)");
		ImGui::Text("Camera Control: WASD/QE to move, Right-Click Drag to rotate");
	}

	ImGui::Separator();
	if (ImGui::Button("Reload Level (F5)", ImVec2(160, 40)))
	{
		ReloadLevel();
	}
	ImGui::SameLine();
	if (ImGui::Button("Respawn Enemies", ImVec2(160, 40)))
	{
		ReloadEnemiesOnly();
	}

	ImGui::Separator();
	if (railCameraController_)
	{
		float p = railCameraController_->GetProgress();
		if (ImGui::SliderFloat("繧�E�繝ｼ繝譎る俣 (Rail Progress)", &p, 0.0f, 1.0f))
		{
			railCameraController_->SetProgress(p);
		}
	}
	ImGui::SliderFloat("繧�E�繝ｼ繝繧�E�繝斐・繝�E(Game Speed)", &baseGameSpeed_, 0.0f, 5.0f);


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
	ImGui::Checkbox("繝ｬ繝ｼ繝ｫ繧定｡�E�遉ｺ (Draw Rail)", &isDrawRail_);
	ImGui::Checkbox("繧�E�繝ｩ繧�E�繝繝ｼ繧定｡�E�遉ｺ (Draw Collider)", &isDrawCollider_);
	ImGui::End();

	if (player_)
	{
		player_->DrawUI();

		if (hpBarSprite_)
		{
			float hpRate = (float)player_->GetHp() / player_->GetMaxHp();
			if (hpRate < 0.0f) hpRate = 0.0f;
			hpBarSprite_->SetSize(Vector2(400.0f * hpRate, 32.0f));


			if (hpRate > 0.5f)
			{
				hpBarSprite_->SetColor(Vector4(0.0f, 1.0f, 0.0f, 1.0f));
			}
			else if (hpRate > 0.2f)
			{
				hpBarSprite_->SetColor(Vector4(1.0f, 1.0f, 0.0f, 1.0f));
			}
			else
			{
				hpBarSprite_->SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
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


    ImGui::Begin("モデル");
	if (!allModels.empty())
	{
		static int currentModelIndex = 0;
		if (currentModelIndex >= allModels.size()) currentModelIndex = 0;

		std::vector<const char*> namePtrs;
		for (const auto& name : modelNames)
		{
			namePtrs.push_back(name.c_str());
		}

        ImGui::Combo("対象モデル (Target Model)", &currentModelIndex, namePtrs.data(), (int)namePtrs.size());

		Object3d* obj = allModels[currentModelIndex];


		Vector3 t = obj->GetTranslate();
		float tArr[3] = { t.x, t.y, t.z };
		if (ImGui::DragFloat3("蠎ｧ讓�E(Translate)", tArr, 0.05f))
		{
			obj->SetTranslate(Vector3(tArr[0], tArr[1], tArr[2]));
		}

		Vector3 r = obj->GetRotation();
		float rArr[3] = { r.x, r.y, r.z };
		if (ImGui::DragFloat3("蝗櫁E���E� (Rotation)", rArr, 0.5f))
		{
			obj->SetRotation(Vector3(rArr[0], rArr[1], rArr[2]));
		}

		Vector3 s = obj->GetScale();
		float sArr[3] = { s.x, s.y, s.z };
		if (ImGui::DragFloat3("繧�E�繧�E�繝ｼ繝ｫ (Scale)", sArr, 0.01f, 0.001f, 100000.0f))
		{
			obj->SetScale(Vector3(sArr[0], sArr[1], sArr[2]));
		}

		ImGui::Separator();


		if (obj->GetModel())
		{
			float envCoeff = obj->GetModel()->GetEnvironmentCoefficient();
            if (ImGui::DragFloat("環境光係数 (Environment Coeff)", &envCoeff, 0.01f, 0.0f, 1.0f))
			{
				obj->SetEnvironmentCoefficient(envCoeff);
			}
		}

		ImGui::Separator();


		Model* sampleModel = allModels[0]->GetModel();
		if (sampleModel)
		{
			int currentSelect = sampleModel->GetSelectLightings();


			const char* lightingNames[] = {
                "0: テクスチャのみ (TextureOnly)",
                "1: 平行光源 拡散反射 (Directional Diffuse)",
                "2: 平行光源 ソフト (Directional Soft)",
                "3: 平行光源 スペキュラ (Directional Specular)",
                "4: 統合ライト (All Lights)"
				"5: 繧�E�繝昴ャ繝医Λ繧�E�繝�E(Spot)"
			};


            if (ImGui::Combo("ライティングモード(一括変更)", &currentSelect, lightingNames, IM_ARRAYSIZE(lightingNames)))
			{

				for (auto& mObj : allModels)
				{
					Model* m = mObj->GetModel();
					if (m) m->SetSelectLightings(currentSelect);
				}
			}
		}
	}
	ImGui::End();




    ImGui::Begin("カメラ");
	if (camera)
	{
		Vector3& camPosRef = camera->GetTranslate();
		float camPosArr[3] = { camPosRef.x, camPosRef.y, camPosRef.z };
		if (ImGui::DragFloat3("蠎ｧ讓�E(Translate)", camPosArr, 0.1f))
		{
			camera->SetTranslate(Vector3(camPosArr[0], camPosArr[1], camPosArr[2]));
		}

		Vector3& camRotRef = camera->GetRotation();
		float camRotArr[3] = { camRotRef.x, camRotRef.y, camRotRef.z };
		if (ImGui::DragFloat3("蝗櫁E���E� (Rotation)", camRotArr, 0.1f))
		{
			camera->SetRotation(Vector3(camRotArr[0], camRotArr[1], camRotArr[2]));
		}

		float fov = camera->GetFovY();
		if (ImGui::DragFloat("逕ｻ隗�E(FOV Y)", &fov, 0.01f, 0.01f, 3.14f))
		{
			camera->SetFovY(fov);
		}

		float aspect = camera->GetAspectRatio();
		if (ImGui::DragFloat("Aspect", &aspect, 0.01f, 0.1f, 10.0f))
		{
			camera->SetAspectRatio(aspect);
		}

		float nearC = camera->GetNearClip();
		if (ImGui::DragFloat("霑代け繝ｪ繝�E・", &nearC, 0.001f, 0.001f, 100.0f))
		{
			camera->SetNearClip(nearC);
		}

		float farC = camera->GetFarClip();
		if (ImGui::DragFloat("驕繧�E�繝ｪ繝�E・", &farC, 0.1f, 1.0f, 100000.0f))
		{
			camera->SetFarClip(farC);
		}
	}
	ImGui::End();



	ImGui::Begin("GlobalLight");
	if (!allModels.empty())
	{
		Object3d* firstObj = allModels[0];
		Model* sampleModel = firstObj ? firstObj->GetModel() : nullptr;
		int lightingMode = sampleModel ? sampleModel->GetSelectLightings() : 0;


		auto usesDirectional = [](int mode)
			{
				return mode == 1 || mode == 2 || mode == 3 || mode == 4;
			};
		auto usesPoint = [](int mode)
			{
				return mode == 4;
			};
		auto usesSpot = [](int mode)
			{
				return mode == 5;
			};


		if (usesDirectional(lightingMode))
		{
			ImGui::Separator();
			ImGui::Text("蟷�E�陦悟�E貁E�E(Directional Light)");

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
			if (ImGui::ColorEdit4("濶�E� (Color)##Dir", dcArr))
			{
				dirColor = Vector4(dcArr[0], dcArr[1], dcArr[2], dcArr[3]);
				for (auto& m : allModels) m->SetDirectionalLightColor(dirColor);
			}

			float ddArr[3] = { dirDirection.x, dirDirection.y, dirDirection.z };
			if (ImGui::DragFloat3("譁E��蜷・(Direction)##Dir", ddArr, 0.01f, -10.0f, 10.0f))
			{
				dirDirection = Vector3(ddArr[0], ddArr[1], ddArr[2]);
				for (auto& m : allModels) m->SetDirectionalLightDirection(dirDirection);
			}

			if (ImGui::DragFloat("蠑ｷ蠎ｦ (Intensity)##Dir", &dirIntensity, 0.01f, 0.0f, 100.0f))
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

			if (ImGui::Checkbox("蟷�E�陦悟�E貁E�E�E�譛牙柑蛹・(global)##Dir", &dirEnabledGlobal))
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


		if (usesPoint(lightingMode))
		{
			ImGui::Separator();
			ImGui::Text("轤�E�蜈画�E�・(Point Light)");

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

			if (ImGui::Checkbox("轤�E�蜈画�E�舌ｒ譛牙柑蛹・#Point", &pointLightEnabled))
			{
				if (!pointLightEnabled)
				{
					for (auto& m : allModels) m->SetPointLightIntensity(0.0f);
				}
				else
				{
					for (auto& m : allModels) m->SetPointLightIntensity(prevPointIntensity);
				}
			}

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
			ImGui::BeginDisabled(!pointLightEnabled);
#else
			if (!pointLightEnabled)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
#endif

			float pcArr[4] = { pointColor.x, pointColor.y, pointColor.z, pointColor.w };
			if (ImGui::ColorEdit4("濶�E� (Color)##Point", pcArr))
			{
				pointColor = Vector4(pcArr[0], pcArr[1], pcArr[2], pcArr[3]);
				for (auto& m : allModels) m->SetPointLightColor(pointColor);
			}

			float ppArr[3] = { pointPosition.x, pointPosition.y, pointPosition.z };
			if (ImGui::DragFloat3("菴咲�E��E� (Position)##Point", ppArr, 0.05f, -100.0f, 100.0f))
			{
				pointPosition = Vector3(ppArr[0], ppArr[1], ppArr[2]);
				for (auto& m : allModels) m->SetPointLightPosition(pointPosition);
			}

			if (ImGui::DragFloat("蠑ｷ蠎ｦ (Intensity)##Point", &pointIntensity, 0.01f, 0.0f, 100.0f))
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

			if (ImGui::DragFloat("蜊雁�E�・(Radius)##Point", &pointRadius, 0.01f, 0.1f, 100.0f))
			{
				for (auto& m : allModels) m->SetPointLightRadius(pointRadius);
			}
			if (ImGui::DragFloat("貂幁E���E�遽・峁E(Decay Range)##Point", &pointRange, 0.01f, 0.1f, 50.0f))
			{
				for (auto& m : allModels) m->SetPointLightDecry(pointRange);
			}

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
			ImGui::EndDisabled();
#else
			if (!pointLightEnabled)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
#endif
		}


		if (usesSpot(lightingMode))
		{
			ImGui::Separator();
			ImGui::Text("繧�E�繝昴ャ繝医Λ繧�E�繝�E(Spot Light)");

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

			if (ImGui::Checkbox("繧�E�繝昴ャ繝医Λ繧�E�繝医�E�譛牙柑蛹・#Spot", &spotLightEnabled))
			{
				if (!spotLightEnabled)
				{
					for (auto& m : allModels) m->SetSpotLightIntensity(0.0f);
				}
				else
				{
					for (auto& m : allModels) m->SetSpotLightIntensity(prevSpotIntensity);
				}
			}

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
			ImGui::BeginDisabled(!spotLightEnabled);
#else
			if (!spotLightEnabled)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
#endif


			{
				float scArr[4] = { spotColor.x, spotColor.y, spotColor.z, spotColor.w };
				if (ImGui::ColorEdit4("濶�E� (Color)##Spot", scArr))
				{
					spotColor = Vector4(scArr[0], scArr[1], scArr[2], scArr[3]);
					for (auto& m : allModels) m->SetSpotLightColor(spotColor);
				}
			}


			{
				float spArr[3] = { spotPosition.x, spotPosition.y, spotPosition.z };
				if (ImGui::DragFloat3("菴咲�E��E� (Position)##Spot", spArr, 0.05f, -100.0f, 100.0f))
				{
					spotPosition = Vector3(spArr[0], spArr[1], spArr[2]);
					for (auto& m : allModels) m->SetSpotLightPosition(spotPosition);
				}
			}


			{
				float sdArr[3] = { spotDirection.x, spotDirection.y, spotDirection.z };
				if (ImGui::DragFloat3("譁E��蜷・(Direction)##Spot", sdArr, 0.01f, -10.0f, 10.0f))
				{
					spotDirection = Vector3(sdArr[0], sdArr[1], sdArr[2]);
					for (auto& m : allModels) m->SetSpotLightDirection(spotDirection);
				}
			}


			if (ImGui::DragFloat("蠑ｷ蠎ｦ (Intensity)##Spot", &spotIntensity, 0.01f, 0.0f, 100.0f))
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


			if (ImGui::DragFloat("霍晞屬 (Distance)##Spot", &spotDistance, 0.1f, 0.0f, 10000.0f))
			{
				for (auto& m : allModels) m->SetSpotLightDistance(spotDistance);
			}
			if (ImGui::DragFloat("貂幁E���E�邁E�E(Decay)##Spot", &spotDecay, 0.01f, 0.0f, 10.0f))
			{
				for (auto& m : allModels) m->SetSpotLightDecay(spotDecay);
			}


			if (ImGui::SliderFloat("隗貞ｺ�E� (Angle deg)##Spot", &spotAngleDeg, 1.0f, 90.0f))
			{
				for (auto& m : allModels) m->SetSpotLightAngleDeg(spotAngleDeg);
			}

#if defined(IMGUI_VERSION) && (IMGUI_VERSION_NUM >= 18000)
			ImGui::EndDisabled();
#else
			if (!spotLightEnabled)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
#endif
		}


		if (!usesDirectional(lightingMode) && !usesPoint(lightingMode) && !usesSpot(lightingMode))
		{
			ImGui::TextWrapped("No editable lights for this model.");
		}
	}
	ImGui::End();



	ImGui::Begin("Effect Selector");
	const char* items[] = { "Thruster", "Explosion", "Hit" };
	ImGui::Combo("Edit Target", &currentEditEffectIndex_, items, IM_ARRAYSIZE(items));
	ImGui::End();

	if (currentEditEffectIndex_ == 0) thrusterEffect_.DrawImGui();
	else if (currentEditEffectIndex_ == 1) explosionEffect_.DrawImGui();
	else if (currentEditEffectIndex_ == 2) hitEffect_.DrawImGui();


#endif 

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

	if (object3dCommon) object3dCommon->SetCommonDrawSetting();

	for (auto& model : modelInstances) if (model) model->Draw();



	if (object3dCommon) object3dCommon->SetCommonDrawSetting();
	for (auto& enemy : enemies_)
	{
		enemy->Draw();
	}
	for (auto& obstacle : obstacles_)
	{
		obstacle->Draw();
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
}

void GamePlayScene::DrawUI()
{
	auto services = EngineServices::GetInstance();
	auto spriteCommon = services->GetSpriteCommon();
	if (spriteCommon) spriteCommon->SetCommonDrawSetting();
	if (hpBarBgSprite_) { hpBarBgSprite_->Draw(); }
	if (hpBarSprite_) { hpBarSprite_->Draw(); }
	if (isDisplaySprite) { for (auto& sprite : sprites) if (sprite) sprite->Draw(); }
}



