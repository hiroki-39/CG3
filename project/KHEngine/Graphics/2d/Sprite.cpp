#include "Sprite.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"

void Sprite::Initialize(SpriteCommon* spriteCommon)
{
	//引数で受け取ってメンバ変数に記録する
	this->spriteCommon_ = spriteCommon;

	//頂点バッファ・インデックスバッファの作成
	CreateBufferResource();

}

void Sprite::CreateBufferResource()
{
	assert(spriteCommon_ != nullptr);

	// DirectXCommon取得
	DirectXCommon* dxCommon = spriteCommon_->GetDirectXCommon();
	assert(dxCommon != nullptr);

	// --- VertexResourceを作る ---

	//　頂点リソースを作る
	vertexResource_ = dxCommon->CreateBufferResource(sizeof(vertexData) * 4);
	
	//　リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	
	//　リソースの先頭アドレスから使う
	vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(vertexData) * 4);
	
	//　1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(vertexData));

	// VertexResourceにデータを書き込むためのアドレスを取得してvertexData_に割り当てる
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	// 頂点データの設定
	
	// 左下
	vertexData_[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	vertexData_[0].texcoord = { 0.0f, 1.0f };
	vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };

	// 左上
	vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[1].texcoord = { 0.0f, 0.0f };
	vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };

	// 右下
	vertexData_[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	vertexData_[2].texcoord = { 1.0f, 1.0f };
	vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };

	// 右上
	vertexData_[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[3].texcoord = { 1.0f, 0.0f };
	vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

	// --- IndexResourceを作る ---
	
	//　インデックスリソースを作る
	indexResource_ = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);

	// リソースの先頭アドレスから使う
	indexBufferView.BufferLocation = indexResource_->GetGPUVirtualAddress();
	
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * 6);
	
	// インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// IndexResourceにデータを書き込むためのアドレスを取得してindexData_に割り当てる
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

	// インデックスの設定
	indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
	indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;
}

void Sprite::CreateMaterialResource()
{
}
