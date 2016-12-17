#include "..\..\imga.h"

#pragma comment(lib, "d3d9.lib")

class TestWindowModule : public imga::Module {

	void OnFrame(IDirect3DDevice9* dev) {
		ImGui::ShowTestWindow();
	}

};



DWORD WINAPI DllMain(HMODULE mod, DWORD reason, LPVOID reserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		if (!imga::Initialize(*(HWND*)0xa377a8))
			return FALSE;
		imga::AddModule(new TestWindowModule());
		break;
	case DLL_PROCESS_DETACH:
		imga::Destruct();
		break;
	}
	return TRUE;
}