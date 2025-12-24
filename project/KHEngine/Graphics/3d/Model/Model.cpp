#include "Model.h"
#include "KHEngine/Graphics/Resource/TextureManager.h"
#include <fstream>
#include <sstream>

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename)
{
	// 引数で受け取ってメンバ変数に記録
	this->modelCommon = modelCommon;
	assert(this->modelCommon != nullptr);

	dxCommon = modelCommon->GetDirectXCommon();
	assert(dxCommon != nullptr);

	//モデルの読み込み(Plane.ogj)
	modelData = LoadObjFile(directoryPath, filename);
	

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
	dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureIndex));

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
	materialData_->selectLightings = 2;

	//単位行列を書き込む
	materialData_->uvTransform = Matrix4x4::Identity();
}

Model::ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	/*--- 1.中で必要となる変数の宣言 ---*/
	//構成するモデルデータ
	ModelData modelData;
	//位置
	std::vector<Vector4> positions;
	//法線
	std::vector<Vector3> normals;
	//テクスチャ座標
	std::vector<Vector2> texcoords;
	//ファイルから読んだ1行を格納するもの
	std::string line;

	// 頂点データ 
	std::unordered_map<VertexData, uint32_t> vertexToIndex;


	/*--- 2.ファイルを開く ---*/
	//ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);

	//開かなかったら止める
	assert(file.is_open());

	/*--- 3.実際にファイルを読み、ModelDataを構築していく ---*/
	while (std::getline(file, line))
	{
		std::string identfier;
		std::istringstream s(line);

		//先頭の識別子を読む
		s >> identfier;

		/* "V" : 頂点位置
		   "vt": 頂点テクスチャ座標
		   "vn": 頂点法線
		   "f" : 面
		*/

		//頂点情報を読む
		if (identfier == "v")
		{
			Vector4 position;

			s >> position.x >> position.y >> position.z;

			//位置のxを反転させ、左手座標系にする
			position.x *= -1.0f;

			position.w = 1.0f;

			positions.push_back(position);
		}
		else if (identfier == "vt")
		{
			Vector2 texcoord;

			s >> texcoord.x >> texcoord.y;

			//テクスチャのyを反転させ、左手座標系にする
			texcoord.y = 1.0f - texcoord.y;

			texcoords.push_back(texcoord);
		}
		else if (identfier == "vn")
		{
			Vector3 normal;

			s >> normal.x >> normal.y >> normal.z;

			//法線のxを反転させ、左手座標系にする
			normal.x *= -1.0f;

			normals.push_back(normal);
		}
		else if (identfier == "f")
		{
			// 今から読み込む三角形の3つのインデックスを一時的に格納
			std::array<uint32_t, 3> indices;

			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex)
			{
				std::string VertexDefinition;
				s >> VertexDefinition;

				// 頂点定義を分解（位置 / UV / 法線）に分ける
				std::istringstream v(VertexDefinition);

				// 頂点情報（位置/UV/法線）のインデックス（OBJは1始まりなので後で-1する）
				uint32_t posIndex = 0;
				uint32_t texIndex = 0;
				uint32_t normIndex = 0;

				// スラッシュの位置を検索（例: 1/2/3 → 1が位置, 2がUV, 3が法線）
				size_t firstSlash = VertexDefinition.find('/');
				size_t secondSlash = VertexDefinition.find('/', firstSlash + 1);

				if (firstSlash != std::string::npos && secondSlash != std::string::npos)
				{
					// フォーマット: v/vt/vn
					std::string posStr = VertexDefinition.substr(0, firstSlash);
					std::string texStr = VertexDefinition.substr(firstSlash + 1, secondSlash - firstSlash - 1);
					std::string normStr = VertexDefinition.substr(secondSlash + 1);

					if (!posStr.empty())
					{
						posIndex = std::stoi(posStr);
					}

					if (!texStr.empty())
					{
						texIndex = std::stoi(texStr);
					}

					if (!normStr.empty())
					{
						normIndex = std::stoi(normStr);
					}
				}
				else if (firstSlash != std::string::npos && VertexDefinition.find("//") != std::string::npos)
				{
					// フォーマット: v//vn（UVなし）
					std::string posStr = VertexDefinition.substr(0, firstSlash);
					std::string normStr = VertexDefinition.substr(firstSlash + 2); // "//" のあと

					if (!posStr.empty())
					{
						posIndex = std::stoi(posStr);
					}

					if (!normStr.empty())
					{
						normIndex = std::stoi(normStr);
					}
				}
				else if (firstSlash != std::string::npos)
				{
					// フォーマット: v/vt（法線なし）
					std::string posStr = VertexDefinition.substr(0, firstSlash);
					std::string texStr = VertexDefinition.substr(firstSlash + 1);

					if (!posStr.empty())
					{
						posIndex = std::stoi(posStr);
					}

					if (!texStr.empty())
					{
						texIndex = std::stoi(texStr);
					}
				}
				else
				{
					// フォーマット: v（位置のみ）
					posIndex = std::stoi(VertexDefinition);
				}

				// インデックスを使って頂点情報を取得
				Vector4 position = positions[posIndex - 1];
				Vector2 texcoord = texIndex ? texcoords[texIndex - 1] : Vector2{ 0.0f, 0.0f };
				Vector3 normal = normIndex ? normals[normIndex - 1] : Vector3{ 0.0f, 0.0f, 0.0f };



				// この3要素を1つの頂点データとしてまとめる
				VertexData vertex{ position, texcoord, normal };

				// 頂点が未登録なら新規追加、すでにあるなら再利用
				if (vertexToIndex.count(vertex) == 0)
				{
					uint32_t newIndex = static_cast<uint32_t>(modelData.vertices.size());

					vertexToIndex[vertex] = newIndex;

					// 頂点リストに追加
					modelData.vertices.push_back(vertex);
				}

				// 頂点に対応するインデックスを三角形インデックス配列に保存
				indices[faceVertex] = vertexToIndex[vertex];
			}

			// 頂点の並び順を反転して左手系に対応
			modelData.indices.push_back(indices[2]);
			modelData.indices.push_back(indices[1]);
			modelData.indices.push_back(indices[0]);

		}
		else if (identfier == "mtllib")
		{
			//materialTemplateLibraryファイルの名前を取得
			std::string materialFilename;
			s >> materialFilename;

			//基本的にobjファイルと同一階層にmtlは存在させるので、
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);

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