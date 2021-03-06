// Win32Project1.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "Includes.h"
#include "OverlayClasses.h"

class D3DApp:public DXOverlay
{
public:
	D3DApp(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa);
	~D3DApp();
	void DrawScene(); 
};

D3DApp::D3DApp(String title,UINT width,UINT height,HINSTANCE inst,String target,UINT msaa):DXOverlay(title,width,height,inst,target,msaa)
{
	//just call base class constructor
}
D3DApp::~D3DApp()
{
	
}
void D3DApp::DrawScene()
{
	assert(m_pImmediateDeviceContext);
	assert(m_pSwapChain);

	float clearColor[4]={0.0f,0.0f,0.0f,0.0f};
	float blend[4]={0};
		
	m_pImmediateDeviceContext->ClearRenderTargetView(m_pRenderTargetView, reinterpret_cast<const float*>(&clearColor));
	m_pImmediateDeviceContext->OMSetBlendState( m_pAlphaOnBlendState, blend, 0xffffffff );
	
	DrawString(XMFLOAT2(m_width/2,m_height/2),1.0f,true,"Width:%d Height:%d",m_width,m_height);
	
	XMVECTOR pos1={m_width/2,m_height/2+1};
	XMVECTOR pos2={m_width/2,m_height/2-1};
	DrawLine(pos1,pos2,Colors::Red);

	DrawCircle(pos1,Colors::Green,30,30);
	m_pImmediateDeviceContext->OMSetBlendState( m_pAlphaOffBlendState, blend, 0xffffffff );
	HR(m_pSwapChain->Present(0, 0));
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	D3DApp app("Overlay",600,600,hInstance,"Untitled - Notepad",4);
	app.MakeWindow();
	app.InitializeDX();
	app.SetToTarget();
	return app.RunOverlay();
}



