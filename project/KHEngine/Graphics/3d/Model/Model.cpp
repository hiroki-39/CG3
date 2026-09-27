#include "Model.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename)
{
	// 引数で受け取ってメンバ変数に記録
	this->modelCommon = modelCommon;
	assert(this->modelCommon != nullptr);

	dxCommon = modelCommon->GetDirectXCommon();
	assert(dxCommon != nullptr);

	//モデルの読み込み
	modelData = LoadObjFile(directoryPath, filename);

	// 頂点データ・インデックスデータの作成
	CreateBufferResource();

	//マテリアルの作成
	CreateMaterialResource();

	// 全マテリアルのテクスチャを読み込み
	for (auto& mat : modelData.materials)
	{
		if (!mat.textureFilePath.empty())
		{
			TextureManager::GetInstance()->LoadTexture(mat.textureFilePath);
			mat.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(mat.textureFilePath);
		}
		else
		{
			mat.textureIndex = TextureManager::GetInstance()->GetDefaultTextureIndex();
		}
	}

	// 単一マテリアル（後方互換性）の更新
	if (!modelData.materials.empty())
	{
		modelData.material = modelData.materials[0];
	}
	else
	{
		if (!modelData.material.textureFilePath.empty())
		{
			TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
			modelData.material.textureIndex =
				TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
		}
		else
		{
			modelData.material.textureIndex = TextureManager::GetInstance()->GetDefaultTextureIndex();
		}
	}
}

void Model::Initialize(DirectXCommon* dxCommon, const ModelData& data)
{
	// ModelCommonは使わないのでnullptrのままでOK
	this->modelCommon = nullptr;
	assert(dxCommon != nullptr);
	this->dxCommon = dxCommon;

	// モデルデータをコピー
	modelData = data;

	// バッファ作成
	CreateBufferResource();

	// マテリアル作成
	CreateMaterialResource();

	// テクスチャはスカイボックス側で管理するのでここでは何もしない
	modelData.material.textureIndex = TextureManager::GetInstance()->GetDefaultTextureIndex();
}

void Model::Draw(D3D12_GPU_VIRTUAL_ADDRESS materialCBV)
{
	if (modelData.vertices.empty() || modelData.indices.empty()) return;

	// VBVの設定
	dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	// IBVの設定
	dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	// サブメッシュが存在しない場合（スカイボックスや手動生成モデルなど）は従来通り一括描画
	if (modelData.subMeshes.empty())
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbv = (materialCBV != 0) ? materialCBV : materialResource_->GetGPUVirtualAddress();
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, cbv);
		SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, modelData.material.textureIndex);
		dxCommon->GetCommandList()->DrawIndexedInstanced(UINT(modelData.indices.size()), 1, 0, 0, 0);
	}
	else
	{
		// サブメッシュごとにマテリアル/テクスチャを切り替えて描画（マルチマテリアル対応）
		for (const auto& subMesh : modelData.subMeshes)
		{
			if (subMesh.indexCount == 0) continue;

			uint32_t matIdx = subMesh.materialIndex;
			uint32_t texIdx = modelData.material.textureIndex;
			D3D12_GPU_VIRTUAL_ADDRESS cbv = (materialCBV != 0) ? materialCBV : materialResource_->GetGPUVirtualAddress();

			if (matIdx < modelData.materials.size())
			{
				const auto& mat = modelData.materials[matIdx];
				texIdx = mat.textureIndex;
				if (materialCBV == 0 && mat.materialResource)
				{
					cbv = mat.materialResource->GetGPUVirtualAddress();
				}
			}

			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, cbv);
			SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, texIdx);
			dxCommon->GetCommandList()->DrawIndexedInstanced(subMesh.indexCount, 1, subMesh.startIndex, 0, 0);
		}
	}
}

