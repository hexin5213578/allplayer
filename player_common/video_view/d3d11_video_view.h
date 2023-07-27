#pragma once
#include "as_config.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)

#include "avs_video_view.h"
#include <Windows.h>
#include <windowsx.h>
#include <ShlObj.h>
#include <wrl.h>
#include <Xinput.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include "video_quad.h"

using Microsoft::WRL::ComPtr;
using std::unique_ptr;
using std::make_unique;

class D3D11VideoView : public AvsVideoView
{
public:
	D3D11VideoView(void* hwnd, ViewType type) : AvsVideoView(hwnd, type) {
	}
	virtual ~D3D11VideoView() = default;

	virtual int init() override;
	
	bool setTexturePara(int width, int height, Format fmt) override;

	bool draw(unsigned char* data[], int width, int height, int surfaceWidth, int surfaceHeight) override;

	void setMarginBlankRatio(int iTop, int iRight, int iBottom, int iLeft) override;

	void close() override;

	void clear() override;

private:
	ComPtr<IDXGISwapChain>			m_pSwapChain;
	ComPtr<ID3D11Device>			m_pd3dDevice;
	ComPtr<ID3D11DeviceContext>		m_pd3dDeviceContext;
	ComPtr<ID3D11RenderTargetView>  m_pRenderTargetView;
	unique_ptr<nv::VideoQuad>		m_vq;
	bool							m_bMarginChange = false;
};

#endif
