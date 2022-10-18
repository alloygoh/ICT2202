#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Windows.h>
#include <psapi.h>
#include <ctime>
#include <thread>
#include <vector>

LRESULT __stdcall hookCallback(int, WPARAM, LPARAM);
bool releaseHook();
bool setHook();
void replayStoredKeystrokes();
void keyboardHook();

extern HHOOK ghHook;
extern KBDLLHOOKSTRUCT kbdStruct;
extern std::vector<INPUT> vInputs;
extern bool ALLOW_INPUT;
extern bool INPUT_BELOW_THRESHOLD;

#endif
