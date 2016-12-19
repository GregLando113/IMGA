#include "..\imga.h"

#include "..\imgui\imgui_impl_dx9.h"

typedef HRESULT(WINAPI *d3d9Endscene_t)(IDirect3DDevice9* dev);
typedef HRESULT(WINAPI *d3d9Reset_t)(IDirect3DDevice9* dev, D3DPRESENT_PARAMETERS* params);

extern IDirect3DDevice9* getd3d9device_default();

imga::Context* g__ctx = nullptr;
imga::Context* imga::get_ctx() { return g__ctx; }

class PageProtection {
public:
	PageProtection(void* addr, DWORD len, DWORD prot):
	addr_(addr),
	len_(len),
	prot_(prot){
		VirtualProtect(addr_, len_, prot_, &old_);
	}
	~PageProtection() {
		VirtualProtect(addr_, len_, old_, &old_);
	}
private:
	void* addr_;
	DWORD len_;
	DWORD prot_;
	DWORD old_;
};

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

static HRESULT WINAPI imga_endscene(IDirect3DDevice9* dev)
{
	IDirect3DStateBlock9* state;
	dev->CreateStateBlock(D3DSBT_ALL, &state);

	ImGui_ImplDX9_NewFrame();
	call_onframe(dev);
	ImGui::Render();
	state->Apply();
	state->Release();

	return ((d3d9Endscene_t)g__ctx->vftable_original[42])(dev);
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
	ImGui_ImplDX9_WndProcHandler(wnd, msg, wparam, lparam);

	auto& io = ImGui::GetIO();
	switch (msg) {
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_MOUSEWHEEL:
		if (io.WantCaptureMouse)
			return true;
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_IME_CHAR:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		if (io.WantTextInput)
			return true;
		break;
	}

	return CallWindowProc(g__ctx->original_wndproc, wnd, msg, wparam, lparam);
}

bool
imga::Initialize(Context* ctx, DeviceFetcher_t fetcher) {
	if (ctx == nullptr)
		g__ctx = new Context;
	else
		g__ctx = ctx;


	IDirect3DDevice9* dev;
	if (fetcher)
		dev = fetcher();
	else
		dev = getd3d9device_default();

	D3DDEVICE_CREATION_PARAMETERS cparams;
	dev->GetCreationParameters(&cparams);
	g__ctx->hwnd = cparams.hFocusWindow;

	ImGui_ImplDX9_Init(cparams.hFocusWindow, dev);

	DWORD oldprot;

	g__ctx->vftable_original = *(void***)dev;
	g__ctx->vftable_fake = (void**)malloc(0x200 * sizeof(void*));
	VirtualProtect(g__ctx->vftable_fake, sizeof(void*)*0x200, PAGE_EXECUTE_READWRITE, &oldprot);
	memcpy(g__ctx->vftable_fake, g__ctx->vftable_original, 0x200 * sizeof(void*));
	{
		PageProtection p((void*)dev, sizeof(void*), PAGE_EXECUTE_READWRITE);
		*(void***)dev = g__ctx->vftable_fake;
	}

	g__ctx->dev = dev;

	g__ctx->vftable_fake[42] = imga_endscene;
	g__ctx->vftable_fake[16] = imga_reset;


	g__ctx->original_wndproc = (WNDPROC)SetWindowLongPtr((HWND)cparams.hFocusWindow, GWLP_WNDPROC, (LONG_PTR)imga_wndproc);
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
	SetWindowLongPtr((HWND)g__ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)g__ctx->original_wndproc);
	if(g__ctx->dev)
	{
		PageProtection p((void*)g__ctx->dev, sizeof(void*), PAGE_EXECUTE_READWRITE);
		*(void***)g__ctx->dev = g__ctx->vftable_original; 
	}
	ImGui_ImplDX9_Shutdown();
}