#ifndef KEYBOARD_H
#define KEYBOARD_H
#define MODEL_HOST L"127.0.0.1"
#define MODEL_PORT 8000
#define MODEL_ENDPOINT L"/predict"

#include <Windows.h>
#include <psapi.h>
#include <ctime>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

LRESULT __stdcall hookCallback(int, WPARAM, LPARAM);
bool releaseHook();
bool setHook();
int setModKeyState(int, int);
std::wstring formatKey(wchar_t);
bool predictWithModel(std::wstring);
void replayStoredKeystrokes();
void keyboardHook();

extern HHOOK ghHook;
extern KBDLLHOOKSTRUCT kbdStruct;
extern std::mutex keyboardHookMutex;
extern std::wstring kbBuffer;
extern std::vector<INPUT> vInputs;
extern std::map<wchar_t, wchar_t> layeredKeys;
extern std::map<int, std::wstring> mapSpecialKeys;
extern std::vector<std::wstring> monitoredWindows;

#endif
