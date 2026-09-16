#include "Model.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

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

	// .objの参照しているテクスチャを読み込み（存在しない場合はデフォルトテクスチャにフォールバック）
	if (!modelData.material.textureFilePath.empty())
	{
		TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
		modelData.material.textureIndex =
			TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
	}
	else
	{
		// テクスチャ未指定のモデルはデフォルトテクスチャを使って描画する
		modelData.material.textureIndex = TextureManager::GetInstance()->GetDefaultTextureIndex();
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

void Model::Draw()
{
	if (modelData.vertices.empty() || modelData.indices.empty()) return;

	//VBVの設定
	dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	//IBVの設定
	dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	//CBVの設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	//SRVのDescriptorTableの先頭を設定
	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, modelData.material.textureIndex);

	//描画！
	dxCommon->GetCommandList()->DrawIndexedInstanced(UINT(modelData.indices.size()), 1, 0, 0, 0);
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
	//マテリアル用のリソースを作る
	materialResource_ = dxCommon->CreateBufferResource(sizeof(Material));

	//書き込む為のアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	//色の設定
	materialData_->color = { 1.0f,1.0f,1.0f,1.0f };

	//Lightingを有効化
	materialData_->enableLighting = true;

	//Lightingの種類の設定
	materialData_->selectLightings = 2;

	//単位行列を書き込む
	materialData_->uvTransform = Matrix4x4::Identity();

	//鏡面反射の強さ
	materialData_->shininess = 40.0f;
	materialData_->specularColor = { 1.0f,1.0f,1.0f };
	materialData_->environmentCoefficient = 0.0f;
	materialData_->fresnelF0 = 0.04f; // 非金属のデフォルト
}

Model::ModelData Model::LoadObjFile(const std::string & directoryPath, const std::string & filename)
{
	ModelData modelData;
	Model model;

	/*--- 1.ファイルの読み込み ---*/
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
	assert(scene->HasMeshes());

	/*--- 3.マテリアル情報の読み込み ---*/

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
	{
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
		{
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);

			std::string texPath = textureFilePath.C_Str();
			std::string fullPath = directoryPath + "/" + texPath;
			if (std::filesystem::exists(fullPath)) {
				modelData.material.textureFilePath = fullPath;
			} else {
				modelData.material.textureFilePath = texPath;
			}
		}
	}

	// Assimpがテクスチャを見つけられなかった場合のフォールバック
	if (modelData.material.textureFilePath.empty())
	{
		std::string mtlFilename = filename.substr(0, filename.find_last_of('.')) + ".mtl";
		MaterialData fallbackMat = LoadMaterialTemplateFile(directoryPath, mtlFilename);
		if (!fallbackMat.textureFilePath.empty())
		{
			std::string fullPath = fallbackMat.textureFilePath;
			if (std::filesystem::exists(fullPath)) {
				modelData.material.textureFilePath = fullPath;
			}
		}
	}

	modelData.rootNode = model.ReadNode(scene->mRootNode);

	/*--- 2.ノード情報の読み込み ---*/
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		aiMesh* mesh = scene->mMeshes[meshIndex];
		//法線がないmeshは対応しない
		assert(mesh->HasNormals());
		for (uint32_t faceINdex = 0; faceINdex < mesh->mNumFaces; ++faceINdex)
		{
			aiFace& face = mesh->mFaces[faceINdex];
			// 三角形のみ対応
			assert(face.mNumIndices == 3);

			// 1フェース分の新規インデックスを一時保存
			uint32_t newIndices[3];

			for (uint32_t element = 0; element < face.mNumIndices; ++element)
			{
				uint32_t vertexIndex = face.mIndices[element];

				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				
				VertexData vertex;
				vertex.position = { position.x, position.y, -position.z, 1.0f }; // RH to LH: Zを反転
				vertex.normal = { normal.x, normal.y, -normal.z }; // RH to LH: 法線のZも反転

				// テクスチャ座標があるかチェック
				if (mesh->HasTextureCoords(0)) {
					aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
					vertex.texcoord = { texcoord.x, texcoord.y };
				}
				else {
					vertex.texcoord = { 0.0f, 0.0f };
				}

				uint32_t newIndex = static_cast<uint32_t>(modelData.vertices.size());
				modelData.vertices.push_back(vertex);
				newIndices[element] = newIndex;
			}

			// Zを反転してLH座標系にしたため、面が裏返らないように頂点のインデックス順を逆（0, 2, 1）にします。
			modelData.indices.push_back(newIndices[0]);
			modelData.indices.push_back(newIndices[2]);
			modelData.indices.push_back(newIndices[1]);
		}
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

