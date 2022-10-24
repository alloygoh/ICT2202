/*
 * [KEYBOARD HOOK]
 * Recommended usage is to start the keyboard hook for however long is a separate thread, then call releaseHook from the main thread when the keyboard inputs are sanitised. See example below:

 * int main() {
 *    std::thread t1(keyboardHook);
 *    t1.detach();
 *    Sleep(10000);
 *    releaseHook();
 *    replayStoredKeystrokes();

 *    return 0;
 * }
 */

#include "keyboard.h"
#include "stats.h"
#include "utils.h"

// "Global variables"
std::mutex maliciousIndicatorsMutex;
std::mutex hasNewDeviceMutex;
bool hasNewDevice = true;
// keeps track of the different checks performed
// whether input should be allowed is an OR operation between all elements
std::map<std::wstring, bool> maliciousIndicators = {
	{L"SD", 0},
	{L"MODEL", 0},
	{L"AMSI", 0},
};

/* Start of thread-safe functions */
// Returns the value of hasNewDevice (thread safe)
bool getHasNewDevice() {
	hasNewDeviceMutex.lock();
	bool result = hasNewDevice;
	hasNewDeviceMutex.unlock();
	return result;
}

void setHasNewDevice(bool value) {
	hasNewDeviceMutex.lock();
	hasNewDevice = value;
	hasNewDeviceMutex.unlock();
}

bool getMaliciousIndicator(std::wstring key) {
	maliciousIndicatorsMutex.lock();
	bool result = maliciousIndicators.at(key);
	maliciousIndicatorsMutex.unlock();
	return result;
}

std::pair<std::map<std::wstring, bool>::iterator, bool> setMaliciousIndicator(std::wstring key, bool value) {
	maliciousIndicatorsMutex.lock();
	std::pair<std::map<std::wstring, bool>::iterator, bool> result = maliciousIndicators.insert_or_assign(key, value);
	maliciousIndicatorsMutex.unlock();
	return result;
}
/* End of thread-safe functions */

 // maps keys that are hidden behind the SHIFT layer
std::map<wchar_t, wchar_t> layeredKeys = {
	{L'`', L'~'},
	{L'1', L'!'},
	{L'2', L'@'},
	{L'3', L'#'},
	{L'4', L'$'},
	{L'5', L'%'},
	{L'6', L'^'},
	{L'7', L'&'},
	{L'8', L'*'},
	{L'9', L'('},
	{L'0', L')'},
	{L'[', L'{L'},
	{L']', L'}'},
	{L'\\', L'|'},
	{L';', L':'},
	{L'\'', L'\"'},
	{L',', L'<'},
	{L'.', L'>'},
	{L'/', L'?'},
};

// map to track keystrokes that cannot be mapped with MapVirtualKeyExW
std::map<int, std::wstring> mapSpecialKeys = {
	{VK_BACK, L"BACKSPACE"},
	{VK_RETURN, L"\n"},
	{VK_SPACE, L" "},
	{VK_TAB, L"\t"},
	{VK_LWIN, L"WIN"},
	{VK_RWIN, L"WIN"},
	{VK_ESCAPE, L"ESCAPE"},
	{VK_END, L"END"},
	{VK_HOME, L"HOME"},
	{VK_LEFT, L"LEFT"},
	{VK_RIGHT, L"RIGHT"},
	{VK_UP, L"UP"},
	{VK_DOWN, L"DOWN"},
	{VK_PRIOR, L"PG_UP"},
	{VK_NEXT, L"PG_DOWN"},
	{VK_OEM_PERIOD, L"."},
	{VK_DECIMAL, L"."},
	{VK_OEM_PLUS, L"+"},
	{VK_OEM_MINUS, L"-"},
	{VK_ADD, L"+"},
	{VK_SUBTRACT, L"-"},
	{VK_INSERT, L"INSERT"},
	{VK_DELETE, L"DELETE"},
	{VK_PRINT, L"PRINT"},
	{VK_SNAPSHOT, L"PRINTSCREEN"},
	{VK_SCROLL, L"SCROLL"},
	{VK_PAUSE, L"PAUSE"},
	{VK_NUMLOCK, L"NUMLOCK"},
	{VK_F1, L"F1"},
	{VK_F2, L"F2"},
	{VK_F3, L"F3"},
	{VK_F4, L"F4"},
	{VK_F5, L"F5"},
	{VK_F6, L"F6"},
	{VK_F7, L"F7"},
	{VK_F8, L"F8"},
	{VK_F9, L"F9"},
	{VK_F10, L"F10"},
	{VK_F11, L"F11"},
	{VK_F12, L"F12"},
};

