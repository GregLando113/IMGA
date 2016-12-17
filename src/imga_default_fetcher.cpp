
#include <Windows.h>
#include <d3d9.h>


HWND g__hWnd;
void* g__VEH;
BYTE g__restore;
BYTE* g__pEndScene;
IDirect3DDevice9* g__pDev = nullptr;


#define INT3 0xCC

static LONG WINAPI VEHHandler(PEXCEPTION_POINTERS exec)
{
	if (exec->ExceptionRecord->ExceptionAddress == g__pEndScene)
	{
		g__pDev = *(IDirect3DDevice9**)(exec->ContextRecord->Esp + 4);
		*g__pEndScene = g__restore;
		RemoveVectoredExceptionHandler(g__VEH);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}


IDirect3DDevice9* getd3d9device_default()
{
	WNDCLASSEXA wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = GetModuleHandleA(NULL);
	wc.lpszClassName = "d3dfetch";
	RegisterClassExA(&wc);
	g__hWnd = CreateWindowA("d3dfetch", 0, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), 0, wc.hInstance, 0);

	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D) return NULL;
	D3DPRESENT_PARAMETERS d3dPar = { 0 };
	d3dPar.Windowed = TRUE;
	d3dPar.SwapEffect = D3DSWAPEFFECT_DISCARD;
	LPDIRECT3DDEVICE9 pDev = NULL;
	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, g__hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPar, &pDev);
	if (!pDev) return NULL;

	g__pEndScene = ((BYTE***)pDev)[0][42];

	pDev->Release();
	pD3D->Release();

	g__VEH = AddVectoredExceptionHandler(1, VEHHandler);
	InterlockedExchange8((CHAR*)g__pEndScene, INT3);
	while (g__pDev == nullptr)
		Sleep(5);

	DestroyWindow(g__hWnd);
	UnregisterClassA("d3dfetch", wc.hInstance);

	return g__pDev;

} 