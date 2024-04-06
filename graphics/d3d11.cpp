#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <d3dcommon.h>

#define ASSERT_HR(hr) ASSERT(SUCCEEDED(hr))

struct D3D11Texture {
	ID3D11ShaderResourceView* resource;
	ID3D11SamplerState* sampler;
};

ID3D11ShaderResourceView* GenerateTexture(ID3D11Device1* device, Image image) {
	DXGI_FORMAT formats[4] = {
		DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8G8_UNORM, (DXGI_FORMAT)0 /*not supported*/, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
	};

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width             = image.width;
	desc.Height            = image.height;
	desc.MipLevels         = 1;
	desc.ArraySize         = 1;
	desc.Format            = formats[image.channels - 1];
	desc.SampleDesc.Count  = 1;
	desc.Usage             = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags         = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA subResource = {};
	subResource.pSysMem     = image.data;
	subResource.SysMemPitch = image.width * image.channels;

	ID3D11Texture2D* temp;
	HRESULT hr = device->CreateTexture2D(&desc, &subResource, &temp);
	ASSERT_HR(hr);
	ID3D11ShaderResourceView* resource;
	hr = device->CreateShaderResourceView(temp, NULL, &resource);
	ASSERT_HR(hr);
	return resource;
}