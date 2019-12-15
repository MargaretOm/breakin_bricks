#include <windows.h>
#include "resource.h"
#include <array>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadResources();
void InitializeBall(HWND);
void InitializePlatform(HWND);
void InitializeBricks(HWND);
void DrawAll(HWND);

enum DirectionX { LEFT, RIGHT, NONE_X };
enum DirectionY { UP, DOWN, NONE_Y };
enum Status { FIRST, SECOND };

int timer = 1;

HBITMAP hBmpBall;
HBITMAP hBmpPlatform;
HBITMAP hBmpRedBrick;
HBITMAP hBmpGreenBrick;
HBITMAP hBmpGameOver;
HBITMAP hBmpYouWin;
HBITMAP hBmpBadBrick;
bool flagGameOver = false;
bool flagYouWin = false;

BOOL DrawBitmap(HDC hDc, int x, int y, HBITMAP hBitmap);

constexpr auto START_SPEED = 20.0;
constexpr auto BOOST = 0.25;
constexpr auto ALLOWED_FAULT = 1.0;

typedef struct ball {
	float X, Y, Radius;
	float SpeedX, SpeedY;
	DirectionX directionX;
	DirectionY directionY;
} Ball;

typedef struct Platform {
	float X, Y, height, width;
	DirectionX directionX;
} Platform;

typedef struct Brick {
	float X, Y, height, width;
	Status status;
} Brick;

Ball ball;
Platform platform;
std::array< std::array<Brick*, 10>, 5 > arrayOfBricks;

int WINDOW_WIDTH = 1000;
int WINDOW_HEIGHT = 600;

void SetUpLeftHit();
void SetUpRightHit();

BOOL LeftHittenBall();
BOOL RightHittenBall();
BOOL TopHittenBall();
BOOL BottomHittenBall();

BOOL LeftHittenPlatform();
BOOL RightHittenPlatform();

void RecalculateBallPosition();
void RecalculatePlatformPosition();

void ThreadProc(HWND);

HINSTANCE hInst;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hMainWnd;
	wchar_t szClassName[] = L"MyClass";
	MSG msg;
	WNDCLASSEX wc;
	hInst = hInstance;

	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Cannot register class", L"Error", MB_OK);
		return 0;
	}

	hMainWnd = CreateWindow(
		szClassName, L"Breakin bricks", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, 0,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		(HWND)NULL, (HMENU)NULL,
		(HINSTANCE)hInstance, NULL
	);
	if (!hMainWnd) {
		MessageBox(NULL, L"Cannot create main window", L"Error", MB_OK);
		return 0;
	}

	ShowWindow(hMainWnd, nCmdShow);

	while (GetMessage(&msg, NULL, 0, 0)) {
		DispatchMessage(&msg);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		LoadResources();
		InitializeBall(hWnd);
		InitializePlatform(hWnd);
		InitializeBricks(hWnd);
		SetTimer(hWnd, timer, 1, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, hWnd, 0, NULL);
		break;
	case WM_TIMER:
		RecalculateBallPosition();
		RecalculatePlatformPosition();
		break;
	case WM_PAINT:
		DrawAll(hWnd);
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			SetUpLeftHit();
			break;
		case VK_RIGHT:
			SetUpRightHit();
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void ThreadProc(HWND hWnd)
{
	bool flagArrayNULL;
	while (!flagGameOver && !flagYouWin) {
		flagArrayNULL = true;
		for (int i = 0; i <= 4; i++) {
			for (int j = 0; j <= 9; j++) {
				if (arrayOfBricks[i][j] != NULL) {
					flagArrayNULL = false;
					break;
				}
			}
			if (!flagArrayNULL) { break; }
		}
		if (flagArrayNULL) {
			flagYouWin = true;
		}
		InvalidateRect(hWnd, NULL, true);
		Sleep(40);
	}
	InvalidateRect(hWnd, NULL, true);
	KillTimer(hWnd, 1);
}

void LoadResources()
{
	hBmpBall = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1));
	hBmpPlatform = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP2));
	hBmpRedBrick = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP3));
	hBmpGreenBrick = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP5));
	hBmpGameOver = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP4));
	hBmpYouWin = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP6));
	hBmpBadBrick = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP8));
}

