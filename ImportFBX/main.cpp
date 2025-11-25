// Loading an .fbx mesh in DirectX 9
// Example released by Bobby Thurman
// Most of this code is stolen from various places.
// Thanks to Doug Rogers and Ken Wright

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define D3D_DEBUG_INFO

#include <windows.h>
#include <assert.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h> // sprintf(..)

#include "SingleFbxMesh.h"
#include "fbxSdk.h"

HWND                    g_hWnd = NULL;
LPDIRECT3D9             g_pD3D = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;
SingleFbxMesh        g_SingleFbxMesh;
static bool				g_Wire = false;
static float			g_fSpinX = -25.0f;
static float			g_fSpinY = 0.0f;
static float         g_scale = 0.018f;

void init(void);         // - called at the start of our program
void render(void);       // - our mainloop..called over and over again
void shutDown(void);     // - last function we call before we end!

LRESULT WINAPI MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static POINT ptLastMousePosit;
	static POINT ptCurrentMousePosit;
	static bool bMousing;

	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_ESCAPE:
			shutDown();
			PostQuitMessage(0);
			break;

		case 'W':
			g_Wire = !g_Wire;
			break;
		}

		break;
	}

	case WM_LBUTTONDOWN:
	{
		ptLastMousePosit.x = ptCurrentMousePosit.x = LOWORD(lParam);
		ptLastMousePosit.y = ptCurrentMousePosit.y = HIWORD(lParam);
		bMousing = true;

		break;
	}

	case WM_LBUTTONUP:
	{
		bMousing = false;

		break;
	}

	case WM_MOUSEMOVE:
	{
		ptCurrentMousePosit.x = LOWORD(lParam);
		ptCurrentMousePosit.y = HIWORD(lParam);

		if (bMousing)
		{
			g_fSpinX -= (ptCurrentMousePosit.x - ptLastMousePosit.x);
			g_fSpinY -= (ptCurrentMousePosit.y - ptLastMousePosit.y);
		}

		ptLastMousePosit.x = ptCurrentMousePosit.x;
		ptLastMousePosit.y = ptCurrentMousePosit.y;

		break;
	}
	case WM_MOUSEWHEEL:
	{
		short wheelMovement = -((short)HIWORD(wParam)) / WHEEL_DELTA;

		if (wheelMovement > 0)
		{
			g_scale = max(g_scale * 0.5f, 0.001f);
		}
		else
		{
			g_scale = min(g_scale * 1.5f, 10.0f);
		}

		break;
	}
	}

	if (uMsg == WM_DESTROY)
	{
		shutDown();
		PostQuitMessage(0);
		return 0;
	}

	return (long)DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int _stdcall WinMain(HINSTANCE i, HINSTANCE, char* k, int)
{
	MSG msg;
	WCHAR szname[] = L"DirectX3D";
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
					  GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					  szname, NULL };
	RegisterClassEx(&wc);
	g_hWnd = CreateWindowEx(WS_EX_APPWINDOW,
		szname, L"Display FBX in DX9",
		WS_OVERLAPPEDWINDOW,//for fullscreen make into WS_POPUP
		50, 50, 640, 480,    //for full screen GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	init();

	ShowWindow(g_hWnd, SW_SHOW);
	UpdateWindow(g_hWnd);

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
				break;

			DispatchMessage(&msg);
		}
		else
		{
			// This is where we advance the animation time and build the bone matrices.
			g_SingleFbxMesh.advanceTime();

			render();
		}
	}
	return 0;
}

void init(void)
{
	D3DDISPLAYMODE d3ddm;

	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	D3DPOOL_DEFAULT;

	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice);

	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// Initialise our Texture and Mesh Classes
	g_SingleFbxMesh.Init();
	g_SingleFbxMesh.load(g_pd3dDevice, "scorpid.fbx", "scorp.dds", 50);
}

void shutDown(void)
{
	g_SingleFbxMesh.release();

	g_pd3dDevice->Release();
	g_pd3dDevice = NULL;

	g_pD3D->Release();
	g_pD3D = NULL;
}

void render()
{
	D3DXMATRIX worldViewProj;

	{
		D3DXMATRIX matScale;
		D3DXMATRIX matTrans;
		D3DXMATRIX matRot;
		D3DXMATRIX matProj;

		D3DXMatrixScaling(&matScale, g_scale, g_scale, g_scale);
		D3DXMatrixTranslation(&matTrans, 0.0f, -5.0f, 20.0f);
		D3DXMatrixRotationYawPitchRoll(&matRot, D3DXToRadian(g_fSpinX),
			D3DXToRadian(g_fSpinY),
			0.0f);

		D3DXMatrixPerspectiveFovLH(&matProj, D3DXToRadian(45.0f),
			640.0f / 480.0f, 0.1f, 500.0f);

		worldViewProj = matScale * matRot * matTrans * matProj;
	}

	g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, g_Wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
	g_pd3dDevice->BeginScene();

	g_SingleFbxMesh.render(worldViewProj);

	g_pd3dDevice->EndScene();
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}
