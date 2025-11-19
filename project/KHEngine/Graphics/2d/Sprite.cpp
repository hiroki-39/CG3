#include "Sprite.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"

void Sprite::Initialize(SpriteCommon* spriteCommon, uint32_t textureIndex)
{
	// 引数で受け取ってメンバ変数に記録する
	this->spriteCommon_ = spriteCommon;

	// dxCommon を確実に初期化（NULLチェック含む）
	assert(spriteCommon_ != nullptr);

	this->dxCommon = spriteCommon->GetDirectXCommon();

	assert(this->dxCommon != nullptr);

	// 受け取ったテクスチャインデックスを保存する
	this->textureIndex = textureIndex;

	// 頂点バッファ・インデックスバッファの作成
	CreateBufferResource();
	CreateMaterialResource();
	CreateTransformationMatrixResource();

	AdjustTextureSize();
}

void Sprite::Update()
{
	// 頂点リソースにデータを書き込む(4点分)

	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	//左右反転
	if (isFlipX_)
	{
		left = -left;
		right = -right;
	}

	//上下反転
	if (isFlipY_)
	{
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata =
		TextureManager::GetInstance()->GetMetaData(textureIndex);

	float tex_left = textureLeftTop.x / static_cast<float>(metadata.width);
	float tex_right = (textureLeftTop.x + textureSize.x) / static_cast<float>(metadata.width);
	float tex_top = textureLeftTop.y / static_cast<float>(metadata.height);
	float tex_bottom = (textureLeftTop.y + textureSize.y) / static_cast<float>(metadata.height);
	
	
	// 左下
	vertexData_[0].position = { left, bottom, 0.0f, 1.0f };
	vertexData_[0].texcoord = { tex_left, tex_bottom };
	vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };

	// 左上
	vertexData_[1].position = { left, top, 0.0f, 1.0f };
	vertexData_[1].texcoord = { tex_left, tex_top };
	vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };

	// 右下
	vertexData_[2].position = { right, bottom, 0.0f, 1.0f };
	vertexData_[2].texcoord = { tex_right, tex_bottom };
	vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };

	// 右上
	vertexData_[3].position = { right, top, 0.0f, 1.0f };
	vertexData_[3].texcoord = { tex_right, tex_top };
	vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };

	// インデックスリソースにデータを書き込む(6点分)
	indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
	indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;

	//　Transform情報を作る

	Transform transform;
	transform.translate = { position.x,position.y,0.0f };
	transform.rotation = { 0.0f, 0.0f, rotation };
	transform.scale = { size.x, size.y, 1.0f };

	// TransformからWorldMatrixを作る
	Matrix4x4 worldMatrix = transform.GetWorldMatrix();

	// ViewMatrixを作って単位行列を代入
	Matrix4x4 viewMatrix = Matrix4x4::Identity();

	// ProjectionMatrixを作って平行投影行列を書き込む
	Matrix4x4 projectionMatrix = Matrix4x4::Orthographic(0.0f, 0.0f, float(winApp_->kClientWidth), float(winApp_->kClientHeight), 0.0f, 100.0f);


	transformationMatrixData_->WVP = Matrix4x4::Multiply(worldMatrix, Matrix4x4::Multiply(viewMatrix, projectionMatrix));
	transformationMatrixData_->World = worldMatrix;
}

void Sprite::Draw()
{
	// VertexBufferViewを設定
	dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	// IndexBufferViewを設定
	dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	// MaterialCBufferの場所の設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	// TransformationMatrixCBufferの場所の設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

	// SRVのDescriptorTableの先頭の設定
	dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));

	// 描画
	dxCommon->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::SetTexture(uint32_t textureIndex)
{
	this->textureIndex = textureIndex;
}

void Sprite::CreateBufferResource()
{
	assert(spriteCommon_ != nullptr);

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
}

void Sprite::CreateMaterialResource()
{
	// --- マテリアルリソースを作る ---

	//Sprite用のリソースを作る
	materialResource_ = dxCommon->CreateBufferResource(sizeof(Material));

	//書き込む為のアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	//色の設定
	materialData_->color = { 1.0f,1.0f,1.0f,1.0f };

	//Lightingを有効化
	materialData_->enableLighting = false;

	////Lightingの種類の設定
	//materialDataSprite->selectLightings = 0;

	//単位行列を書き込む
	materialData_->uvTransform = Matrix4x4::Identity();

}


void Sprite::CreateTransformationMatrixResource()
{
	//Sprite用のTransformationMatrix用のリソースを作る
	transformationMatrixResource_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));

	//書き込むためのアドレスを取得
	transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	//単位行列を書き込んでおく
	transformationMatrixData_->WVP = Matrix4x4::Identity();

	transformationMatrixData_->World = Matrix4x4::Identity();
}

void Sprite::AdjustTextureSize()
{
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata =
		TextureManager::GetInstance()->GetMetaData(textureIndex);

	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);

	// 画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}
