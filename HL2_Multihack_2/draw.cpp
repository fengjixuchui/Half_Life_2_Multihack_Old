#include "draw.h"

void Draw::DrawBorderBox(int x, int y, int w, int h, int thickness, D3DCOLOR color, IDirect3DDevice9* pDevice)
{
	DrawFilledRect(x, y, w, thickness, color, pDevice); //Top horiz line
	DrawFilledRect(x, y, thickness, h, color, pDevice); // Left vertical line
	DrawFilledRect((x + w), y, thickness, h, color, pDevice); //right vertical line
	DrawFilledRect(x, y + h, w + thickness, thickness, color, pDevice); //bottom horiz line
}

void Draw::DrawFilledRect(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9 * dev)
{
	D3DRECT BarRect = { x, y, x + w, y + h }; //create the rect
	dev->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, color, 0, 0); //clear = re-draw target area
}

void Draw::DrawString(LPCSTR TextToDraw, int x, int y, D3DCOLOR color, LPD3DXFONT m_font)
{
	RECT rct = { x - 120, y, x + 120, y + 15 }; //create the rect
	m_font->DrawText(NULL, TextToDraw, -1, &rct, DT_NOCLIP, color);
}

std::string IntToString(int number)
{
	std::stringstream ss;//create a stringstream
	ss << number;//add number to the stream
	return ss.str();//return a string version
}

std::string FloatToString(float number)
{
	//Round our number down
	number = floorf(number);
	std::stringstream ss;
	ss << number;
	return ss.str();
}