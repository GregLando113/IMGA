#include "..\..\imga.h"

#pragma comment(lib, "d3d9.lib")

class TestWindowModule : public imga::Module {

	void OnFrame(IDirect3DDevice9* dev) {
		ImGui::ShowTestWindow();
	}

};


DWORD WINAPI thread(LPVOID) {
	if (!imga::Initialize())
		return FALSE;
	imga::AddModule(new TestWindowModule());
	return 0;
}


DWORD WINAPI DllMain(HMODULE mod, DWORD reason, LPVOID reserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, thread, 0, 0, 0);
		break;
	case DLL_PROCESS_DETACH:
		imga::Destruct();
		break;
	}
	return TRUE;
}