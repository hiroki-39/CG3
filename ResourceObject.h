#pragma once  
#include "externals/DirectXTex/d3dx12.h"  

class ResourceObject
{
public:

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="resource">ポイントをもらう</param>
	ResourceObject(ID3D12Resource* resource) : resource_(resource) {}

	/// <summary>
	/// デストラクタ
	/// </summary>
	~ResourceObject()
	{
		if (resource_)
		{
			resource_->Release();
		}
	}

	ID3D12Resource* Get() { return resource_; }

private:

	ID3D12Resource* resource_;
};
