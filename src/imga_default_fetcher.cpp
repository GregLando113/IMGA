
#include <Windows.h>
#include <d3d9.h>


HWND g__hWnd;
void* g__VEH;
BYTE g__restore;
BYTE* g__pEndScene;
IDirect3DDevice9* g__pDev = nullptr;
DWORD g__oldprot;


#define INT3 0xCC


static HRESULT WINAPI dx_endscene(IDirect3DDevice9* dev)
{
	g__pDev = dev;
	return ((HRESULT(WINAPI*)(IDirect3DDevice9*))g__pEndScene)(dev);
}

static LONG WINAPI VEHHandler(PEXCEPTION_POINTERS exec)
{
	if (exec->ExceptionRecord->ExceptionCode != STATUS_ACCESS_VIOLATION && exec->ExceptionRecord->ExceptionCode != STATUS_SINGLE_STEP)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	if (exec->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	//if (exec->ContextRecord->Eip == (DWORD)g__pEndScene)
	//{
		exec->ContextRecord->Eip = (DWORD)dx_endscene;
		VirtualProtect(g__pEndScene, 1, g__oldprot, &g__oldprot);
		exec->ContextRecord->EFlags |= 0x100;
		return EXCEPTION_CONTINUE_EXECUTION;
	//}
	//return EXCEPTION_CONTINUE_SEARCH;
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
	if (pD3D == nullptr) return NULL;
	D3DPRESENT_PARAMETERS d3dPar = { 0 };
	d3dPar.Windowed = TRUE;
	d3dPar.SwapEffect = D3DSWAPEFFECT_DISCARD;
	LPDIRECT3DDEVICE9 pDev = NULL;
	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, g__hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPar, &pDev);
	if (pDev == nullptr) return NULL;

	g__pEndScene = ((BYTE***)pDev)[0][42];

	pDev->Release();
	pD3D->Release();

	g__VEH = AddVectoredExceptionHandler(1, VEHHandler);
	VirtualProtect(g__pEndScene, 1, PAGE_NOACCESS, &g__oldprot);

	while (g__pDev == nullptr)
		Sleep(1);
	RemoveVectoredExceptionHandler(g__VEH);
	DestroyWindow(g__hWnd);
	UnregisterClassA("d3dfetch", wc.hInstance);

	return g__pDev;

} 