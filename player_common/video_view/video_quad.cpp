#include "video_quad.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)

#include <vector>
#include "PixelShader.h"
#include "PixelShader2.h"
#include "VertexShader.h"
#include "dx_trace.h"
#include "as_log.h"

using namespace nv;
namespace dx = DirectX;

//ComPtr<ID3D11Texture2D> VideoQuad::videoTexture;
//HANDLE VideoQuad::sharedHandle;

VideoQuad::VideoQuad(
	ID3D11Device* device,
	ID3D11DeviceContext* deviceCtx,
	int videoWidth,
	int videoHeight,
	DXGI_FORMAT format,
	const BYTE* pixelShader, size_t pixelShaderSize)
	: _device(device), _deviceCtx(deviceCtx), psConstant({})
{
	m_format = format;
	m_height = videoHeight;
	m_width = videoWidth;
	m_pixelShader = pixelShader;
	m_shaderSize = pixelShaderSize;
}

HRESULT nv::VideoQuad::InitD3D11Resource()
{
	HRESULT hr = S_OK;
	if (videoTexture == nullptr) {
		D3D11_TEXTURE2D_DESC tdesc = {};
		tdesc.Format = m_format;
		tdesc.Usage = D3D11_USAGE_DEFAULT;
		tdesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
		tdesc.ArraySize = 1;
		tdesc.MipLevels = 1;
		tdesc.SampleDesc.Count = 1;
		tdesc.Height = m_height;
		tdesc.Width = m_width;
		tdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		HR(_device->CreateTexture2D(&tdesc, nullptr, videoTexture.GetAddressOf()));
		if (!videoTexture) {
			return -1;
		}

		ComPtr<IDXGIResource> dxgiShareTexture;
		HR(videoTexture->QueryInterface(__uuidof(IDXGIResource), (void**)dxgiShareTexture.GetAddressOf()));
		HR(dxgiShareTexture->GetSharedHandle(&sharedHandle));
	}

	UINT bind_flags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_RENDER_TARGET_VIEW_DESC render_target_videw_desc;
	D3D11_SHADER_RESOURCE_VIEW_DESC resource_view_desc;

	if (bind_flags & D3D11_BIND_RENDER_TARGET) {
		memset(&render_target_videw_desc, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
		memset(&render_target_videw_desc, 0, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
		render_target_videw_desc.Format = m_format;
		render_target_videw_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_videw_desc.Texture2D.MipSlice = 0;

		HR(_device->CreateRenderTargetView(videoTexture.Get(), &render_target_videw_desc, &render_target_view_));
	}

	if (m_format == DXGI_FORMAT_NV12)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC const luminancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
			videoTexture.Get(),
			D3D11_SRV_DIMENSION_TEXTURE2D,
			DXGI_FORMAT_R8_UNORM
		);

		_device->CreateShaderResourceView(
			videoTexture.Get(),
			&luminancePlaneDesc,
			&m_luminanceView
		);

		D3D11_SHADER_RESOURCE_VIEW_DESC const chrominancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
			videoTexture.Get(),
			D3D11_SRV_DIMENSION_TEXTURE2D,
			DXGI_FORMAT_R8G8_UNORM
		);

		_device->CreateShaderResourceView(
			videoTexture.Get(),
			&chrominancePlaneDesc,
			&m_chrominanceView
		);
	}
	else
	{
		memset(&resource_view_desc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		resource_view_desc.Format = m_format;
		resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resource_view_desc.Texture2D.MostDetailedMip = 0;
		resource_view_desc.Texture2D.MipLevels = 1;

		hr = _device->CreateShaderResourceView(videoTexture.Get(), &resource_view_desc, &texture_view_);
		if (FAILED(hr)) {
			return hr;
		}
	}

	HR(InitVertexShader());
	HR(InitPixelShader(m_pixelShader, m_shaderSize));

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER::D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	HR(_device->CreateSamplerState(&samplerDesc, pSampler.GetAddressOf()));
	transformMatrix = dx::XMMatrixRotationX(0);
	return hr;
}