void InitializeBall(HWND hWnd)
{
	BITMAP bmp;
	GetObject(hBmpBall, sizeof(BITMAP), (LPSTR)& bmp);
	ball.X = WINDOW_WIDTH / 2;
	ball.Y = WINDOW_HEIGHT / 2;
	ball.Radius = bmp.bmWidth / 2;
	ball.SpeedX = 5.0;
	ball.SpeedY = 5.0;
	ball.directionX = NONE_X;
	ball.directionY = DOWN;
}

void InitializePlatform(HWND hWnd)
{
	BITMAP bmp;
	GetObject(hBmpPlatform, sizeof(BITMAP), (LPSTR)& bmp);
	platform.X = WINDOW_WIDTH / 2 - bmp.bmWidth / 2;
	platform.Y = 6 * (WINDOW_HEIGHT / 7);
	platform.width = bmp.bmWidth;
	platform.height = bmp.bmHeight;
	platform.directionX = NONE_X;
}

void InitializeBricks(HWND hWnd)
{
	BITMAP bmp;
	GetObject(hBmpRedBrick, sizeof(BITMAP), (LPSTR)& bmp);
	float x = 0, y = 0;
	for (int i = 0; i <= 4; i++) {
		for (int j = 0; j <= 9; j+=2) {
			arrayOfBricks[i][j] = new Brick();
			arrayOfBricks[i][j]->X = x;
			arrayOfBricks[i][j]->Y = y;
			arrayOfBricks[i][j]->width = bmp.bmWidth;
			arrayOfBricks[i][j]->height = bmp.bmHeight;
			arrayOfBricks[i][j]->status = FIRST;
			x = x + 2 * bmp.bmWidth;
		}
		x = bmp.bmWidth;
		for (int j = 1; j <= 9; j += 2) {
			arrayOfBricks[i][j] = new Brick();
			arrayOfBricks[i][j]->X = x;
			arrayOfBricks[i][j]->Y = y;
			arrayOfBricks[i][j]->width = bmp.bmWidth;
			arrayOfBricks[i][j]->height = bmp.bmHeight;
			arrayOfBricks[i][j]->status = FIRST;
			x = x + 2 * bmp.bmWidth;
		}
		x = 0;
		y = y + bmp.bmHeight;
	}
}

void DrawAll(HWND hWnd)
{
	HDC hdc;
	PAINTSTRUCT ps;
	hdc = BeginPaint(hWnd, &ps);
	if (flagGameOver) {
		DrawBitmap(hdc, 0, 0, hBmpGameOver);
	}
	else if (flagYouWin) {
		DrawBitmap(hdc, 0, 0, hBmpYouWin);
	}
	else
	{
		DrawBitmap(hdc, ball.X - ball.Radius, ball.Y - ball.Radius, hBmpBall);
	}
	DrawBitmap(hdc, platform.X, platform.Y, hBmpPlatform);
	for (int i = 0; i <= 4; i++) {
		for (int j = 0; j <= 9; j+=2) {
			if (arrayOfBricks[i][j] != NULL)
			{
				if (arrayOfBricks[i][j]->status == FIRST) {
					DrawBitmap(hdc, arrayOfBricks[i][j]->X, arrayOfBricks[i][j]->Y, hBmpRedBrick);
				}
				else {
					DrawBitmap(hdc, arrayOfBricks[i][j]->X, arrayOfBricks[i][j]->Y, hBmpBadBrick);
				}
			}
		}
		for (int j = 1; j <= 9; j += 2) {
			if (arrayOfBricks[i][j] != NULL)
			{
				if (arrayOfBricks[i][j]->status == FIRST) {
					DrawBitmap(hdc, arrayOfBricks[i][j]->X, arrayOfBricks[i][j]->Y, hBmpGreenBrick);
				}
				else {
					DrawBitmap(hdc, arrayOfBricks[i][j]->X, arrayOfBricks[i][j]->Y, hBmpBadBrick);
				}
			}
		}
	}
	//BOOL result = Ellipse(hdc, ball.X - ball.Radius, ball.Y - ball.Radius, ball.X + ball.Radius, ball.Y + ball.Radius);
	//if (result == FALSE) {
	//	/*MessageBox(NULL,
	//		_T("Call to DrawBitmap failed!"),
	//		_T("Windows Desktop Guided Tour"),
	//		NULL);*/
	//}
	EndPaint(hWnd, &ps);
}

