#include<windows.h>
namespace win_control{
    enum class Color: short
	{
		c_BLACK = 0x0,
		c_DARKBLUE = 0x1,
		c_DBLUE = 0x1,
		c_DARKGREEN = 0x2,
		c_DGREEN = 0x2,
		c_SKYBLUE = 0x3,
		c_DARKRED = 0x4,
		c_DRED = 0x4,
		c_DARKPURPLE = 0x5,
		c_DPURPLE = 0x5,
		c_YELLOW = 0x6,
		c_GREY = 0x7,
		c_DARKGREY = 0x8,
		c_DGREY = 0x8,
		c_LIGHTBLUE = 0x9,
		c_LBLUE = 0x9,
		c_LIGHTGREEN = 0xa,
		c_LGREEN = 0xa,
		c_LIME = 0xb,
		c_RED = 0xc,
		c_PURPLE = 0xd,
		c_LIGHTYELLOW = 0xe,
		c_LYELLOW = 0xe,
		c_WHITE = 0xf
	};
    BOOL CtrlHandler(DWORD fdwCtrlType);
    void setColor(Color, Color);
	void goxy(short, short);

	void hideCursor();

	void consoleInit();

	void cls();

	void sleep(int);

	void setTitle(const wchar_t*);
    void setCtrlHandler();
    void sendQuitMessage();
    namespace input_record
	{
		void keyHandler(int);
		void getInput();
	}
}