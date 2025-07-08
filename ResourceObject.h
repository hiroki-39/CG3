#pragma once  
#include "externals/DirectXTex/d3dx12.h"  

class ResourceObject
{
public:

	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	/// <param name="resource">�|�C���g�����炤</param>
	ResourceObject(ID3D12Resource* resource) : resource_(resource) {}

	/// <summary>
	/// �f�X�g���N�^
	/// </summary>
	~ResourceObject()
	{
		if (resource_)
		{
			resource_->Release();
		}
	}

	ID3D12Resource* get() { return resource_; }

private:

	ID3D12Resource* resource_;
};
