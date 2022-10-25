#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbt.h>
#include "stats.h"
#include "keyboard.h"
#include "utils.h"
#include <map>
#include <fstream>

HDEVNOTIFY ghDeviceNotify;
HANDLE hFileLog;
char* deviceCache = (char*)malloc(256);
char* knownWhitelist = NULL;
std::map<std::string, bool> knownDevices;
bool enrollment = false;

std::map<std::string, bool> readKnownDevices(std::wstring filename) {
	std::ifstream infile(filename);

	std::string line;
	std::map<std::string, bool> result;
	while (std::getline(infile, line))
	{
		MyOutputDebugStringW(L"%S\n", line.data());
		result.insert_or_assign(std::string(line).data(), true);
	}
	return result;
}

std::wstring getConfigPath() {
	wchar_t* path;
	size_t pathLen = 0;
	_wdupenv_s(&path, &pathLen, L"APPDATA");
	if (path == NULL) {
		return L"";
	}

	std::wstring configPath = std::wstring(path) + L"\\USBubble";
	free(path);
	MyOutputDebugStringW(L"Config path: %s\n", configPath.data());

	return configPath;
}

std::string getDeviceID(char* deviceName) {
	char* temp = strstr((char*)deviceName, "#");

	// TODO: guard against edge cases of failed searches
	if (!temp) {
		return "";
	}

	temp = strstr(temp + 1, "#");
	if (!temp) {
		return "";
	}
	// extract unique id
	char* id = strstr(temp, "&");
	if (!id) {
		return "";
	}
	char* idEnd = strstr(id + 1, "&");
	if (idEnd)
		*idEnd = 0;

	return std::string(id);
}