void SetUpLeftHit()
{
	platform.directionX = LEFT;
}

void SetUpRightHit()
{
	platform.directionX = RIGHT;
}

void RecalculateBallPosition()
{
	switch (ball.directionX) {
	case LEFT:
		ball.X -= ball.SpeedX;
		if (LeftHittenBall()) { ball.directionX = RIGHT; }
		break;
	case RIGHT:
		ball.X += ball.SpeedX;
		if (RightHittenBall()) { ball.directionX = LEFT; }
		break;
	case NONE_X:
		break;
	}

	switch (ball.directionY) {
	case UP:
		ball.Y -= ball.SpeedY;
		if (TopHittenBall()) { ball.directionY = DOWN; }
		break;
	case DOWN:
		ball.Y += ball.SpeedY;
		if (BottomHittenBall()) { ball.directionY = UP; }
		break;
	case NONE_Y:
		break;
	}
}

void RecalculatePlatformPosition()
{
	switch (platform.directionX) {
	case LEFT:
		if (!LeftHittenPlatform()) { platform.X -= 30; }
		break;
	case RIGHT:
		if (!RightHittenPlatform()) { platform.X += 30; }
		break;
	case NONE_X:
		break;
	}
	platform.directionX = NONE_X;

	/*switch (platform.directionY) {
	case UP:
		platform.Y -= ball.SpeedY - ball.BoostY / 2;
		if (TopHitten()) { ball.directionY = DOWN; }
		break;
	case DOWN:
		ball.Y += ball.SpeedY - ball.BoostY / 2;
		if (BottomHitten()) { ball.directionY = UP; }
		break;
	case NONE_Y:
		break;
	}*/
}

BOOL LeftHittenBall()
{
	for (int i = 4; i >= 0; i--) {
		for (int j = 9; j >= 0; j--) {
			if (arrayOfBricks[i][j] != NULL
				&& arrayOfBricks[i][j]->X <= ball.X - ball.Radius
				&& arrayOfBricks[i][j]->X + arrayOfBricks[i][j]->width >= ball.X - ball.Radius
				&& arrayOfBricks[i][j]->Y <= ball.Y
				&& arrayOfBricks[i][j]->Y + arrayOfBricks[i][j]->height >= ball.Y)
			{
				if (arrayOfBricks[i][j]->status == FIRST) {
					arrayOfBricks[i][j]->status = SECOND;
				}
				else
				{
					arrayOfBricks[i][j] = NULL;
				}
				return TRUE;
			}
		}
	}
	if (ball.X - ball.Radius <= 0) { return TRUE; }
	else { return FALSE; }
}

BOOL RightHittenBall()
{
	for (int i = 4; i >= 0; i--) {
		for (int j = 9; j >= 0; j--) {
			if (arrayOfBricks[i][j] != NULL
				&& arrayOfBricks[i][j]->X <= ball.X + ball.Radius
				&& arrayOfBricks[i][j]->X + arrayOfBricks[i][j]->width >= ball.X + ball.Radius
				&& arrayOfBricks[i][j]->Y <= ball.Y
				&& arrayOfBricks[i][j]->Y + arrayOfBricks[i][j]->height >= ball.Y)
			{
				if (arrayOfBricks[i][j]->status == FIRST) {
					arrayOfBricks[i][j]->status = SECOND;
				}
				else
				{
					arrayOfBricks[i][j] = NULL;
				}
				return TRUE;
			}
		}
	}
	if (ball.X + ball.Radius >= WINDOW_WIDTH) { return TRUE; }
	else { return FALSE; }
}

