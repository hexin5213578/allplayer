#include "as_config.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "as_log.h"

#define D3D_LOG 0
#if D3D_LOG
#include "util.h"
#endif  

#include "avs_errno.h"
#include "d3d11_video_view.h"
#include "PixelShader2.h"
#include "dx_trace.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "xinput.lib")

#define DX_SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = NULL; } } 

int D3D11VideoView::init()
{
	if (!m_hWnd) {
		return D3D_ERROR_HWND_INVALID;
	}
	
	RECT clientRect;
	if (!GetClientRect((HWND)m_hWnd, &clientRect)) {
		return D3D_ERROR_HWND_INVALID;
	}
	
	ShowWindow((HWND)m_hWnd, SW_SHOW);
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;

	// D3D11
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	auto& bufferDesc = swapChainDesc.BufferDesc;
	bufferDesc.Width = clientWidth;
	bufferDesc.Height = clientHeight;
	bufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
	bufferDesc.RefreshRate.Numerator = 0;
	bufferDesc.RefreshRate.Denominator = 0;
	bufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = (HWND)m_hWnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	D3D_FEATURE_LEVEL level;
	UINT flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // DEBUG

	HRE(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, NULL, D3D11_SDK_VERSION, &swapChainDesc,
		&m_pSwapChain, &m_pd3dDevice, &level, &m_pd3dDeviceContext), D3D_ERROR_CREATE_DEVICE);

	m_surfaceWidth = clientWidth;
	m_surfaceHeight = clientHeight;

	// 重设交换链并且重新创建渲染目标视图
	ComPtr<ID3D11Texture2D> backBuffer;
	HRE(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf()), D3D_ERROR_SWAPCHAIN_GETBUFFER);
	CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM);
	HRE(m_pd3dDevice->CreateRenderTargetView(backBuffer.Get(), &renderTargetViewDesc, m_pRenderTargetView.GetAddressOf()), D3D_ERROR_CREATE_RENDERR);
	backBuffer.Reset();
	return D3D_ERROR_SUCCESS;
}

bool D3D11VideoView::setTexturePara(int width, int height, Format fmt)
{
	if (m_width != width || m_height != height || fmt != m_format) {
		DXGI_FORMAT dxgi_format;
		if (NV12 == fmt || D3D11 == fmt) {
			dxgi_format = DXGI_FORMAT_NV12;
		}
		else if (YUVJ420P == fmt || YUVJ420P == fmt) {
			dxgi_format = DXGI_FORMAT_R8_UNORM;
		}
		else {
			return false;
		}
		m_vq = make_unique<nv::VideoQuad>(m_pd3dDevice.Get(), m_pd3dDeviceContext.Get(), width, height, dxgi_format);
		//vq2 = make_unique<nv::VideoQuad>(d3ddeivce.Get(), d3ddeviceCtx.Get(), width, height, g_PS2, sizeof(g_PS2));
		if (!m_vq) {
			AS_LOG(AS_LOG_ERROR, "windos[%p] init d3d11 videoquad fail, memory out.", m_hWnd);
			return false;
		}
		HRESULT hr = m_vq->InitD3D11Resource();
		if (S_OK != hr) {
			m_vq->Destroy();
			AS_LOG(AS_LOG_ERROR, "windos[%p] init d3d11 resource fail, %ld.", m_hWnd, hr);
			return false;
		}
	}

	if (m_bMarginChange) {
		RECT clipRegion{ m_iLeft, m_iTop, m_iRight, m_iBottom };
		HRE(m_vq->setVertexTexCoordinate(clipRegion), 0);
		m_bMarginChange = false;
	}
	m_format = fmt;
	m_width = width;
	m_height = height;
	return true;
}