HRESULT nv::VideoQuad::InitVertexShader()
{
	if (!_device) {
		return -1;
	}

	HRESULT hr = S_OK;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = sizeof(vertices);
	bd.StructureByteStride = sizeof(Vertex);
	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;
	HR(_device->CreateBuffer(&bd, &sd, pVertexBuffer.GetAddressOf()));

	const UINT16 indices[] = {
		0,1,2, 0,2,3
	};
	indicesSize = std::size(indices);

	D3D11_BUFFER_DESC ibd = {};
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.ByteWidth = sizeof(indices);
	ibd.StructureByteStride = sizeof(UINT16);
	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices;
	HR(_device->CreateBuffer(&ibd, &isd, &pIndexBuffer));

	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.ByteWidth = sizeof(constant);
	cbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &constant;
	HR(_device->CreateBuffer(&cbd, &csd, &pConstantBuffer));

	D3D11_INPUT_ELEMENT_DESC ied[] = {
		{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TexCoord", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	HR(_device->CreateInputLayout(ied, std::size(ied), g_main, sizeof(g_main), &pInputLayout));
	HR(_device->CreateVertexShader(g_main, sizeof(g_main), nullptr, &pVertexShader));
	return hr;
}

HRESULT nv::VideoQuad::InitPixelShader(const BYTE* pixel_shader, size_t pixel_shader_size)
{
	if (pixel_shader) {
		// 这是有特殊效果的着色器
		HR(_device->CreatePixelShader(pixel_shader, pixel_shader_size, nullptr, &pPixelShader));

		D3D11_BUFFER_DESC cbd = {};
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbd.ByteWidth = sizeof(psConstant);
		cbd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA csd = {};
		psConstant.MosaicSetting.PixelSize = 128;
		psConstant.BSmoothSetting.iteration = 4;
		psConstant.BSmoothSetting.pixelSize[0] = 10;
		psConstant.SharpenSetting.offset = 2;
		csd.pSysMem = &psConstant;
		HR(_device->CreateBuffer(&cbd, &csd, &PSConstantBuffer));
	}
	else {
		// 这是默认没有任何效果的着色器
		HR(_device->CreatePixelShader(g_PS, sizeof(g_PS), nullptr, &pPixelShader));
	}
	return S_OK;
}

HRESULT nv::VideoQuad::setVertexTexCoordinate(RECT texRect)
{
	if (pVertexBuffer) {
		pVertexBuffer.Reset();
	}
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = sizeof(vertices);
	bd.StructureByteStride = sizeof(Vertex);
	D3D11_SUBRESOURCE_DATA sd = {};

	vertices[0].tex.u = (float)texRect.left / 100.f;
	vertices[0].tex.v = (float)texRect.top / 100.f;
	vertices[1].tex.u = (float)texRect.right / 100.f;
	vertices[1].tex.v = (float)texRect.top / 100.f;
	vertices[2].tex.u = (float)texRect.right / 100.f;
	vertices[2].tex.v = (float)texRect.bottom / 100.f;
	vertices[3].tex.u = (float)texRect.left / 100.f;
	vertices[3].tex.v = (float)texRect.bottom / 100.f;

	sd.pSysMem = vertices;
	HR(_device->CreateBuffer(&bd, &sd, pVertexBuffer.GetAddressOf()));
	return S_OK;
}

void nv::VideoQuad::MulTransformMatrix(const DirectX::XMMATRIX& matrix)
{
	transformMatrix *= matrix;
}

void VideoQuad::UpdateByRatio(double srcRatio, double dstRatio) {
	if (srcRatio > dstRatio) {
		MulTransformMatrix(dx::XMMatrixScaling(1, dstRatio / srcRatio, 1));
	}
	else if (srcRatio < dstRatio) {
		MulTransformMatrix(dx::XMMatrixScaling(srcRatio / dstRatio, 1, 1));
	}
	else {
		MulTransformMatrix(dx::XMMatrixScaling(1, 1, 1));
	}
}

void nv::VideoQuad::BeginDraw()
{
	transformMatrix = dx::XMMatrixRotationX(0);
}

HANDLE nv::VideoQuad::GetsharedHandle()
{
	return sharedHandle;
}

void VideoQuad::Draw() {

	D3D11_MAPPED_SUBRESOURCE map;
	_deviceCtx->Map(pConstantBuffer.Get(), 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &map);

	auto m = dx::XMMatrixTranspose(transformMatrix);
	memcpy(map.pData, &m, sizeof(m));

	_deviceCtx->Unmap(pConstantBuffer.Get(), 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0u;
	_deviceCtx->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
	_deviceCtx->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	_deviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceCtx->IASetInputLayout(pInputLayout.Get());

	_deviceCtx->VSSetShader(pVertexShader.Get(), 0, 0);
	_deviceCtx->VSSetConstantBuffers(0, 1, pConstantBuffer.GetAddressOf());

	_deviceCtx->PSSetShader(pPixelShader.Get(), 0, 0);
	_deviceCtx->PSSetShaderResources(0, 1, m_luminanceView.GetAddressOf());
	_deviceCtx->PSSetShaderResources(1, 1, m_chrominanceView.GetAddressOf());
	_deviceCtx->PSSetSamplers(0, 1, pSampler.GetAddressOf());
	if (PSConstantBuffer != nullptr) {
		D3D11_MAPPED_SUBRESOURCE map;
		_deviceCtx->Map(PSConstantBuffer.Get(), 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &map);
		memcpy(map.pData, &psConstant, sizeof(psConstant));
		_deviceCtx->Unmap(PSConstantBuffer.Get(), 0);
		_deviceCtx->PSSetConstantBuffers(0, 1, PSConstantBuffer.GetAddressOf());
	}

	_deviceCtx->DrawIndexed(indicesSize, 0, 0);
}

void nv::VideoQuad::SetMosaicSize(int mosaicSize)
{
	psConstant.MosaicSetting.PixelSize = mosaicSize;
}

void nv::VideoQuad::Destroy()
{
	pVertexBuffer.Reset();
	pIndexBuffer.Reset();
	pConstantBuffer.Reset();
	pInputLayout.Reset();
	pVertexShader.Reset();
	pPixelShader.Reset();
	pSampler.Reset();
	PSConstantBuffer.Reset();

	videoTexture.Reset();
	m_luminanceView.Reset();
	m_chrominanceView.Reset();
}

#endif
