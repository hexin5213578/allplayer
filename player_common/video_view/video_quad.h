#pragma once

#include "as_config.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)

#include <memory>
#include <vector>

#include <Windows.h>
#include <ShlObj.h>
#include <wrl.h>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <dxgi1_2.h>
#pragma comment(lib, "dxgi.lib")
#include <DirectXMath.h>

namespace nv {
	using namespace Microsoft::WRL;

	struct Vertex {
		float x; 
		float y; 
		float z;
		struct {
			float u;
			float v;
		} tex;
	};

	class VideoQuad 
	{
	public:
		__declspec(align(16)) struct 
		{
			int whatEnable;

			__declspec(align(16)) struct {
				int PixelSize;
			} MosaicSetting;
			
			__declspec(align(16)) struct {
				float pixelSize[2];
				float iteration;
				bool whiteLine;
			} BSmoothSetting;

			__declspec(align(16)) struct {
				int mode;
			} SobelEdgeSetting;
			
			__declspec(align(16)) struct {
				float offset;
			} SharpenSetting;
		} psConstant;

		VideoQuad(
			ID3D11Device* device,
			ID3D11DeviceContext* deviceCtx,
			int videoHeight,
			int videoWidth,
			DXGI_FORMAT format,
			const BYTE* pixelShader = nullptr, 
			size_t pixelShaderSize = 0);

		HRESULT InitD3D11Resource();
		HRESULT InitVertexShader();
		HRESULT InitPixelShader(const BYTE* pixel_shader = NULL, size_t pixel_shader_size = 0);
		HRESULT setVertexTexCoordinate(RECT texRect);
		void MulTransformMatrix(const DirectX::XMMATRIX& matrix);
		void UpdateByRatio(double srcRatio, double dstRatio);
		void BeginDraw();
		HANDLE GetsharedHandle();
		void Draw();
		void SetMosaicSize(int mosaicSize);
		void Destroy();
	
	private:

		ID3D11Device* _device;
		ID3D11DeviceContext* _deviceCtx;
		ComPtr<ID3D11Texture2D> videoTexture;
		HANDLE sharedHandle;
		ComPtr<ID3D11ShaderResourceView> m_luminanceView;
		ComPtr<ID3D11ShaderResourceView> m_chrominanceView;
		ComPtr<ID3D11Buffer> pVertexBuffer;
		ComPtr<ID3D11Buffer> pIndexBuffer;
		ComPtr<ID3D11Buffer> pConstantBuffer;
		ComPtr<ID3D11InputLayout> pInputLayout;
		ComPtr<ID3D11VertexShader> pVertexShader;
		ComPtr<ID3D11PixelShader> pPixelShader;
		ComPtr<ID3D11SamplerState> pSampler;
		ComPtr<ID3D11Buffer> PSConstantBuffer;

		ID3D11RenderTargetView* render_target_view_ = NULL;
		ID3D11ShaderResourceView* texture_view_ = NULL;

		DXGI_FORMAT m_format;
		int m_height;
		int m_width;
		const BYTE* m_pixelShader;
		size_t m_shaderSize;

		size_t indicesSize;
		DirectX::XMMATRIX transformMatrix;

		Vertex vertices[4] = {
			{-1,		1,		0,		0, 0},
			{1,			1,		0,		1, 0},
			{1,			-1,		0,		1, 1},
			{-1,		-1,		0,		0, 1},
		};

		DirectX::XMMATRIX constant = DirectX::XMMatrixRotationZ(0);
	};
}

#endif