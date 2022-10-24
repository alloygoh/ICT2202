#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <wininet.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <iterator>
#include <map>

void pollKillSwitch();
std::wstring getEnvVar(const wchar_t* envVarName, const wchar_t* defaultValue);
std::string sendRequest(std::wstring verb, std::wstring server_name, int server_port, std::wstring endpoint,
	std::map<std::wstring, std::wstring>* requestBody);
int notify(std::wstring data, std::wstring type = L"text");
bool checkRCDOKey();
void MyOutputDebugStringW(const wchar_t* fmt, ...);

#endif
