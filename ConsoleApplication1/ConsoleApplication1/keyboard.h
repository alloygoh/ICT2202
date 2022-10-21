#ifndef KEYBOARD_H
#define KEYBOARD_H

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
char formatKey(char);

void replayStoredKeystrokes();
void keyboardHook();

extern HHOOK ghHook;
extern KBDLLHOOKSTRUCT kbdStruct;
extern std::mutex keyboardHookMutex;
extern std::string kbBuffer;
extern std::vector<INPUT> vInputs;
extern std::map<char, char> layeredKeys;
extern std::map<int, std::string> mapSpecialKeys;
extern bool ALLOW_INPUT;
extern bool INPUT_BELOW_THRESHOLD;

#endif
