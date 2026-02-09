#include "Model.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <fstream>
#include <sstream>
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
	//modelData = LoadObjFile(directoryPath, filename);
	modelData = LoadObjFile("./resources", "plane.gltf");

	// 頂点データ・インデックスデータの作成
	CreateBufferResource();

	//マテリアルの作成
	CreateMaterialResource();

	// .objの参照しているテクスチャを読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);

	// 読み込んだテクスチャの番号を取得
	modelData.material.textureIndex =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

void Model::Draw()
{
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
	materialData_->selectLightings = 4;

	//単位行列を書き込む
	materialData_->uvTransform = Matrix4x4::Identity();

	//鏡面反射の強さ
	materialData_->shininess = 40.0f;

	materialData_->specularColor = { 1.0f,1.0f,1.0f };
}

Model::ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	ModelData modelData;
	Model model;

	/*--- 1.ファイルの読み込み ---*/
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());

	modelData.rootNode = model.ReadNode(scene->mRootNode);

	/*--- 2.ノード情報の読み込み ---*/
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		aiMesh* mesh = scene->mMeshes[meshIndex];
		//法線がないmeshは対応しない
		assert(mesh->HasNormals());
		//テクスチャ座標がないmeshは対応しない
		assert(mash->hasTextureCoords(0));

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
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

				VertexData vertex;

				vertex.position = { -position.x, position.y, position.z, 1.0f };
				vertex.normal = { -normal.x, normal.y, normal.z };
				vertex.texcoord = { 1.0f - texcoord.x,texcoord.y };

				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;

				uint32_t newIndex = static_cast<uint32_t>(modelData.vertices.size());
				modelData.vertices.push_back(vertex);
				newIndices[element] = newIndex;
			}

			// ワインディングを反転してインデックスを追加（0,2,1）
			modelData.indices.push_back(newIndices[0]);
			modelData.indices.push_back(newIndices[2]);
			modelData.indices.push_back(newIndices[1]);
		}
	}

	/*--- 3.マテリアル情報の読み込み ---*/

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
	{
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0)
		{
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
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
	assert(file.is_open());

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

