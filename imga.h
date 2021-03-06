#pragma once

#include <Windows.h>

#include <d3d9.h>
#include <vector>

#include "imgui\imgui.h"

namespace imga {

	typedef IDirect3DDevice9*(*DeviceFetcher_t)();

	class Module 
	{
	public:
		virtual bool OnInit(IDirect3DDevice9* dev) { return true; }
		virtual void OnFrame(IDirect3DDevice9* dev){}
		virtual void OnCleanup(IDirect3DDevice9* dev){}
		virtual void OnPreReset(IDirect3DDevice9* dev){}
		virtual void OnPostReset(IDirect3DDevice9* dev){}
	};

	struct Context
	{
		IDirect3DDevice9* dev = nullptr;
		std::vector<Module*> modules;
		void** vftable_fake = nullptr;
		void** vftable_original = nullptr;
		WNDPROC original_wndproc = nullptr;
		HWND hwnd = nullptr;
	};

	bool Initialize(Context* ctx = nullptr, DeviceFetcher_t fetcher = nullptr);
	void AddModule(Module* m);
	void RemoveModule(Module* m);
	void Destruct();

	Context* get_ctx();

}