#include <iostream>
#include <Windows.h>
#include <algorithm>
#include <random>
#include <thread>
#include <queue>
#include <string>

// Running a timer on a thread
static int s_nSeconds = 0;
static bool s_bFinished = false;
static bool s_bCount = false;
void Time()
{
	using namespace std::literals::chrono_literals;

	while (!s_bFinished)
	{
		if (s_bCount)
		{	// Only start counting after the first lmb click
			std::this_thread::sleep_for(1s);
			s_nSeconds++;
		}
	}
}

enum ATTRIBUTES
{
	TOP_BORDER = 0x0400,
	BOTTOM_BORDER = 0x8000,
	LEFT_BORDER = 0x0800,
	RIGHT_BORDER = 0x1000,
	FULL_BORDER = TOP_BORDER | BOTTOM_BORDER | LEFT_BORDER | RIGHT_BORDER
};

enum COLORS
{
	FG_BLACK = 0x0000,
	FG_GREY = 0x0007,
	FG_DARK_GREY = 0x0008,
	FG_WHITE = 0x000F,
	FG_BLUE = 0x0009,
	FG_GREEN = 0x000A,
	FG_DARK_YELLOW = 0x0006,
	FG_RED = 0x000C,
	FG_YELLOW = 0x000E,
	FG_MAGENTA = 0x000D,
	FG_DARK_BLUE = 0x0001,
	FG_DARK_RED = 0x0004,
	FG_CYAN = 0x000B,
	BG_BLACK = 0x0000,
	BG_YELLOW = 0x00E0,
	BG_GREY = 0x0070,
	BG_DARK_YELLOW = 0x0060,
	BG_DARK_GREY = 0x0080,
	BG_WHITE = 0x00F0
};

class Minesweeper
{
public:
	Minesweeper(int width, int height, int bombs)
		:
		width(width),
		height(height),
		bombs(bombs)
	{
		playField = new short[width * height] { 0 };
		mask = new short[width * height] { 0 };
	}

	~Minesweeper()
	{
		delete[] playField;
		delete[] mask;
	}

private:
	int width;
	int height;
	short bombs;
	short* playField;
	short* mask;
	bool generated = false;
	bool failed = false;

	std::pair<int, int> directions[8] = { {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}, {0,1} };
	bool isInside(int x, int y) { return 0 <= x && x < width && 0 <= y && y < height; }
	void fail()
	{
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				if (playField[y * width + x] == 9 && mask[y * width + x] != 2)
					mask[y * width + x] = 1;
				else if (playField[y * width + x] != 9 && mask[y * width + x] == 2)
					mask[y * width + x] = 3;

		s_bCount = false;
		failed = true;
	}
	void BFS(int x, int y)
	{
		std::queue<std::pair<int, int> > q;
		q.push({ x, y });
		mask[y * width + x] = 1;

		while (!q.empty())
		{
			int currX = q.front().first;
			int currY = q.front().second;
			q.pop();

			if (playField[currY * width + currX] != 0)
				continue;

			for (auto dir : directions)
			{
				int newX = currX + dir.first;
				int newY = currY + dir.second;

				if (isInside(newX, newY) && mask[newY * width + newX] == 0)
				{
					q.push({ newX, newY });
					mask[newY * width + newX] = 1;
				}
			}
		}
	}
	void numberField()
	{
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				if (playField[y * width + x] != 9)
				{
					int count = 0;
					for (auto dir : directions)
					{
						int newX = x + dir.first;
						int newY = y + dir.second;

						if (isInside(newX, newY) && playField[newY * width + newX] == 9)
							count++;
					}

					playField[y * width + x] = count;
				}
			}
		}
	}
	void firstClickGeneration(int x, int y)
	{
		// The way to create the minefield is very interesting
		// Add the amount of bombs you'll play with to the start of the playfield and then randomly shuffle it
		// Keep shuffling until you reach a configuration where the cell of the first lmb is empty
		std::fill(playField, playField + bombs, 9);
		do
		{
			unsigned int seed = unsigned int(std::time(0));
			std::srand(seed);
			std::shuffle(playField, playField + width * height, std::default_random_engine(rand()));
			numberField();
		} while (playField[y * width + x] != 0);

		s_bCount = true;
		generated = true;
	}