// stores booleans for mod keys
// +-------+------+------+
// | SHIFT | CAPS | CASE |
// +-------+------+------+
// |     0 |    0 |    0 |
// |     0 |    1 |    1 |
// |     1 |    0 |    1 |
// |     1 |    1 |    0 |
// +-------+------+------+
std::map<std::wstring, bool> modKeyStates = {
	{L"SHIFT", 0},
	{L"CAPS", 0},
};

// Whitelisted window names to monitor
std::vector<std::wstring> monitoredWindows = { L"cmd.exe", L"powershell.exe" };

std::mutex keyboardHookMutex;
HHOOK ghHook;
KBDLLHOOKSTRUCT kbdStruct;
std::wstring kbBuffer;
std::wstring kbBufferFiltered;
std::vector<INPUT> vInputs;
HAMSICONTEXT ghAMSI;

// flag to toggle whether input is let through
bool NOTIFIED = false;
bool FIRST_CHECK_DONE = false;
int INPUT_WINDOW = 0;

bool setHook() {

	// set the keyboard hook for all processes on the computer
	// runs the hookCallback function when hooked is triggered
	if (!(ghHook = SetWindowsHookExW(WH_KEYBOARD_LL, hookCallback, NULL, 0))) {
		wprintf(L"%s: %d\n", L"Hooked failed to install with error code", GetLastError());
		return 1;
	}
	else {
		wprintf(L"Hook successfully installed!\n");
	}

	return 0;
}

bool releaseHook() {
	return UnhookWindowsHookEx(ghHook);
}


int setModKeyState(int vkCode, int event){
	std::map<int, std::wstring> vkCodeToString = {
		{VK_SHIFT, L"SHIFT"},
		{VK_LSHIFT, L"SHIFT"},
		{VK_RSHIFT, L"SHIFT"},
		{VK_CAPITAL, L"CAPS"},
		{VK_CONTROL, L"CTRL"},
		{VK_LCONTROL, L"CTRL"},
		{VK_RCONTROL, L"CTRL"},
		{VK_MENU, L"ALT"},
		{VK_RMENU, L"ALT"},
		{VK_LMENU, L"ALT"},
		{VK_LWIN, L"WIN"},
		{VK_RWIN, L"WIN"},
	};

	if (vkCodeToString.find(vkCode) == vkCodeToString.end())
		// must log, not a mod key
		return 0;

	std::wstring keyName = vkCodeToString.at(vkCode);

	if (modKeyStates.find(keyName) == modKeyStates.end())
		return 0;

	int isKeyDown = (event == WM_KEYDOWN || event == WM_SYSKEYDOWN);

	if (keyName != L"CAPS"){
		modKeyStates.at(keyName) = isKeyDown;
		return 1;
	}

	modKeyStates.at(keyName) = isKeyDown;

	return 1;
}

std::wstring formatKey(wchar_t key){
	// handles the transforming of keys that are modified using the SHIFT and CapsLock keys

	std::wstring key_string = { key };

	if (isalpha(key)) {
		key_string[0] = (modKeyStates.at(L"SHIFT") ^ modKeyStates.at(L"CAPS")) ? key : tolower(key);
		return key_string;
	}

	if (modKeyStates.at(L"SHIFT")){
		auto layeredKeyEntry = layeredKeys.find(key);
		key_string[0] = (layeredKeyEntry == layeredKeys.end()) ? key : layeredKeyEntry->second;
	}

	return key_string;
}

bool predictWithModel(std::wstring data) {
	std::map<std::wstring, std::wstring> requestBody;
	requestBody[L"content"] = data;
	std::string out = sendRequest(L"POST", MODEL_HOST, MODEL_PORT, MODEL_ENDPOINT, &requestBody);

	return stoi(out);
}

bool amsiDetect(std::wstring data) {
	// if no init-ed, init amsi
	if (!ghAMSI) {
		AmsiInitialize(L"USBubble", &ghAMSI);
	}
	AMSI_RESULT result;
	AmsiScanBuffer(ghAMSI, (PVOID)data.data(), data.length() * 2, L"sample buffer", NULL, &result);
	return AmsiResultIsMalware(result);
}

