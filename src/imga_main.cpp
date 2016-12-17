#include "..\imga.h"

#include "..\imgui\imgui_impl_dx9.h"

typedef HRESULT(WINAPI *d3d9Present_t)(IDirect3DDevice9* dev, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion);
typedef HRESULT(WINAPI *d3d9Reset_t)(IDirect3DDevice9* dev, D3DPRESENT_PARAMETERS* params);

extern IDirect3DDevice9* getd3d9device_default();

imga::Context* g__ctx = nullptr;
imga::Context* imga::get_ctx() { return g__ctx; }

void call_onframe(IDirect3DDevice9* dev) {
	if (g__ctx->modules.empty())
		return;

	for (int i = 0; i < g__ctx->modules.size(); ++i)
		g__ctx->modules[i]->OnFrame(dev);
}
void call_onprereset(IDirect3DDevice9* dev, D3DPRESENT_PARAMETERS* params) {
	if (g__ctx->modules.empty())
		return;

	for (int i = 0; i < g__ctx->modules.size(); ++i)
		g__ctx->modules[i]->OnPreReset(dev);
}
void call_onpostreset(IDirect3DDevice9* dev, D3DPRESENT_PARAMETERS* params) {
	if (g__ctx->modules.empty())
		return;

	for (int i = 0; i < g__ctx->modules.size(); ++i)
		g__ctx->modules[i]->OnPostReset(dev);
}

static HRESULT WINAPI imga_present(IDirect3DDevice9* dev, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion)
{
	IDirect3DStateBlock9* state;
	dev->CreateStateBlock(D3DSBT_ALL, &state);

	ImGui_ImplDX9_NewFrame();
	call_onframe(dev);

	state->Apply();
	state->Release();

	return ((d3d9Present_t)g__ctx->vftable_original[17])(dev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

static HRESULT WINAPI imga_reset(IDirect3DDevice9* dev, D3DPRESENT_PARAMETERS* params)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	call_onprereset(dev, params);
	HRESULT result = ((d3d9Reset_t)g__ctx->vftable_original[16])(dev, params);
	if (result == D3D_OK) {
		ImGui_ImplDX9_CreateDeviceObjects();
		call_onpostreset(dev, params);
	}
	return result;
}

extern IMGUI_API LRESULT ImGui_ImplDX9_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI imga_wndproc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplDX9_WndProcHandler(wnd, msg, wparam, lparam))
		return TRUE;
	return CallWindowProc(g__ctx->original_wndproc, wnd, msg, wparam, lparam);
}

bool
imga::Initialize(void* hWnd, Context* ctx, DeviceFetcher_t fetcher) {
	if (ctx == nullptr)
		g__ctx = new Context{ nullptr, std::vector<Module*>(4) };
	else
		g__ctx = ctx;


	IDirect3DDevice9* dev;
	if (fetcher)
		dev = fetcher();
	else
		dev = getd3d9device_default();

	ImGui_ImplDX9_Init(hWnd, dev);
	memcpy(g__ctx->vftable_fake, *(void***)dev, 0x200 * sizeof(void*));
	g__ctx->vftable_original = *(void***)dev;
	*(void***)dev = g__ctx->vftable_fake;

	g__ctx->dev = dev;

	g__ctx->original_wndproc = (WNDPROC)SetWindowLongPtr((HWND)hWnd, GWL_WNDPROC, (LONG)imga_wndproc);
	
	return true;
}

void 
imga::AddModule(Module* m) {
	g__ctx->modules.push_back(m);
}

void 
imga::RemoveModule(Module* m) {
	for (auto& it = g__ctx->modules.begin(); it != g__ctx->modules.end(); ++it)
		if (*it == m)
			g__ctx->modules.erase(it);
}

void 
imga::Destruct() {
	SetWindowLongPtr((HWND)g__ctx->hwnd, GWL_WNDPROC, (LONG)g__ctx->original_wndproc);
	*(void***)g__ctx->dev = g__ctx->vftable_original;
	ImGui_ImplDX9_Shutdown();
}