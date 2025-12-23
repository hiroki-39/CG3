#pragma once
#include "KHEngine/Graphics/3d/Model/ModelCommon.h"

#include "KHEngine/Math/MathCommon.h"

class Model
{
public: //構造体

	struct VertexData
	{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;

		bool operator==(const VertexData& other) const
		{
			return position.x == other.position.x && position.y == other.position.y && position.z == other.position.z &&
				texcoord.x == other.texcoord.x && texcoord.y == other.texcoord.y &&
				normal.x == other.normal.x && normal.y == other.normal.y && normal.z == other.normal.z;
		}
	};

	struct Material
	{
		Vector4 color;
		bool enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		int32_t selectLightings;
	};

	struct MaterialData
	{
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct  ModelData
	{
		std::vector<VertexData> vertices;
		std::vector<uint32_t> indices;
		MaterialData material;
	};

public: //メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(ModelCommon* modelCommon);


	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

private: //メンバ関数

	/// <summary>
	///　頂点バッファ・インデックスバッファの作成
	/// </summary>
	void CreateBufferResource();

	/// <summary>
	/// マテリアルの作成
	/// </summary>
	void CreateMaterialResource();

	/// <summary>
	/// objファイルの読み込み
	/// </summary>
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// mtlファイルの読み込み
	/// </summary>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

private: //メンバ変数

	// DirectXCommon取得
	DirectXCommon* dxCommon = nullptr;

	//ModelCommonのポインタ
	ModelCommon* modelCommon = nullptr;

	// objファイルのデータ
	ModelData modelData;

	// ---- バッファリソース ----

	// 頂点バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	// 頂点データの仮想アドレス
	VertexData* vertexData_ = nullptr;
	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;


	// インデックスバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	// インデックスデータの仮想アドレス
	uint32_t* indexData_ = nullptr;
	// インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW indexBufferView;


	// マテリアルリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

	// マテリアルデータの仮想アドレス
	Material* materialData_ = nullptr;

};

namespace std
{
	template <>
	struct hash<Model::VertexData>
	{
		size_t operator()(const Model::VertexData& v) const
		{
			size_t h1 = hash<float>()(v.position.x) ^ hash<float>()(v.position.y) ^ hash<float>()(v.position.z);
			size_t h2 = hash<float>()(v.texcoord.x) ^ hash<float>()(v.texcoord.y);
			size_t h3 = hash<float>()(v.normal.x) ^ hash<float>()(v.normal.y) ^ hash<float>()(v.normal.z);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}
