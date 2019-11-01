#include <iostream>
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <detours.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include "memory.h"
#include "draw.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "detours.lib")


//Startup

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);

static HWND window;

//Defines

#define COLOR_WHITE           ImVec4(1, 1, 1, 1)
#define COLOR_DARK_GRAY       ImVec4(0.02f, 0.02f, 0.02f, 1);
#define COLOR_DARK_GRAY_2     ImVec4(0.008f, 0.008f, 0.008f, 1);
#define COLOR_WHITE_GRAY      ImVec4(0.4f, 0.4f, 0.4f, 1);
#define COLOR_LIGHT_GRAY      ImVec4(0.05f, 0.05f, 0.05f, 1);
#define COLOR_LIGHT_RED		  ImVec4(0.8f, 0, 0, 1);
#define COLOR_DARK_RED		  ImVec4(0.5f, 0, 0, 1);
#define COLOR_RED             ImVec4(1, 0, 0, 1)
#define COLOR_BLACK           ImVec4(0, 0, 0, 1)

//Keys

#define KEY_MENU VK_INSERT
#define KEY_BHOP VK_SPACE

//Variables

RECT screen;
bool loaded = false;
int crosshairSize[2];
float crosshairColors[4];

//Menu Variables

static bool bhop = false;
static bool crosshair = false;
static bool menu_crosshair = false;
static bool inf_health = false;
static bool inf_armor = false;
static bool inf_aux_power = false;
static bool inf_ammo = false;

struct gameOffsets
{
	//Game Modules

	char clientMod[11] = "client.dll";
	char engineMod[11] = "engine.dll";
	char serverMod[11] = "server.dll";
	char shaderapiMod[17] = "shaderapidx9.dll";

	//client.dll
	const DWORD dwJump = 0x48BF5C;
	const DWORD bCrosshairOn = 0x489608;

	//engine.dll
	const DWORD bInGame = 0x3F28F0;

	//server.dll
	const DWORD playerBase = 0x634174;

	//shaderapidx9.dll

	const DWORD bShadersLoaded = 0x184778;

	//Player

	const DWORD dwHealth = 0xE0;
	const DWORD dwArmor = 0xD30;
	const DWORD flAuxPower = 0x10CC;
	const DWORD bOnAir = 0xB60;
}offsets;

struct gameBytes
{
	char subNop[3] = "\x90\x90";
	char subNop2[7] = "\x90\x90\x90\x90\x90\x90";

	char subAmmoP[176] = "\x89\x3E\x5F\x8B\xC6\x5E\x5D\xC2\x04\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xA1\x00\x00\x00\x00\xA8\x01\x75\x5B\x83\xC8\x01\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x68\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xA1\x00\x00\x00\x00\xA8\x01\x75\x60"; //Pattern
	char subAmmoM[176] = "xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxx????????x????x????xx????????xx????????xx????????xx????????xx????????xx????????x????xxxxx????????x????xx????????xx????????xxxxxxxxxx????xxxx"; //Mask
	char subAmmoO[2]; //Original Bytes

	char subAmmo2P[33] = "\x89\x1E\x8B\xC7\x5F\x5E\x5B\x8B\xE5\x5D\xC2\x04\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x55\x8B\xEC\x56\x68\x00\x00\x00\x00";
	char subAmmo2M[33] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxx????";
	char subAmmo2O[2];

	char subAmmo3P[12] = "\x89\x83\x00\x00\x00\x00\x8B\x06\x8D\x4D\xE8";
	char subAmmo3M[12] = "xx????xxxxx";
	char subAmmo3O[6];

	char subAmmo4P[6] = "\x89\x1E\x5F\x5E\x5B";
	char subAmmo4M[6] = "xxxxx";
	char subAmmo4O[6];

	char subAmmo5P[9] = "\x89\x87\x00\x00\x00\x00\x8B\x03";
	char subAmmo5M[9] = "xx????xx";
	char subAmmo5O[6];
}bytes;

struct gameValues
{
	//Game addresses
	DWORD client;
	DWORD engine;
	DWORD server;
	DWORD shaderapi;
	DWORD inGame;
	DWORD localPlayer;
	//Game functions' addresses
	DWORD subAmmo;
	DWORD subAmmo2;
	DWORD subAmmo3;
	DWORD subAmmo4;
	DWORD subAmmo5;
	DWORD subHealth;
	DWORD subArmor;
}game;

//Hack

void Bunnyhop()
{
	if (bhop)
	{
		DWORD onAir = *(DWORD*)(game.localPlayer + offsets.bOnAir);
		if(GetAsyncKeyState(KEY_BHOP) && onAir == 0)
			*(DWORD*)(game.client + offsets.dwJump) = 2;
	}
}