void Model::CreateBufferResource()
{
	if (modelData.vertices.empty() || modelData.indices.empty()) return;

	/*--- 頂点バッファ用リソースを作る ---*/

	//頂点リソースを作る
	vertexResource_ = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

	//リソースの先頭からアドレスから使う
	vertexBufferView.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点サイズ
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	//1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	//頂点リソースにデータを書き込む
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	std::memcpy(vertexData_, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	/*--- インデックスバッファ用リソースを作る ---*/

	//頂点リソースを作る
	indexResource_ = dxCommon->CreateBufferResource(sizeof(uint32_t) * modelData.indices.size());

	//リソースの先頭からアドレスから使う
	indexBufferView.BufferLocation = indexResource_->GetGPUVirtualAddress();

	//使用するリソースのサイズは頂点サイズ
	indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * modelData.indices.size());

	//1頂点あたりのサイズ
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	//頂点リソースにデータを書き込む
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
	std::memcpy(indexData_, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
	indexResource_->Unmap(0, nullptr);

}

void Model::CreateMaterialResource()
{
	// マテリアル用の共有リソースを作る（単一マテリアルやフォールバック用）
	materialResource_ = dxCommon->CreateBufferResource(sizeof(Material));

	// 書き込む為のアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	// 色の設定
	materialData_->color = { 1.0f,1.0f,1.0f,1.0f };

	// Lightingを有効化
	materialData_->enableLighting = true;

	// Lightingの種類の設定
	materialData_->selectLightings = 2;

	// 単位行列を書き込む
	materialData_->uvTransform = Matrix4x4::Identity();

	// 鏡面反射の強さ
	materialData_->shininess = 40.0f;
	materialData_->specularColor = { 1.0f,1.0f,1.0f };
	materialData_->environmentCoefficient = 0.0f;
	materialData_->fresnelF0 = 0.04f; // 非金属のデフォルト

	// 各マテリアルごとのConstantBufferリソース作成
	for (auto& mat : modelData.materials)
	{
		mat.materialResource = dxCommon->CreateBufferResource(sizeof(Material));
		mat.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&mat.materialData));
		mat.materialData->color = { 1.0f,1.0f,1.0f,1.0f };
		mat.materialData->enableLighting = true;
		mat.materialData->selectLightings = 2;
		mat.materialData->uvTransform = Matrix4x4::Identity();
		mat.materialData->shininess = 40.0f;
		mat.materialData->specularColor = { 1.0f,1.0f,1.0f };
		mat.materialData->environmentCoefficient = 0.0f;
		mat.materialData->fresnelF0 = 0.04f;
	}
}

