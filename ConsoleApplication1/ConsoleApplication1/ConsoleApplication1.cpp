#include <Windows.h>
#include <stdio.h>
#include <dbt.h>

HDEVNOTIFY ghDeviceNotify;
HANDLE hFileLog;
LRESULT CALLBACK Wndproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
    case WM_DEVICECHANGE: {
        if (wParam == DBT_DEVICEARRIVAL) {
            DEV_BROADCAST_HDR *device = (DEV_BROADCAST_HDR*)lParam;
            if (device->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                DEV_BROADCAST_DEVICEINTERFACE_A* deviceInterface = (DEV_BROADCAST_DEVICEINTERFACE_A*)device;
                char* deviceName = deviceInterface->dbcc_name;
				char* index = strrchr((char *)deviceName, '#');
				if(index != NULL) {
					*index = 0;
				}
                // iterate all devices
                UINT numDevices = 0;
                GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST));
                char cNum[5];
                RAWINPUTDEVICELIST* ridList = (RAWINPUTDEVICELIST*)malloc(numDevices * sizeof(RAWINPUTDEVICELIST));
                GetRawInputDeviceList(ridList, &numDevices, sizeof(RAWINPUTDEVICELIST));
				//char pData[256];
                void* pData = malloc(256);
                for (int i = 0; i < numDevices; i++) {
                    UINT size = 256;
					GetRawInputDeviceInfoA(ridList[i].hDevice, RIDI_DEVICENAME, pData, &size);
					*(strrchr((char*)pData, '#')) = 0;
					WriteFile(hFileLog, (char*)pData, strlen((char*)pData), NULL, NULL);
					WriteFile(hFileLog, "\n", 1, NULL, NULL);
					WriteFile(hFileLog, deviceName, strlen(deviceName), NULL, NULL);
					WriteFile(hFileLog, "\n",1, NULL, NULL);
					if (strcmp(deviceName, (char*)pData) == 0) {
						// verify keyboard status only if name matches
						RID_DEVICE_INFO deviceInfo;
						size = sizeof(deviceInfo);
						GetRawInputDeviceInfoA(ridList[i].hDevice, RIDI_DEVICEINFO, &deviceInfo, &size);
						if (deviceInfo.dwType == RIM_TYPEKEYBOARD || deviceInfo.dwType == RIM_TYPEHID) {
							WriteFile(hFileLog, "New HID Device Detected\n\n", 25, NULL, NULL);
						}
					}
				}
				free(pData);
			}
		}
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

		//UINT numDevices = 0;
		//GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST));
		//char cNum[5];
		//WriteFile(_itoa_s(numDevices, cNum, 5, 10));
		//RAWINPUTDEVICELIST* ridList = malloc(numDevices * sizeof(RAWINPUTDEVICELIST));
		//GetRawInputDeviceList(ridList, &numDevices, sizeof(RAWINPUTDEVICELIST));
		//int hooked = 0;
		//for (int i = 0; i < numDevices; i++) {
		//  UINT size;
		//  GetRawInputDeviceInfoA(ridList[i].hDevice, RIDI_DEVICENAME, NULL, &size);
		//  void* pData = malloc(size * sizeof(char));
		//  GetRawInputDeviceInfoA(ridList[i].hDevice, RIDI_DEVICENAME, pData, &size);
		//  WriteFile(pData);
		//  free(pData);
		//  RID_DEVICE_INFO deviceInfo;
		//  size = sizeof(deviceInfo);
		//  // hook all keyboards
		//  GetRawInputDeviceInfoA(ridList[i].hDevice, RIDI_DEVICEINFO, &deviceInfo, &size);
		//  if (deviceInfo.dwType == RIM_TYPEKEYBOARD && !hooked) {
		  //RAWINPUTDEVICE rid[1];
		  //rid[0].hwndTarget = hWnd;
		  //rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		  //rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
		  //rid[0].dwFlags = RIDEV_DEVNOTIFY;
		  //if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE))) {
		  //  WriteFile("failed at RegisterRawIputDevices");
		  //}
		//    WriteFile("keyboard detected!");
		//    hooked = 1;
		//  }
		//}
		break;
	}
	default:
		return DefWindowProcA(hWnd, Msg, wParam, lParam);
	}
	return ERROR_SUCCESS;
}

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd) {
	WNDCLASSEXA WndClass;
	RtlZeroMemory(&WndClass, sizeof(WndClass));
	WndClass.cbSize = sizeof(WNDCLASSEXA);
	WndClass.lpfnWndProc = Wndproc;
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = "temptemp";
	hFileLog = CreateFileA("output.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!RegisterClassExA(&WndClass)) {
		WriteFile(hFileLog, "Error in RegisterClassExA\n", 38, NULL, NULL);
		char error[16];
		OutputDebugStringA((LPCSTR)_itoa_s(GetLastError(), error, 16, 16));
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

