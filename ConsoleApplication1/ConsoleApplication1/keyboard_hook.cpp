#include "header.h"

HHOOK ghHook;
KBDLLHOOKSTRUCT kbdStruct;
std::vector<INPUT> inputs;

bool setHook(){

    // set the keyboard hook for all processes on the computer
    // runs the hookCallback function when hooked is triggered
    if (!(ghHook = SetWindowsHookExW(WH_KEYBOARD_LL, hookCallback, NULL, 0))){
        wprintf(L"%s: %d\n", "Hooked failed to install with error code", GetLastError());
        return 1;
    }
    else {
        wprintf(L"Hook successfully installed!\n");
    }

    return 0;
}

bool releaseHook(){

    return UnhookWindowsHookEx(ghHook);
}

LRESULT __stdcall hookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    // function handler for all keyboard strokes

    // If nCode is less than zero, the hook procedure must pass the
    // message to the CallNextHookEx function without further processing
    // and should return the value returned by CallNextHookEx.
    if (nCode < 0) {
        return CallNextHookEx(ghHook, nCode, wParam, lParam);
    }

    kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
    int vkCode = kbdStruct.vkCode;

    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = kbdStruct.vkCode;

    printf("%d\n", wParam);
    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP || WM_IME_KEYUP)
        input.ki.dwFlags = KEYEVENTF_KEYUP;

    inputs.push_back(input);

	return 0;
}

void start() {

    setHook();
	MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) != 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int main() {
    std::thread keyboardMonitor(start);
    keyboardMonitor.detach();
    Sleep(3000);
    releaseHook();

    UINT cInputs = inputs.size();
    LPINPUT pInputs = (LPINPUT)malloc(sizeof(INPUT) * cInputs);
    int i = 0;
    for (INPUT input : inputs) {
        printf("%d\n", input.ki.wVk);
        pInputs[i] = input;
    }

    int res = SendInput(cInputs, pInputs, sizeof(INPUT));
    printf("%d\n", res);
    return 0;
}