Model::ModelData Model::LoadObjFile(const std::string & directoryPath, const std::string & filename)
{
	ModelData modelData;
	Model model;

	/*--- 1.ファイルの読み込み ---*/
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
	if (!scene || !scene->HasMeshes())
	{
		return modelData;
	}

	// テクスチャファイルパスを解決するラムダヘルパー（大文字小文字無視・スペルミスフォールバック対応）
	auto resolveTexturePath = [&](const std::string& texName) -> std::string {
		if (texName.empty()) return "";
		std::string fullPath = directoryPath + "/" + texName;
		if (std::filesystem::exists(fullPath)) return fullPath;
		if (std::filesystem::exists(texName)) return texName;

		try {
			std::string filenameOnly = std::filesystem::path(texName).filename().string();
			for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
				if (entry.is_regular_file()) {
					std::string entryName = entry.path().filename().string();
					if (_stricmp(entryName.c_str(), filenameOnly.c_str()) == 0) {
						return entry.path().string();
					}
				}
			}
			// yellow / yallow のスペル違いフォールバック
			if (filenameOnly.find("yellow") != std::string::npos || filenameOnly.find("yallow") != std::string::npos) {
				std::string alt1 = directoryPath + "/yellow.png";
				std::string alt2 = directoryPath + "/yallow.png";
				if (std::filesystem::exists(alt1)) return alt1;
				if (std::filesystem::exists(alt2)) return alt2;
			}
		} catch (...) {}

		return fullPath;
	};

	/*--- 3.マテリアル情報の読み込み ---*/
	modelData.materials.resize(scene->mNumMaterials);
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
	{
		aiMaterial* material = scene->mMaterials[materialIndex];
		aiString matName;
		material->Get(AI_MATKEY_NAME, matName);
		modelData.materials[materialIndex].name = matName.C_Str();

		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
		{
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.materials[materialIndex].textureFilePath = resolveTexturePath(textureFilePath.C_Str());
		}
	}

	// Assimpがテクスチャを見つけられなかった場合のフォールバック（.mtl直接パース）
	bool anyTextureFound = false;
	for (const auto& m : modelData.materials) {
		if (!m.textureFilePath.empty()) { anyTextureFound = true; break; }
	}

	if (!anyTextureFound)
	{
		std::string mtlFilename = filename.substr(0, filename.find_last_of('.')) + ".mtl";
		std::vector<MaterialData> mtlMats;
		std::ifstream mtlFile(directoryPath + "/" + mtlFilename);
		if (mtlFile.is_open())
		{
			std::string line;
			MaterialData curMat;
			bool inMat = false;
			while (std::getline(mtlFile, line))
			{
				std::istringstream s(line);
				std::string ident;
				s >> ident;
				if (ident == "newmtl")
				{
					if (inMat) mtlMats.push_back(curMat);
					curMat = MaterialData();
					s >> curMat.name;
					inMat = true;
				}
				else if (ident == "map_Kd")
				{
					std::string texName;
					s >> texName;
					curMat.textureFilePath = resolveTexturePath(texName);
				}
			}
			if (inMat) mtlMats.push_back(curMat);
		}

		for (size_t i = 0; i < mtlMats.size() && i < modelData.materials.size(); ++i)
		{
			if (modelData.materials[i].textureFilePath.empty())
			{
				modelData.materials[i].textureFilePath = mtlMats[i].textureFilePath;
			}
		}
		if (modelData.materials.empty() && !mtlMats.empty())
		{
			modelData.materials = mtlMats;
		}
	}

	if (!modelData.materials.empty())
	{
		modelData.material = modelData.materials[0];
	}

	modelData.rootNode = model.ReadNode(scene->mRootNode);

	/*--- 2.ノード・頂点情報の読み込み ---*/
	size_t totalVertices = 0;
	size_t totalIndices = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		aiMesh* mesh = scene->mMeshes[meshIndex];
		totalVertices += mesh->mNumVertices;
		totalIndices += mesh->mNumFaces * 3;
	}
	modelData.vertices.reserve(totalVertices);
	modelData.indices.reserve(totalIndices);

	Vector3 minPos = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
	Vector3 maxPos = { -(std::numeric_limits<float>::max)(), -(std::numeric_limits<float>::max)(), -(std::numeric_limits<float>::max)() };

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		aiMesh* mesh = scene->mMeshes[meshIndex];
		uint32_t baseVertexIndex = static_cast<uint32_t>(modelData.vertices.size());
		uint32_t startSubMeshIndex = static_cast<uint32_t>(modelData.indices.size());

		for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
		{
			const aiVector3D& pos = mesh->mVertices[v];
			aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0.0f, 1.0f, 0.0f);

			VertexData vertex;
			vertex.position = { pos.x, pos.y, -pos.z, 1.0f }; // RH to LH: Zを反転
			vertex.normal = { normal.x, normal.y, -normal.z }; // RH to LH: 法線のZも反転

			if (mesh->HasTextureCoords(0)) {
				const aiVector3D& texcoord = mesh->mTextureCoords[0][v];
				vertex.texcoord = { texcoord.x, texcoord.y };
			}
			else {
				vertex.texcoord = { 0.0f, 0.0f };
			}

			modelData.vertices.push_back(vertex);

			minPos.x = (std::min)(minPos.x, vertex.position.x);
			minPos.y = (std::min)(minPos.y, vertex.position.y);
			minPos.z = (std::min)(minPos.z, vertex.position.z);
			maxPos.x = (std::max)(maxPos.x, vertex.position.x);
			maxPos.y = (std::max)(maxPos.y, vertex.position.y);
			maxPos.z = (std::max)(maxPos.z, vertex.position.z);
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			const aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices != 3) continue;

			// LH座標系のため、面が裏返らないように頂点インデックス順を反転（0, 2, 1）
			modelData.indices.push_back(baseVertexIndex + face.mIndices[0]);
			modelData.indices.push_back(baseVertexIndex + face.mIndices[2]);
			modelData.indices.push_back(baseVertexIndex + face.mIndices[1]);
		}

		uint32_t subMeshIndexCount = static_cast<uint32_t>(modelData.indices.size()) - startSubMeshIndex;
		if (subMeshIndexCount > 0)
		{
			SubMesh subMesh;
			subMesh.indexCount = subMeshIndexCount;
			subMesh.startIndex = startSubMeshIndex;
			subMesh.materialIndex = mesh->mMaterialIndex;
			modelData.subMeshes.push_back(subMesh);
		}
	}

	// バウンディング中心と半径を算出
	if (!modelData.vertices.empty())
	{
		modelData.boundingCenter = {
			(minPos.x + maxPos.x) * 0.5f,
			(minPos.y + maxPos.y) * 0.5f,
			(minPos.z + maxPos.z) * 0.5f
		};
		float maxDistSq = 0.0f;
		for (const auto& v : modelData.vertices)
		{
			float dx = v.position.x - modelData.boundingCenter.x;
			float dy = v.position.y - modelData.boundingCenter.y;
			float dz = v.position.z - modelData.boundingCenter.z;
			float distSq = dx * dx + dy * dy + dz * dz;
			if (distSq > maxDistSq) maxDistSq = distSq;
		}
		modelData.boundingRadius = std::sqrt(maxDistSq);
	}

	/*--- 4.Modeldataを返す ---*/
	return modelData;
}