void CustomCrosshair(IDirect3DDevice9* pDevice)
{
	if (crosshair && loaded && game.inGame != 0)
	{
		int w = crosshairSize[0];
		int h = crosshairSize[1];
		D3DCOLOR color = D3DCOLOR_ARGB((int)(crosshairColors[3] * 255), (int)(crosshairColors[0] * 255), (int)(crosshairColors[1] * 255), (int)(crosshairColors[2] * 255));
		w % 2 == 1 ? 0 : w += 1;
		h % 2 == 1 ? 0 : h += 1;
		int screenW = screen.right - screen.left;
		int screenH = screen.bottom - screen.top;
		int screenCenterX = (int)(screenW / 2);
		int screenCenterY = (int)(screenH / 2);
		int centerX = (int)(w / 2);
		int centerY = (int)(h / 2);
		Draw::DrawFilledRect(screenCenterX - centerX, screenCenterY - centerY, w, h, color, pDevice);
		Draw::DrawFilledRect(screenCenterX - centerY, screenCenterY - centerX, h, w, color, pDevice);

		DWORD crosshairOn = *(DWORD*)(game.client + offsets.bCrosshairOn);
		if (crosshairOn != 0)
		{
			*(DWORD*)(game.client + offsets.bCrosshairOn) = 0;
		}
	}

	else
	{
		DWORD crosshairOn = *(DWORD*)(game.client + offsets.bCrosshairOn);
		if(crosshairOn != 1)
			*(DWORD*)(game.client + offsets.bCrosshairOn) = 1;
	}
}

void InfiniteAmmo()
{
	if (loaded)
	{
		if (inf_ammo)
		{
			WriteToMemory(game.subAmmo, bytes.subNop, 2);
			WriteToMemory(game.subAmmo2, bytes.subNop, 2);
			WriteToMemory(game.subAmmo3, bytes.subNop2, 6);
			WriteToMemory(game.subAmmo4, bytes.subNop, 2);
			WriteToMemory(game.subAmmo5, bytes.subNop2, 6);
		}
		else if (!inf_ammo)
		{
			WriteToMemory(game.subAmmo, bytes.subAmmoO, 2);
			WriteToMemory(game.subAmmo2, bytes.subAmmo2O, 2);
			WriteToMemory(game.subAmmo3, bytes.subAmmo3O, 6);
			WriteToMemory(game.subAmmo4, bytes.subAmmo4O, 2);
			WriteToMemory(game.subAmmo5, bytes.subAmmo5O, 6);
		}
	}
}

void InfiniteHealth()
{
	if (inf_health)
	{
		DWORD health = *(DWORD*)(game.localPlayer + offsets.dwHealth);
		if (health < 1000)
		{
			*(DWORD*)(game.localPlayer + offsets.dwHealth) = 1000;
		}
	}
}

void InfiniteArmor()
{
	if (inf_armor)
	{
		DWORD armor = *(DWORD*)(game.localPlayer + offsets.dwArmor);
		if (armor < 1000)
		{
			*(DWORD*)(game.localPlayer + offsets.dwArmor) = 1000;
		}
	}
}

void InfiniteAuxPower()
{
	if (inf_aux_power)
	{
		float auxPower = *(float*)(game.localPlayer + offsets.flAuxPower);
		if (auxPower < 100)
		{
			*(float*)(game.localPlayer + offsets.flAuxPower) = 100;
		}
	}
}

DWORD WINAPI Hack(LPVOID lpReserved)
{
	game.client = (DWORD)GetModuleHandle(offsets.clientMod);
	game.engine = (DWORD)GetModuleHandle(offsets.engineMod);
	game.server = (DWORD)GetModuleHandle(offsets.serverMod);
	game.shaderapi = (DWORD)GetModuleHandle(offsets.shaderapiMod);
	game.subAmmo = FindPattern(offsets.serverMod, bytes.subAmmoP, bytes.subAmmoM);
	game.subAmmo2 = FindPattern(offsets.serverMod, bytes.subAmmo2P, bytes.subAmmo2M);
	game.subAmmo3 = FindPattern(offsets.serverMod, bytes.subAmmo3P, bytes.subAmmo3M);
	game.subAmmo4 = FindPattern(offsets.serverMod, bytes.subAmmo4P, bytes.subAmmo4M);
	game.subAmmo5 = FindPattern(offsets.serverMod, bytes.subAmmo5P, bytes.subAmmo5M);

	memcpy(&bytes.subAmmoO, reinterpret_cast<char*>(game.subAmmo), 2);
	memcpy(&bytes.subAmmo2O, reinterpret_cast<char*>(game.subAmmo2), 2);
	memcpy(&bytes.subAmmo3O, reinterpret_cast<char*>(game.subAmmo3), 6);
	memcpy(&bytes.subAmmo4O, reinterpret_cast<char*>(game.subAmmo4), 2);
	memcpy(&bytes.subAmmo5O, reinterpret_cast<char*>(game.subAmmo5), 6);

	crosshairSize[0] = 1;
	crosshairSize[1] = 10;
	crosshairColors[0] = 1;
	crosshairColors[1] = 0;
	crosshairColors[2] = 0;
	crosshairColors[3] = 1;
	while (loaded == false)
	{
		GetClientRect(window, &screen);
		if (screen.right != 0 && screen.bottom != 0)
			loaded = true;
	}

	while (true)
	{
		GetClientRect(window, &screen);
		game.inGame = *(DWORD*)(game.shaderapi + offsets.bShadersLoaded);
		if (game.inGame != 0)
		{
			game.localPlayer = *(DWORD*)(game.server + offsets.playerBase);
			Bunnyhop();
			InfiniteHealth();
			InfiniteArmor();
			InfiniteAuxPower();
		}
	}
	return 0;
}

