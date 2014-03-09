#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }




#ifndef UNICODE  
typedef std::string String; 
#else
typedef std::wstring String; 
#endif

#ifdef _UNICODE
#define _T(x) x
#else 
#define _T(x)  L##x
#endif

#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x)                                              \
{                                                          \
	HRESULT hr = (x);                                      \
	if(FAILED(hr))                                         \
{                                                      \
	DXTraceW(__FILE__, (DWORD)__LINE__, hr, _T(#x), true); \
}                                                      \
}
#endif
#else
#ifndef HR
#define HR(x) (x)
#endif
#endif 



class DXOverlay
{
public:
	DXOverlay(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa);
	bool MakeWindow();
	int RunOverlay();
	void SetToTarget();
	virtual ~DXOverlay();
	bool InitializeDX();
	virtual void OnResize();
	virtual void DrawScene()=0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	String WindowTitle;
	UINT m_width;
	UINT m_height;
	HINSTANCE m_appInst;
	HWND m_MainWndHandle;
	String m_targetWindowName;
	HWND m_targetApplication;
	bool m_Paused;
	bool m_Minimized;
	bool m_Resizing;
	UINT m_MSAAQualtiy;

	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pImmediateDeviceContext;
	ID3D11RenderTargetView* m_pRenderTargetView;
	IDXGISwapChain* m_pSwapChain;
	D3D_DRIVER_TYPE m_DriverType;
	D3D11_VIEWPORT m_Viewport;
};
namespace
{
	DXOverlay* DXInst=0;
}
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return DXInst->MsgProc(hwnd, msg, wParam, lParam);
}
bool DXOverlay::MakeWindow()
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= m_appInst;
	wcex.hIcon			= LoadIcon(0, IDI_APPLICATION);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
	wcex.lpszMenuName	= MAKEINTRESOURCE(109);
	wcex.lpszClassName	= WindowTitle.c_str();
	wcex.hIconSm		= LoadIcon(0, IDI_APPLICATION);

	
	if( !RegisterClassEx(&wcex) )
	{
		return false;
	}
//
	m_MainWndHandle = CreateWindowEx(WS_EX_TOPMOST| WS_EX_LAYERED  , WindowTitle.c_str(), WindowTitle.c_str(),  WS_POPUP, 1, 1, m_width, m_height, 0, 0, 0, 0);
	
	SetLayeredWindowAttributes(m_MainWndHandle, 0, 0.0f, LWA_ALPHA);
	SetLayeredWindowAttributes(m_MainWndHandle, 0, RGB(0, 0, 0), LWA_COLORKEY);
	ShowWindow( m_MainWndHandle, SW_SHOW);
	
	return true;
}
bool DXOverlay::InitializeDX()
{
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	m_DriverType=D3D_DRIVER_TYPE_HARDWARE;
	HRESULT hr = D3D11CreateDevice(
		0,                 // default adapter
		m_DriverType,
		0,                 // no software device
		createDeviceFlags, 
		0, 0,              // default feature level array
		D3D11_SDK_VERSION,
		&m_pDevice,
		&featureLevel,
		&m_pImmediateDeviceContext);

	if(FAILED(hr))
		return false;

	if(featureLevel!=D3D_FEATURE_LEVEL_11_0)
		return false;

	HR(m_pDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM,4,&m_MSAAQualtiy));

	assert(m_MSAAQualtiy>0);

	DXGI_SWAP_CHAIN_DESC swapdesc;
	swapdesc.BufferDesc.Width  = m_width;
	swapdesc.BufferDesc.Height = m_height;
	swapdesc.BufferDesc.RefreshRate.Numerator = 75;
	swapdesc.BufferDesc.RefreshRate.Denominator = 1;
	swapdesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapdesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapdesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	swapdesc.SampleDesc.Count=4;
	swapdesc.SampleDesc.Quality=m_MSAAQualtiy-1;

	swapdesc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapdesc.BufferCount  = 1;
	swapdesc.OutputWindow = m_MainWndHandle;
	swapdesc.Windowed     = true;
	swapdesc.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	swapdesc.Flags        = 0;

	IDXGIDevice* dxgiDevice = 0;
	HR(m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = 0;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	HR(dxgiFactory->CreateSwapChain(m_pDevice, &swapdesc, &m_pSwapChain));

	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);
	OnResize();
}
void DXOverlay::OnResize()
{
	assert(m_pImmediateDeviceContext);
	assert(m_pDevice);
	assert(m_pSwapChain);

	ReleaseCOM(m_pRenderTargetView);
	
	HR(m_pSwapChain->ResizeBuffers(1, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* backBuffer;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(m_pDevice->CreateRenderTargetView(backBuffer, 0, &m_pRenderTargetView));
	ReleaseCOM(backBuffer);

	m_pImmediateDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView,NULL);

	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width    = static_cast<float>(m_width/2);
	m_Viewport.Height   = static_cast<float>(m_height/2);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_pImmediateDeviceContext->RSSetViewports(1, &m_Viewport);
	
}
int DXOverlay::RunOverlay()
{
	MSG msg = {0};
	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}else{	
			SetToTarget();
			DrawScene();
		}
	}
	return (int)msg.wParam;
}
void DXOverlay::SetToTarget()
{
	m_targetApplication = FindWindow(0, m_targetWindowName.c_str());
	if (m_targetApplication)
	{
		RECT tClient,tSize;
		GetClientRect(m_targetApplication,&tClient);
		GetWindowRect(m_targetApplication, &tSize);

		DWORD dwStyle = GetWindowLong(m_targetApplication, GWL_STYLE);
		int windowheight;
		int windowwidth;
		int clientheight;
		int clientwidth;
		int borderheight;
		int borderwidth;
		if(dwStyle & WS_BORDER)
		{	
			windowwidth=tSize.right-tSize.left;
			windowheight=tSize.bottom-tSize.top;

			clientwidth=tClient.right-tClient.left;
			clientheight=tClient.bottom-tClient.top;

			borderheight=(windowheight-tClient.bottom);
			borderwidth=(windowwidth-tClient.right)/2; //only want one side
			borderheight-=borderwidth; //remove bottom

			tSize.left+=borderwidth;
			tSize.top+=borderheight;
		}
		m_width=clientwidth;
		m_height=clientheight;
		MoveWindow(m_MainWndHandle, tSize.left, tSize.top,clientwidth , clientheight, true);
	}
}
LRESULT DXOverlay::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	switch( msg )
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		return 0;  
	case WM_SIZE:
		// Save the new client area dimensions.
		
		if( m_pDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				m_Minimized = true;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				m_Minimized = false;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				if( m_Minimized )
				{
					m_Minimized = false;
					OnResize();
				}else if( m_Resizing ){
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_Resizing  = true;
		return 0;

	case WM_EXITSIZEMOVE:
		m_Resizing  = false;
		OnResize();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
DXOverlay::DXOverlay(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa)
{
	WindowTitle=title;
	m_width=width;
	m_height=height;
	m_appInst=inst;
	DXInst=this;//for msgproc
	m_Paused=false;
	m_Minimized=false;
	m_Resizing=false;
	m_MSAAQualtiy=msaa;
	m_targetWindowName=target;
	ZeroMemory(&m_Viewport, sizeof(D3D11_VIEWPORT));
}
DXOverlay::~DXOverlay()
{
	ReleaseCOM(m_pRenderTargetView);
	ReleaseCOM(m_pDevice);
	if(m_pImmediateDeviceContext)
		m_pImmediateDeviceContext->ClearState();
	ReleaseCOM(m_pImmediateDeviceContext);
	ReleaseCOM(m_pSwapChain);
}