Model::MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	/*---	1.中で必要となる変数の宣言	---*/

	//構築するMaterialData
	MaterialData materialData;

	//ファイルから読み込んだ1行を格納用
	std::string line;

	/*---	2.ファイルを開く	---*/

	//ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);

	//とりあえず開かなかったら止める
	if (!file.is_open()) {
		return materialData;
	}

	/*---	3.実際にファイルを読み	---*/

	//ファイルを読み、MaterialDataを構築
	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);

		s >> identifier;

		//identfierに応じた処理
		if (identifier == "map_Kd")
		{
			std::string textureFilename;
			s >> textureFilename;

			//連結してファイルをパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;

		}
	}


	/*---	4.MaterialDataを返す	---*/

	return  materialData;
}

Model::ModelData Model::CreateSkyboxModelData()
{
	ModelData modelData;
	// 頂点データ (x,y,z,w)
	modelData.vertices = {
		// 前
		{{-1.0f, -1.0f, -1.0f, 1.0f}},
		{{-1.0f, +1.0f, -1.0f, 1.0f}},
		{{+1.0f, +1.0f, -1.0f, 1.0f}},
		{{+1.0f, -1.0f, -1.0f, 1.0f}},
		// 後
		{{-1.0f, -1.0f, +1.0f, 1.0f}},
		{{-1.0f, +1.0f, +1.0f, 1.0f}},
		{{+1.0f, +1.0f, +1.0f, 1.0f}},
		{{+1.0f, -1.0f, +1.0f, 1.0f}},
	};

	// インデックスデータ
	modelData.indices = {
		0, 1, 2, 0, 2, 3, // 前
		4, 6, 5, 4, 7, 6, // 後
		4, 5, 1, 4, 1, 0, // 左
		3, 2, 6, 3, 6, 7, // 右
		1, 5, 6, 1, 6, 2, // 上
		4, 0, 3, 4, 3, 7, // 下
	};
	return modelData;
}

void Model::SetColor(const Vector4& color)
{
	if (materialData_) {
		materialData_->color = color;
	}
	for (auto& mat : modelData.materials) {
		if (mat.materialData) {
			mat.materialData->color = color;
		}
	}
}

void Model::SetEnableLighting(bool enable)
{
	if (materialData_) {
		materialData_->enableLighting = enable ? 1 : 0;
	}
	for (auto& mat : modelData.materials) {
		if (mat.materialData) {
			mat.materialData->enableLighting = enable ? 1 : 0;
		}
	}
}

void Model::SetSelectLightings(int32_t v)
{
	if (materialData_) {
		materialData_->selectLightings = v;
	}
	for (auto& mat : modelData.materials) {
		if (mat.materialData) {
			mat.materialData->selectLightings = v;
		}
	}
}

void Model::SetEnvironmentCoefficient(float v)
{
	if (materialData_) {
		materialData_->environmentCoefficient = v;
	}
	for (auto& mat : modelData.materials) {
		if (mat.materialData) {
			mat.materialData->environmentCoefficient = v;
		}
	}
}

int32_t Model::GetSelectLightings() const
{
	return materialData_ ? materialData_->selectLightings : 0;
}

float Model::GetEnvironmentCoefficient() const
{
	return materialData_ ? materialData_->environmentCoefficient : 0.0f;
}