LRESULT CALLBACK Wndproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_DEVICECHANGE: {
		if (wParam != DBT_DEVICEARRIVAL) {
			return ERROR_SUCCESS;
		}
		DEV_BROADCAST_HDR* device = (DEV_BROADCAST_HDR*)lParam;
		if (device->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE) {
			return ERROR_SUCCESS;
		}

		DEV_BROADCAST_DEVICEINTERFACE_A* deviceInterface = (DEV_BROADCAST_DEVICEINTERFACE_A*)device;
		char* deviceName = deviceInterface->dbcc_name;
		std::string deviceID = getDeviceID(deviceName);
		MyOutputDebugStringW(L"Temp: %S\n", deviceName);
		MyOutputDebugStringW(L"ID: %S\n", deviceID);

		// already hooked, can safely return
		if (knownDevices.find(deviceID) != knownDevices.end()) {
			MyOutputDebugStringW(L"Known devices located\n");
			break;
		}

		// iterate all devices
		UINT numDevices = 0;
		GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST));
		char cNum[5];
		// ++ to prevent weird bug where program starts with a device plugged in and unplugs it causing
		// a weird misreporting of device count, which might lead to lesser than required space being allocated
		RAWINPUTDEVICELIST* ridList = (RAWINPUTDEVICELIST*)malloc(++numDevices * sizeof(RAWINPUTDEVICELIST));
		GetRawInputDeviceList(ridList, &numDevices, sizeof(RAWINPUTDEVICELIST));
		//char pData[256];
		void* pData = malloc(256);
		for (int i = 0; i < numDevices; i++) {
			UINT size = 256;
			GetRawInputDeviceInfoA(ridList[i].hDevice, RIDI_DEVICENAME, pData, &size);

			std::string curDeviceID = getDeviceID((char *)pData);

			//WriteFile(hFileLog, (char*)pData, strlen((char*)pData), NULL, NULL);
			WriteFile(hFileLog, deviceID.data(), deviceID.size(), NULL, NULL);
			WriteFile(hFileLog, "\n", 1, NULL, NULL);
			//WriteFile(hFileLog, deviceName, strlen(deviceName), NULL, NULL);
			WriteFile(hFileLog, curDeviceID.data(), curDeviceID.size(), NULL, NULL);
			WriteFile(hFileLog, "\n", 1, NULL, NULL);

			if (deviceID.compare(curDeviceID) != 0) {
				continue;
			}

			// verify keyboard status only if name matches
			RID_DEVICE_INFO deviceInfo;
			size = sizeof(deviceInfo);
			GetRawInputDeviceInfoA(ridList[i].hDevice, RIDI_DEVICEINFO, &deviceInfo, &size);
			if (!(deviceInfo.dwType == RIM_TYPEKEYBOARD || deviceInfo.dwType == RIM_TYPEHID)) {
				continue;
			}

			WriteFile(hFileLog, "New HID Device Detected\n\n", 25, NULL, NULL);
			// can call ReadFile over and over again in enrollment mode as speed not a concern
			// Add to known devices
			knownDevices.insert_or_assign(deviceID, true);
			if (enrollment) {
				// Write to configuration file
				std::wstring configPath = getConfigPath();
				// ensure directory exists
				CreateDirectoryW(configPath.c_str(), NULL);
				configPath = configPath + L"\\config";

				std::ofstream outfile;
				outfile.open(configPath, std::ios_base::app); // append instead of overwrite
				outfile << deviceID << std::endl;
				MessageBoxA(NULL, "Successfully Enrolled HID!", "Success", MB_OK);
				break;
			}

			// For new device
			// hook keyboard here
			setTotalCharCount(0);

			// These two threads have internal mutex to ensure that it is only called once
			std::thread kbHookThread(keyboardHook);
			kbHookThread.detach();
			std::thread pollSwitchThread(pollKillSwitch);
			pollSwitchThread.detach();
			break;
		}
		free(pData);
		break;
	}
	case WM_CREATE:
	{
		//GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72,0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };
		//GUID WceusbshGUID = { 0x745a17a0, 0x74d3, 0x11d0,0xb6fe,0x00,0xa0,0xc9,0x0f,0x57,0xda };
		GUID WceusbshGUID = { 0x4d1e55b2, 0xf16f, 0x11cf, { 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };
		DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

		ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
		NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		NotificationFilter.dbcc_classguid = WceusbshGUID;

		ghDeviceNotify = RegisterDeviceNotificationA(
			hWnd,                       // events recipient
			&NotificationFilter,        // type of device
			DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
		);
		if (!ghDeviceNotify) {
			WriteFile(hFileLog, "Error in RegisterDeviceNotificationA\n", 38, NULL, NULL);
		}
		break;
	}
	default:
		return DefWindowProcA(hWnd, Msg, wParam, lParam);
	}
	return ERROR_SUCCESS;
}

INT wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
	// Enrollment mode
	if (wcscmp(lpCmdLine, L"enroll") == 0) {
		enrollment = true;
	}

	wchar_t curDir[1000];
	GetCurrentDirectoryW(1000, curDir);
	MyOutputDebugStringW(L"Current directory: %s\n", curDir);

	// Populate whitelist
	std::wstring configPath = getConfigPath();

	// Ensure directory exists
	BOOL success = CreateDirectoryW(configPath.data(), NULL);
	if (!success && GetLastError() != ERROR_ALREADY_EXISTS) {
		MyOutputDebugStringW(L"CreateDirectoryW: %X\n", GetLastError());
		return 1;
	}
	configPath = configPath + L"\\config";
	knownDevices = readKnownDevices(configPath);
	for (auto const& a : knownDevices) {
		MyOutputDebugStringW(L"%S: %d\n", a.first.data(), a.second);
	}

	// USB Connection Hook
	WNDCLASSEXA WndClass;
	RtlZeroMemory(&WndClass, sizeof(WndClass));
	WndClass.cbSize = sizeof(WNDCLASSEXA);
	WndClass.lpfnWndProc = Wndproc;
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = "temptemp";
	hFileLog = CreateFileA("output.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!RegisterClassExA(&WndClass)) {
		const char* data = "Error in RegisterClassExA\n";
		WriteFile(hFileLog, data, strlen(data), NULL, NULL);
		MyOutputDebugStringW(L"RegisterClassExA: %X\n", GetLastError());
	}
	HANDLE WindowHandle = CreateWindowExA(0, "temptemp", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	MSG Msg;
	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return 0;
}