BOOL TopHittenBall()
{
	for (int i = 4; i >= 0; i--) {
		for (int j = 9; j >= 0; j--) {
			if (arrayOfBricks[i][j] != NULL
				&& arrayOfBricks[i][j]->Y + arrayOfBricks[i][j]->height >= ball.Y - ball.Radius
				&& arrayOfBricks[i][j]->Y <= ball.Y - ball.Radius
				&& arrayOfBricks[i][j]->X <= ball.X
				&& arrayOfBricks[i][j]->X + arrayOfBricks[i][j]->width >= ball.X)
			{
				if (arrayOfBricks[i][j]->status == FIRST) {
					arrayOfBricks[i][j]->status = SECOND;
				}
				else
				{
					arrayOfBricks[i][j] = NULL;
				}
				return TRUE;
			}
		}
	}
	if (ball.Y - ball.Radius <= 0) { return TRUE; }
	else { return FALSE; }
}

BOOL BottomHittenBall()
{
	for (int i = 4; i >= 0; i--) {
		for (int j = 9; j >= 0; j--) {
			if (arrayOfBricks[i][j] != NULL
				&& arrayOfBricks[i][j]->Y + arrayOfBricks[i][j]->height >= ball.Y + ball.Radius
				&& arrayOfBricks[i][j]->Y <= ball.Y + ball.Radius
				&& arrayOfBricks[i][j]->X <= ball.X
				&& arrayOfBricks[i][j]->X + arrayOfBricks[i][j]->width >= ball.X)
			{
				if (arrayOfBricks[i][j]->status == FIRST) {
					arrayOfBricks[i][j]->status = SECOND;
				}
				else
				{
					arrayOfBricks[i][j] = NULL;
				}
				return TRUE;
			}
		}
	}
	if (ball.Y + ball.Radius >= platform.Y
		&& ball.Y + ball.Radius <= platform.Y + platform.height
		&& ball.X <= platform.X + platform.width
		&& ball.X >= platform.X
		) 
	{
		if (ball.X <= platform.X + 2 * (platform.width / 3) && ball.X >= platform.X + platform.width / 3) {
			ball.directionX = NONE_X;
		}
		if (ball.X <= platform.X + platform.width / 3 && ball.X >= platform.X) {
			ball.directionX = LEFT;
		}
		if (ball.X <= platform.X + platform.width && ball.X >= platform.X + 2 * (platform.width / 3)) {
			ball.directionX = RIGHT;
		}
		return TRUE;
	}
	if (ball.Y + ball.Radius >= WINDOW_HEIGHT) { flagGameOver = true; return TRUE; }
	else { return FALSE; }
}

BOOL LeftHittenPlatform()
{
	if (platform.X <= 0) { return TRUE; }
	else { return FALSE; }
}

BOOL RightHittenPlatform()
{
	if (platform.X + platform.width >= WINDOW_WIDTH) { return TRUE; }
	else { return FALSE; }
}

BOOL DrawBitmap(HDC hDC, int x, int y, HBITMAP hBitmap)
{
	HBITMAP hBmp, hBmpOld;
	HDC hMemDC;
	BITMAP bmp;
	POINT ptSize, ptOrg;

	hMemDC = CreateCompatibleDC(hDC); // creating memory context, that is compatible with hDC
	if (hMemDC == NULL) {
		return FALSE;
	}


	hBmpOld = (HBITMAP)SelectObject(hMemDC, hBitmap); // select image into the context	
													  // this function returns ID of the BMP that was loaded into the memory context earlier
	if (!hBmpOld) {
		return FALSE;
	}

	SetMapMode(hMemDC, GetMapMode(hDC)); // synchronizing of the memory context and showing context
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)& bmp);

	ptSize.x = bmp.bmWidth;
	ptSize.y = bmp.bmHeight;
	DPtoLP(hDC, &ptSize, 1); // convert units into logical units

	ptOrg.x = 0;
	ptOrg.y = 0;
	DPtoLP(hDC, &ptSize, 1);

	TransparentBlt(hDC, x, y, ptSize.x, ptSize.y, hMemDC, ptOrg.x, ptOrg.y, ptSize.x, ptSize.y, RGB(255, 255, 0));

	SelectObject(hMemDC, hBmpOld); // restore memory context
	DeleteObject(hMemDC);
}