Model::Node Model::ReadNode(aiNode* node)
{

	Node result{};

	// nodeのLocalMatrixを取得
	aiMatrix4x4 aiLocalMatrix = node->mTransformation;

	// 列ベクトル形式 を行ベクトル形式に倒置
	aiLocalMatrix.Transpose();

	// 他の要素も同様に
	result.localMatrix.m[0][0] = aiLocalMatrix[0][0];
	result.localMatrix.m[0][1] = aiLocalMatrix[0][1];
	result.localMatrix.m[0][2] = aiLocalMatrix[0][2];
	result.localMatrix.m[0][3] = aiLocalMatrix[0][3];

	result.localMatrix.m[1][0] = aiLocalMatrix[1][0];
	result.localMatrix.m[1][1] = aiLocalMatrix[1][1];
	result.localMatrix.m[1][2] = aiLocalMatrix[1][2];
	result.localMatrix.m[1][3] = aiLocalMatrix[1][3];

	result.localMatrix.m[2][0] = aiLocalMatrix[2][0];
	result.localMatrix.m[2][1] = aiLocalMatrix[2][1];
	result.localMatrix.m[2][2] = aiLocalMatrix[2][2];
	result.localMatrix.m[2][3] = aiLocalMatrix[2][3];

	result.localMatrix.m[3][0] = aiLocalMatrix[3][0];
	result.localMatrix.m[3][1] = aiLocalMatrix[3][1];
	result.localMatrix.m[3][2] = aiLocalMatrix[3][2];
	result.localMatrix.m[3][3] = aiLocalMatrix[3][3];

	// Node名を格納
	result.name = node->mName.C_Str();

	// 子供の数だけ確保
	result.children.resize(node->mNumChildren);

	// 再帰的に読んで階層構造を作る
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}

std::vector<Triangle> Model::GetWorldTriangles(const Matrix4x4& worldMat) const {
	std::vector<Triangle> triangles;
	const auto& vertices = modelData.vertices;
	const auto& indices = modelData.indices;

	auto TransformPos = [&](const Vector4& p) -> Vector3 {
		float x = p.x * worldMat.m[0][0] + p.y * worldMat.m[1][0] + p.z * worldMat.m[2][0] + worldMat.m[3][0];
		float y = p.x * worldMat.m[0][1] + p.y * worldMat.m[1][1] + p.z * worldMat.m[2][1] + worldMat.m[3][1];
		float z = p.x * worldMat.m[0][2] + p.y * worldMat.m[1][2] + p.z * worldMat.m[2][2] + worldMat.m[3][2];
		float w = p.x * worldMat.m[0][3] + p.y * worldMat.m[1][3] + p.z * worldMat.m[2][3] + worldMat.m[3][3];
		if (std::abs(w) > 0.00001f) {
			return { x / w, y / w, z / w };
		}
		return { x, y, z };
	};

	if (!indices.empty()) {
		triangles.reserve(indices.size() / 3);
		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			uint32_t i0 = indices[i];
			uint32_t i1 = indices[i + 1];
			uint32_t i2 = indices[i + 2];
			if (i0 < vertices.size() && i1 < vertices.size() && i2 < vertices.size()) {
				Triangle tri;
				tri.p0 = TransformPos(vertices[i0].position);
				tri.p1 = TransformPos(vertices[i1].position);
				tri.p2 = TransformPos(vertices[i2].position);

				Vector3 e1 = { tri.p1.x - tri.p0.x, tri.p1.y - tri.p0.y, tri.p1.z - tri.p0.z };
				Vector3 e2 = { tri.p2.x - tri.p0.x, tri.p2.y - tri.p0.y, tri.p2.z - tri.p0.z };
				Vector3 normal = {
					e1.y * e2.z - e1.z * e2.y,
					e1.z * e2.x - e1.x * e2.z,
					e1.x * e2.y - e1.y * e2.x
				};
				float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
				if (len > 0.00001f) {
					tri.normal = { normal.x / len, normal.y / len, normal.z / len };
				} else {
					tri.normal = { 0.0f, 1.0f, 0.0f };
				}
				triangles.push_back(tri);
			}
		}
	} else {
		triangles.reserve(vertices.size() / 3);
		for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
			Triangle tri;
			tri.p0 = TransformPos(vertices[i].position);
			tri.p1 = TransformPos(vertices[i + 1].position);
			tri.p2 = TransformPos(vertices[i + 2].position);

			Vector3 e1 = { tri.p1.x - tri.p0.x, tri.p1.y - tri.p0.y, tri.p1.z - tri.p0.z };
			Vector3 e2 = { tri.p2.x - tri.p0.x, tri.p2.y - tri.p0.y, tri.p2.z - tri.p0.z };
			Vector3 normal = {
				e1.y * e2.z - e1.z * e2.y,
				e1.z * e2.x - e1.x * e2.z,
				e1.x * e2.y - e1.y * e2.x
			};
			float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
			if (len > 0.00001f) {
				tri.normal = { normal.x / len, normal.y / len, normal.z / len };
			} else {
				tri.normal = { 0.0f, 1.0f, 0.0f };
			}
			triangles.push_back(tri);
		}
	}

	return triangles;
}