public:
	void Render(CHAR_INFO* output)
	{
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				switch (mask[y * width + x]) // Render through the mask of the playfield
				{
				case 0: // Unopened cell
					output[y * width + x].Char.UnicodeChar = 0x2591;
					output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_GREY;
					break;
				case 1: // Opened cell
					switch (playField[y * width + x]) // Number cells and bomb cells
					{
					case 0:
						output[y * width + x].Char.UnicodeChar = L' ';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_DARK_GREY;
						break;
					case 1:
						output[y * width + x].Char.UnicodeChar = L'1';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_BLUE;
						break;
					case 2:
						output[y * width + x].Char.UnicodeChar = L'2';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_GREEN;
						break;
					case 3:
						output[y * width + x].Char.UnicodeChar = L'3';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_RED;
						break;
					case 4:
						output[y * width + x].Char.UnicodeChar = L'4';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_DARK_BLUE;
						break;
					case 5:
						output[y * width + x].Char.UnicodeChar = L'5';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_DARK_RED;
						break;
					case 6:
						output[y * width + x].Char.UnicodeChar = L'6';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_CYAN;
						break;
					case 7:
						output[y * width + x].Char.UnicodeChar = L'7';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_MAGENTA;
						break;
					case 8:
						output[y * width + x].Char.UnicodeChar = L'8';
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_DARK_GREY;
						break;
					case 9: // Bomb cell
						output[y * width + x].Char.UnicodeChar = 0x06DE;
						output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_WHITE;
						break;
					}
					break;
				case 2: // Flagged cells
					output[y * width + x].Char.UnicodeChar = 0x2592;
					output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_DARK_YELLOW;
					break;
				case 3: // Incorrectly marked flagged cell after failing
					output[y * width + x].Char.UnicodeChar = 0x2592;
					output[y * width + x].Attributes = FULL_BORDER | BG_BLACK | FG_RED;
					break;
				}
			}
		}
	}
	void ProcessLMB(int x, int y)
	{
		// Generate the field around the first mouse click so that you always get an empty cell
		if (!generated)
			firstClickGeneration(x, y);

		/* Most expansive function, lots of shit happens here
		*
		* Rules are:
		* Click on a closed cell containing nothing -> Open whole area containing empty cells
		* Click on a closed cell containing a number -> Only open that one cell
		* Click on a closed cell containing a bomb -> Open all bombs and mark fail
		* Click on open number cell with neighbouring bombs flagged correctly
		*			-> If a neighbouring cell happens to be empty, then open whole area of empty cells
		*			-> If a neighbouring cell is a number cell, open it
		*			-> If a neighbouring cell is a flagged cell, leave it
		* Click on open number cell with neighbouring bombs flagged incorrectly -> Open all bombs and mark fail
		*/

		if (mask[y * width + x] == 0)
		{	// Closed cell
			if (playField[y * width + x] == 0)
			{	// BFS on closed empty cell
				BFS(x, y);
			}
			else if (playField[y * width + x] == 9)
			{	// FAIL on closed bomb cell
				fail();
			}
			else
			{	// OPEN on closed number cell
				mask[y * width + x] = 1;
			}
		}
		else if (mask[y * width + x] == 1)
		{	// Opened cell
			if (playField[y * width + x] > 0 && playField[y * width + x] < 9)
			{	// Opened number cell
				int neighbouringCorrectFlags = 0;
				int neighbouringFlags = 0;
				for (auto dir : directions)
				{
					int newX = x + dir.first;
					int newY = y + dir.second;

					if (isInside(newX, newY))
					{
						if (mask[newY * width + newX] == 2)
						{
							neighbouringFlags++;
							if (playField[newY * width + newX] == 9)
								neighbouringCorrectFlags++;
						}
					}
				}

				if (neighbouringCorrectFlags == playField[y * width + x] && neighbouringCorrectFlags == neighbouringFlags)
				{	// Mines flagged correctly
					for (auto dir : directions)
					{
						int newX = x + dir.first;
						int newY = y + dir.second;

						if (isInside(newX, newY))
						{
							if (playField[newY * width + newX] == 0)
								BFS(newX, newY);
							else if (playField[newY * width + newX] != 9)
							{
								mask[newY * width + newX] = 1;
							}
						}
					}
				}
				else if (neighbouringFlags >= playField[y * width + x])
				{	// Mines flagged incorrectly
					fail();
				}
			}
		}
	}
	void ProcessRMB(int x, int y)
	{
		// Just toggle flags
		if (mask[y * width + x] != 1)
			mask[y * width + x] = (mask[y * width + x]) ? 0 : 2;
	}
	bool HasFailed() { return failed; }
	bool HasWon()
	{
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
				if (playField[y * width + x] != 9 && mask[y * width + x] == 0)
					return false;

		s_bCount = false;
		return true;
	}
};

