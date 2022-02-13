#include"wincontrol.hpp"
#include<windows.h>
HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

namespace win_control{
    void setColor(Color fColor, Color bColor)
    {
        SetConsoleTextAttribute(hOut, short(fColor) | short(bColor) << 4);
    }
    void goxy(short x, short y)
    {
        COORD pos = {y, x};
        SetConsoleCursorPosition(hOut, pos);
    }
    void hideCursor()
    {
        CONSOLE_CURSOR_INFO cci;
        GetConsoleCursorInfo(hOut, &cci);
        cci.bVisible = false;
        SetConsoleCursorInfo(hOut, &cci);
    }
    void consoleInit()
    {
        system("mode con cols=81 lines=40");
        SetConsoleCP(65001);
        SetConsoleTitle(L"Realtime Network Demo");
        hideCursor();
        setCtrlHandler();
    }
    void sleep(int tm)
    {
        Sleep(tm);
    }

    void setTitle(const wchar_t* title){
        SetConsoleTitle(title);
    }
    void setCtrlHandler(){
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
    }
    
    BOOL CtrlHandler(DWORD fdwCtrlType){
        switch (fdwCtrlType)
        {
            /* handle the CTRL-C signal */
        case CTRL_C_EVENT:
            sendQuitMessage();
            return TRUE;
            /* handle the CTRL-BREAK signal */
        case CTRL_BREAK_EVENT:
            sendQuitMessage();
            return TRUE;
    
            /* handle the CTRL-CLOSE signal */
        case CTRL_CLOSE_EVENT:sendQuitMessage();

            return TRUE;
    
            /* handle the CTRL-LOGOFF signal */
        case CTRL_LOGOFF_EVENT:sendQuitMessage();
            return TRUE;
    
            /* handle the CTRL-SHUTDOWN signal */
        case CTRL_SHUTDOWN_EVENT:sendQuitMessage();

            return TRUE;
    
        default:
            return FALSE;
        }
    }

    namespace input_record
    {
        CONSOLE_SCREEN_BUFFER_INFO bInfo;
        INPUT_RECORD ms_rec;
        DWORD ms_res;
        void getInput()
        {
            ReadConsoleInput(hIn, &ms_rec, 1, &ms_res);
            if (ms_rec.EventType == KEY_EVENT)
            {
                if (ms_rec.Event.KeyEvent.bKeyDown)
                {
                    win_control::input_record::keyHandler(ms_rec.Event.KeyEvent.wVirtualKeyCode);
                }
            }
        }
    }
}