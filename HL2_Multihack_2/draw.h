#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <sstream>
#include <string>

namespace Draw
{
	void DrawString(LPCSTR TextToDraw, int x, int y, D3DCOLOR color, LPD3DXFONT m_font);
	void DrawFilledRect(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* d3dDevice);
	void DrawBorderBox(int x, int y, int w, int h, int thickness, D3DCOLOR color, IDirect3DDevice9* d3dDevice);
};

std::string IntToString(int number);
std::string FloatToString(float number);