//Hook

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE; // skip to next window

	window = handle;
	return FALSE; // window found abort search
}

HWND GetProcessWindow()
{
	window = NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}

bool GetD3D9Device(void** pTable, size_t Size)
{
	if (!pTable)
		return false;

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!pD3D)
		return false;

	IDirect3DDevice9* pDummyDevice = NULL;

	// options to create dummy device
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = false;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetProcessWindow();

	HRESULT dummyDeviceCreated = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

	if (dummyDeviceCreated != S_OK)
	{
		// may fail in windowed fullscreen mode, trying again with windowed mode
		d3dpp.Windowed = !d3dpp.Windowed;

		dummyDeviceCreated = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

		if (dummyDeviceCreated != S_OK)
		{
			pD3D->Release();
			return false;
		}
	}

	memcpy(pTable, *reinterpret_cast<void***>(pDummyDevice), Size);

	pDummyDevice->Release();
	pD3D->Release();
	return true;
}

void* d3d9Device[119];
EndScene oEndScene = NULL;
DWORD endSceneAddr;

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
WNDPROC oWndProc;

void InitImGui(LPDIRECT3DDEVICE9 pDevice)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(pDevice);
}

void Menu()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = COLOR_WHITE;
	colors[ImGuiCol_WindowBg] = COLOR_DARK_GRAY;
	colors[ImGuiCol_TitleBg] = COLOR_RED;
	colors[ImGuiCol_TitleBgActive] = COLOR_DARK_RED;
	colors[ImGuiCol_TitleBgCollapsed] = COLOR_DARK_RED;
	colors[ImGuiCol_FrameBg] = COLOR_BLACK;
	colors[ImGuiCol_FrameBgHovered] = COLOR_DARK_GRAY_2;
	colors[ImGuiCol_FrameBgActive] = COLOR_LIGHT_GRAY;
	/*
	colors[ImGuiCol_FrameBg] = COLOR_WHITE;
	colors[ImGuiCol_FrameBgHovered] = COLOR_WHITE_GRAY;
	colors[ImGuiCol_FrameBgActive] = COLOR_LIGHT_GRAY;
	*/
	colors[ImGuiCol_Button] = COLOR_RED;
	colors[ImGuiCol_ButtonActive] = COLOR_DARK_RED;
	colors[ImGuiCol_ButtonHovered] = COLOR_LIGHT_RED;
	colors[ImGuiCol_SliderGrab] = COLOR_RED;
	colors[ImGuiCol_SliderGrabActive] = COLOR_DARK_RED;
	colors[ImGuiCol_CheckMark] = COLOR_RED;

	ImGui::Begin("HL2 Multihack by rdbo");
	ImGui::Checkbox("Bunnyhop", &bhop);
	ImGui::Checkbox("Infinite Health", &inf_health);
	ImGui::Checkbox("Infinite Armor", &inf_armor);
	ImGui::Checkbox("Infinite Aux Power", &inf_aux_power);
	if (ImGui::Checkbox("Infinite Ammo", &inf_ammo))
		InfiniteAmmo();
	if (ImGui::Button("Custom Crosshair"))
		menu_crosshair = !menu_crosshair;
	if(menu_crosshair)
	{
		ImGui::Begin("Custom Crosshair");
		ImGui::Checkbox("Custom Crosshair", &crosshair);
		ImGui::SliderInt("Crosshair Width", &crosshairSize[0], 1, (screen.right - screen.left), "%i");
		ImGui::SliderInt("Crosshair Height", &crosshairSize[1], 1, (screen.bottom - screen.top), "%i");
		ImGui::ColorPicker4("Crosshair Color", crosshairColors);
		ImGui::End();
	}
	ImGui::End();
}

long __stdcall EndSceneHook(LPDIRECT3DDEVICE9 pDevice)
{
	//Init
	static bool init = true;
	if (init)
	{
		InitImGui(pDevice);
		init = false;
	}

	//ImGUI Main Loop

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//	Your Code Here

	if (GetKeyState(KEY_MENU))
		if (GetKeyState(KEY_MENU) & 1)
			Menu();

	if(loaded)
		CustomCrosshair(pDevice);

	//	Render
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	//DirectX Code Here
	oEndScene = (EndScene)endSceneAddr;
	return oEndScene(pDevice);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	if (GetD3D9Device(d3d9Device, sizeof(d3d9Device)))
	{
		oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		endSceneAddr = (DWORD)d3d9Device[42];
		DetourAttach(&(LPVOID&)endSceneAddr, &EndSceneHook);
		DetourTransactionCommit();
	}
	return 0;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hMod);
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, Hack, hMod, 0, nullptr);
	}

	else if (dwReason == DLL_PROCESS_DETACH)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(LPVOID&)endSceneAddr, &EndSceneHook);
		DetourTransactionCommit();
	}
	return TRUE;
}