int main()
{
	int nScreenWidth = 0;
	int nScreenHeight = 0;
	int nBombs = 0;
	int nFontW = 32;
	int nFontH = 32;

	wchar_t introTitle[256] = L"Minesweeper Remastered";
	SetConsoleTitle(introTitle);

	// Difficulty prompt
	std::cout << "CHOOSE DIFFICULTY\n";
	std::cout << "1) BEGINNER | Size:9x9 Bombs:10\n";
	std::cout << "2) INTERMEDIATE | Size:16x16 Bombs:40\n";
	std::cout << "3) EXPERT | Size:30x16 Bombs:99\n";
	std::cout << "4) CUSTOM\n";

	std::wstring diff;
	int choice;
	do std::cin >> choice;
	while (choice < 1 || choice > 4);
	switch (choice)
	{
	case 1:
		nScreenWidth = 9;
		nScreenHeight = 9;
		nBombs = 10;
		diff = L"Beginner";
		break;
	case 2:
		nScreenWidth = 16;
		nScreenHeight = 16;
		nBombs = 40;
		diff = L"Intermediate";
		break;
	case 3:
		nScreenWidth = 30;
		nScreenHeight = 16;
		nBombs = 99;
		diff = L"Expert";
		break;
	case 4:
		std::cout << "Width (7-40): ";
		do std::cin >> nScreenWidth;
		while (nScreenWidth < 7 || nScreenWidth > 40);

		std::cout << "Height (7-20): ";
		do std::cin >> nScreenHeight;
		while (nScreenHeight < 7 || nScreenHeight > 20);

		std::cout << "Bombs: ";
		do std::cin >> nBombs;
		while (nBombs < 0 || nBombs > nScreenWidth * nScreenHeight);
		diff = L"Custom";

		break;
	}


	/* START OF SETTING UP CONSOLE */
	CHAR_INFO* bufScreen;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);

	SMALL_RECT rectWindow = { 0,0,1,1 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);
	COORD coord = { short(nScreenWidth), short(nScreenHeight) };
	SetConsoleScreenBufferSize(hConsole, coord);
	SetConsoleActiveScreenBuffer(hConsole);

	// Save original cursor info
	CONSOLE_CURSOR_INFO cciOriginal;
	GetConsoleCursorInfo(hConsole, &cciOriginal);

	CONSOLE_CURSOR_INFO cci;
	cci.dwSize = 1;
	cci.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cci);

	// Save original font info
	CONSOLE_FONT_INFOEX cfiOriginal;
	cfiOriginal.cbSize = sizeof(cfiOriginal);
	GetCurrentConsoleFontEx(hConsole, false, &cfiOriginal);

	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = nFontW;
	cfi.dwFontSize.Y = nFontH;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_BOLD;
	wcscpy_s(cfi.FaceName, L"Courier New");
	SetCurrentConsoleFontEx(hConsole, false, &cfi);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	if (csbi.dwMaximumWindowSize.X < nScreenWidth)
		nScreenWidth = csbi.dwMaximumWindowSize.X;
	if (csbi.dwMaximumWindowSize.Y < nScreenHeight)
		nScreenHeight = csbi.dwMaximumWindowSize.Y;

	coord = { short(nScreenWidth), short(nScreenHeight) };
	SetConsoleScreenBufferSize(hConsole, coord);
	rectWindow = { 0,0, short(nScreenWidth) - 1, short(nScreenHeight) - 1 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);

	SetConsoleMode(hConsoleIn, ENABLE_LVB_GRID_WORLDWIDE | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

	bufScreen = new CHAR_INFO[nScreenWidth * nScreenHeight]{ 0 };
	/* END OF SETTING UP CONSOLE */


	// Initializing game and timer
	Minesweeper game(nScreenWidth, nScreenHeight, nBombs);
	std::thread timer(Time);

	int mouseX = 0;
	int mouseY = 0;
	bool lmbOld = false, rmbOld = false;
	bool lmbNew = false, rmbNew = false;
	bool lmbPressed = false, rmbPressed = false;

	std::wstring endTime;

	int endScreenTimer = 0;

	// MAIN GAME LOOP
	while (true)
	{
		// All of this stuff is really just for the fancy title
		wchar_t s[256];
		std::wstring timeFormat;
		int seconds = s_nSeconds % 60;
		int minutes = s_nSeconds / 60;
		timeFormat += (minutes < 10) ? L'0' + std::to_wstring(minutes) : std::to_wstring(minutes);
		timeFormat += L':';
		timeFormat += (seconds < 10) ? L'0' + std::to_wstring(seconds) : std::to_wstring(seconds);
		swprintf_s(s, 256, L"Time: %s | Difficulty: %s | Size: %dx%d | Bombs: %d", timeFormat.c_str(), diff.c_str(), nScreenWidth, nScreenHeight, nBombs);
		SetConsoleTitle(s);


		/* START OF HANDLING INPUT */
		INPUT_RECORD inBuf[32];
		DWORD events = 0;
		GetNumberOfConsoleInputEvents(hConsoleIn, &events);
		if (events > 0)
			ReadConsoleInput(hConsoleIn, inBuf, events, &events);

		for (DWORD i = 0; i < events; i++)
		{
			switch (inBuf[i].EventType)
			{
			case MOUSE_EVENT:
				switch (inBuf[i].Event.MouseEvent.dwEventFlags)
				{
				case MOUSE_MOVED:
					mouseX = inBuf[i].Event.MouseEvent.dwMousePosition.X;
					mouseY = inBuf[i].Event.MouseEvent.dwMousePosition.Y;
					break;
				case 0:
					lmbNew = (inBuf[i].Event.MouseEvent.dwButtonState & (1 << 0));
					rmbNew = (inBuf[i].Event.MouseEvent.dwButtonState & (1 << 1));
					break;
				}
				break;
			}
		}

		lmbPressed = false;
		rmbPressed = false;
		if (lmbOld != lmbNew && lmbNew)
			lmbPressed = true;
		if (rmbOld != rmbNew && rmbNew)
			rmbPressed = true;
		lmbOld = lmbNew;
		rmbOld = rmbNew;

		if (GetAsyncKeyState(VK_ESCAPE)) // End
			break;
		/* END OF HANDLING INPUT */


		// These 2 if statements are really all there is LMAO
		if (!game.HasFailed())
		{
			if (lmbPressed)
				game.ProcessLMB(mouseX, mouseY);
			if (rmbPressed)
				game.ProcessRMB(mouseX, mouseY);
		}
		else
		{
			endTime = timeFormat;
			endScreenTimer++;
			if (endScreenTimer == 3000)
				break;
		}

		if (game.HasWon())
		{
			endTime = timeFormat;
			break;
		}


		game.Render(bufScreen);
		WriteConsoleOutput(hConsole, bufScreen, { short(nScreenWidth), short(nScreenHeight) }, { 0,0 }, &rectWindow);
	}

	// Clear buffer
	memset(bufScreen, 0, sizeof(CHAR_INFO) * nScreenWidth * nScreenHeight);
	WriteConsoleOutput(hConsole, bufScreen, { short(nScreenWidth), short(nScreenHeight) }, { 0,0 }, &rectWindow);

	// Reset font
	SetCurrentConsoleFontEx(hConsole, false, &cfiOriginal);

	// Reset size
	rectWindow = { 0,0,1,1 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);
	SetConsoleScreenBufferSize(hConsole, { 120, 30 });
	rectWindow = { 0,0,119,29 };
	SetConsoleWindowInfo(hConsole, TRUE, &rectWindow);

	// Reset cursor
	SetConsoleCursorInfo(hConsole, &cciOriginal);
	SetConsoleCursorPosition(hConsole, { 0,0 });

	/* END SCREEN */
	if (game.HasFailed())
		std::wcout << L"Failed lmao you suck\n";
	else if (game.HasWon())
		std::wcout << L"WON gg king!!\n";
	else
		std::wcout << L"Pussy nigger why did you quit\n";

	std::wcout << L"Time: " << endTime << L'\n';
	std::wcout << L"Difficulty: " << diff << L'\n';
	std::wcout << L"Size: " << nScreenWidth << L'x' << nScreenHeight << L'\n';
	std::wcout << L"Bombs: " << nBombs << L'\n';
	/* END SCREEN */

	// Clean up
	s_bFinished = true;
	timer.join();
	delete[] bufScreen;

	system("PAUSE");
}