LRESULT __stdcall hookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
	// function handler for all keyboard strokes

	// If nCode is less than zero, the hook procedure must pass the
	// message to the CallNextHookEx function without further processing
	// and should return the value returned by CallNextHookEx.
	if (nCode < 0) {
		return CallNextHookEx(ghHook, nCode, wParam, lParam);
	}

	// Retrieve name of process that the keystroke is meant for
	HWND hWnd = GetForegroundWindow();
	DWORD dwProcessId;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	WCHAR lpBaseName[MAX_PATH];
	GetModuleBaseNameW(hProc, NULL, lpBaseName, MAX_PATH);

	kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
	int vkCode = kbdStruct.vkCode;

	vInputs.push_back(INPUT());
	int i = vInputs.size() - 1;

	vInputs[i].type = INPUT_KEYBOARD;
	vInputs[i].ki.wVk = vkCode;

	if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
		vInputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
	else
		vInputs[i].ki.dwFlags = 0;

	int isModKey = setModKeyState(vkCode, wParam);

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		if (!isModKey) {
			std::wstring key;
			if (mapSpecialKeys.find(vkCode) != mapSpecialKeys.end()) {
				key = mapSpecialKeys.at(vkCode);
			} else {
				HKL kbLayout = GetKeyboardLayout(GetCurrentProcessId());

				key = formatKey(MapVirtualKeyExW(vkCode, MAPVK_VK_TO_CHAR, kbLayout));
			}
			//kbBuffer += key;  // Uncomment only when want to log all keystrokes

			if (std::find(monitoredWindows.begin(), monitoredWindows.end(), lpBaseName) != monitoredWindows.end()) {
				kbBufferFiltered += key;

				if (vkCode == VK_RETURN) {
					// ML detection
					setMaliciousIndicator(L"MODEL", predictWithModel(kbBufferFiltered));

					// AMSI sig detection
					setMaliciousIndicator(L"AMSI", amsiDetect(kbBufferFiltered));

					std::wstring message = L"The following input were sent to a high-risk application:\n" + kbBufferFiltered;
					std::thread(notify, message, L"file").detach();
				}
			}
		}

		MyOutputDebugStringW(L"[vkcode] %d\n", vkCode);
		DWORD now = kbdStruct.time;
		bool allowInput = calculateTiming(now);

		// Determine whether to release captured inputs once WINDOW_SIZE is reached
		if (INPUT_WINDOW < WINDOW_SIZE) {
			++INPUT_WINDOW;
		} else  if (INPUT_WINDOW == WINDOW_SIZE) {
			++INPUT_WINDOW;
			if (allowInput) {
				releaseHook(); //temporarily unhook in order to properly replay keystrokes
				replayStoredKeystrokes();
				setHook();

				setHasNewDevice(false);
			} //once input is tagged as malicious there is no way to unblock input
			else {
				setMaliciousIndicator(L"SD", true);
			}
		} else if (!maliciousIndicators.at(L"SD")) {
			setMaliciousIndicator(L"SD", int(!allowInput));
		}
	}

	if (getHasNewDevice()) {
		// If the device is new, continue monitoring, and don't allow
		return -1;
	}

	// Returns true if any item in the map is true
	maliciousIndicatorsMutex.lock();
	std::wstring temp = L"[malicious indicators]\n";
	for (auto const& it : maliciousIndicators) {
		temp += it.first + L" " + std::to_wstring(it.second) + L"\n";
	}
	MyOutputDebugStringW(temp.c_str());
	
	bool isMaliciousInput = std::any_of(maliciousIndicators.begin(), maliciousIndicators.end(), [](const auto& p) { return p.second; });
	maliciousIndicatorsMutex.unlock();

	if (isMaliciousInput) {
		if (!NOTIFIED) {
			std::thread(notify, L"A possible HID injection attack has been detected!", L"text").detach();
			NOTIFIED = true;
		}

		return -1;
	}

	return 0;
}

void replayStoredKeystrokes() {
	UINT cInputs = vInputs.size();
	for (int i = 0; i < cInputs; ++i) {
		INPUT input = vInputs[i];
		INPUT pInputs[1] = { input };

		int res = SendInput(1, pInputs, sizeof(INPUT));

		if ((input.ki.wVk == VK_LWIN || input.ki.wVk == VK_RWIN) && input.ki.dwFlags == KEYEVENTF_KEYUP) {
			Sleep(100);
		}
	}

	vInputs.clear();
}

void keyboardHook() {
	if (keyboardHookMutex.try_lock()) {
		setHook();
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