bool D3D11VideoView::draw(unsigned char* data[], int width, int height, int surfaceWidth, int surfaceHeight)
{
	#if D3D_LOG
		auto beforeDraw = getNowMs();
	#endif

	ID3D11Texture2D* t_frame = (ID3D11Texture2D*)data[0];
	int t_index = (int)data[1];

	ComPtr<ID3D11Device> device;
	t_frame->GetDevice(device.GetAddressOf());

	ComPtr<ID3D11DeviceContext> deviceCtx;
	device->GetImmediateContext(deviceCtx.GetAddressOf());

	RECT clientRect;
	if (!GetClientRect((HWND)m_hWnd, &clientRect)) {
		return false;
	}
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;
	if (clientWidth <= 0 || clientHeight <= 0) {
		return false;
	}

	ComPtr<ID3D11Texture2D> videoTextureShared;
	HRE(device->OpenSharedResource(m_vq->GetsharedHandle(), __uuidof(ID3D11Texture2D), (void**)videoTextureShared.GetAddressOf()), 0);

	deviceCtx->CopySubresourceRegion(videoTextureShared.Get(), 0, 0, 0, 0, t_frame, t_index, 0);
	deviceCtx->Flush();

	if ((clientWidth > 0 && clientHeight > 0) && (clientWidth != m_surfaceWidth || clientHeight != m_surfaceHeight)) {
		//ResizeSwapChain
		auto ppRenderTarget = m_pRenderTargetView.ReleaseAndGetAddressOf();

		DXGI_SWAP_CHAIN_DESC SwapChainDesc;
		HRE(m_pSwapChain->GetDesc(&SwapChainDesc), 0);
		if (clientWidth <= 0 || clientHeight <= 0)
			return false;
		
		HRE(m_pSwapChain->ResizeBuffers(SwapChainDesc.BufferCount, clientWidth, clientHeight, SwapChainDesc.BufferDesc.Format, SwapChainDesc.Flags), 0);

		// 重设交换链并且重新创建渲染目标视图
		ComPtr<ID3D11Texture2D> backBuffer;
		HRE(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf()), 0);
		CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, SwapChainDesc.BufferDesc.Format);
		HRE(m_pd3dDevice->CreateRenderTargetView(backBuffer.Get(), &renderTargetViewDesc, ppRenderTarget), 0);

		m_surfaceWidth = clientWidth;
		m_surfaceHeight = clientHeight;
	}

	D3D11_VIEWPORT viewPort = {};
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = m_surfaceWidth;
	viewPort.Height = m_surfaceHeight;
	viewPort.MaxDepth = 1;
	viewPort.MinDepth = 0;
	m_pd3dDeviceContext->RSSetViewports(1, &viewPort);
	m_pd3dDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);

	const FLOAT black[] = { 0, 0, 0, 1 };
	m_pd3dDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), black);

	#if D3D_LOG
		auto afterClear = getNowMs();
		AS_LOG(AS_LOG_INFO, "time spend for copy texture is %lld ", afterClear - beforeDraw);
	#endif

	m_vq->BeginDraw();
	m_vq->Draw();

	#if D3D_LOG
	auto afterDraw = getNowMs();
	AS_LOG(AS_LOG_INFO, "time spend for copy texture is %lld ", afterDraw - afterClear);
	#endif
	
	HRE(m_pSwapChain->Present(1, 0), 0);
	
	#if D3D_LOG
	auto afterPresent = getNowMs();
	AS_LOG(AS_LOG_INFO, "time spend for present is %lld ", afterPresent - afterDraw);
	#endif
	return true;
}

void D3D11VideoView::setMarginBlankRatio(int iTop, int iRight, int iBottom, int iLeft)
{
	if (m_iTop != iTop || m_iRight != iRight || m_iBottom != iBottom || m_iLeft != iLeft) {
		m_iTop = iTop;
		m_iRight = iRight;
		m_iBottom = iBottom;
		m_iLeft = iLeft;
		m_bMarginChange = true;
	}
}

void D3D11VideoView::close()
{
	clear();
	m_pRenderTargetView.Reset();
	m_pSwapChain.Reset();
	m_pd3dDeviceContext.Reset();
	if (m_vq) {
		m_vq->Destroy();
	}

#if defined(DEBUG) || defined(_DEBUG)
	ID3D11Debug* d3dDebug;
	HRESULT hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug));
	if (SUCCEEDED(hr)) 
		hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	
	if (d3dDebug != nullptr)			
		d3dDebug->Release();
#endif

	m_pd3dDevice.Reset();
	m_hWnd = nullptr;
}

void D3D11VideoView::clear()
{
	if (m_pd3dDeviceContext) {
		//m_pd3dDeviceContext->ClearRenderTargetView();
		m_pd3dDeviceContext->ClearState();
		m_pd3dDeviceContext->Flush();
	}
}

#endif
