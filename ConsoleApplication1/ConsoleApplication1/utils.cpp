#include "utils.h"

#define BUFSIZE 512
#define KILL_ENDPOINT L"/api/can-kms"
#define USER_AGENT L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/95.0.4638.54 Safari/537.36"

/* These should be defined in GCC compilation options */
/* #define SERVER_NAME L"localhost" */
/* #define SERVER_PORT 5000 */

#define SERVER_NAME L"df.d0minik.me"
#define SERVER_PORT 5000


std::wstring key;

bool checkRCDOKey() {
    key = getEnvVar(L"RCDO_KEY", L"");
    return !key.empty();
}

std::wstring getEnvVar(const wchar_t* envVarName, const wchar_t* defaultValue) {
    std::wstring returnVal(defaultValue);

    size_t requiredSize;
    _wgetenv_s(&requiredSize, NULL, 0, envVarName);
    if (requiredSize == 0) {
        return returnVal;
    }

    wchar_t* buffer = (wchar_t*)malloc(requiredSize * sizeof(wchar_t));
    if (buffer == NULL) {
        printf("Failed to allocate memory!\n");
        return returnVal;
    }

    _wgetenv_s(&requiredSize, buffer, requiredSize, envVarName);

    returnVal = buffer;
    free(buffer);
    return returnVal;
}

/* Returns:
 *      true: success
 *      false: failure
 */
void addKeyToRequestBody(std::map<std::wstring, std::wstring>*& input) {
    if (input == NULL) {
        input = new std::map<std::wstring, std::wstring>();
    }

    if (!checkRCDOKey()) {
        return;
    }
    (*input)[L"key"] = key;
}

#include <sstream>
#include <iomanip>

std::string escape_json(const std::wstring input) {
    std::string s(input.begin(), input.end());

    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
            o << "\\u"
                << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
        }
        else {
            o << *c;
        }
    }
    return o.str();
}

std::string mapToJsonString(std::map<std::wstring, std::wstring> input) {
    std::map<std::wstring, std::wstring>::iterator it;

    std::string output = "{";
    for (it = input.begin(); it != input.end(); it++) {
        output += "\"" + escape_json(it->first) + "\":";
        output += "\"" + escape_json(it->second) + "\",";
    }
    output.pop_back();
    output += "}";
    return output;
}

/*
 * Notifies the web server that a breach has occured
 */
int notify(std::wstring data, std::wstring type) {
    std::wstring notifyEndpoint = L"/api/report/notify";

    std::map<std::wstring, std::wstring> requestBody;
    requestBody[L"message"] = data;
    requestBody[L"type"] = type;
    std::string out = sendRequest(L"POST", notifyEndpoint, &requestBody);
    return out == "true";
}

/*
 * Examples
 * verb (string): GET/POST
 * endpoint (string): /api/something
 * requestBody (JSON string): {"key": "msg"}
 */
std::string sendRequest(std::wstring verb, std::wstring endpoint,
    std::map<std::wstring, std::wstring>* requestBody) {
    // Setup
    addKeyToRequestBody(requestBody);
    std::string szRequestBody = mapToJsonString(*requestBody);

    HINTERNET hInternet = InternetOpenW(USER_AGENT,
        INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    HINTERNET hConnect = InternetConnectW(hInternet, SERVER_NAME,
        SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    // GET Request
    HINTERNET hHttpFile = HttpOpenRequestW(hConnect, verb.c_str(), endpoint.c_str(),
        NULL, NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_CACHE_WRITE, 0);

    std::wstring headers = L"Content-Type: application/json";
    if (!HttpSendRequestW(hHttpFile, headers.c_str(), headers.size(),
        (LPVOID)szRequestBody.c_str(), szRequestBody.size())) {
        std::wcout << L"HttpSendRequest Failed" << std::endl;
        std::wcout << GetLastError() << std::endl;
        return "";
    }

    DWORD contentlen = 0;
    DWORD size_f = sizeof(contentlen);
    if (!(HttpQueryInfoW(hHttpFile,
        HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentlen,
        &size_f, NULL) && contentlen > 0)) {
        std::wcout << L"HttpQueryInfo Failed" << std::endl;
        std::wcout << GetLastError() << std::endl;
        return "";
    }

    BYTE* data = (BYTE*)malloc(contentlen + 1);
    BYTE* currentPtr = data;
    DWORD copied = 0;
    for (;;) {
        BYTE dataArr[BUFSIZE];
        DWORD dwBytesRead;
        BOOL bRead;
        bRead = InternetReadFile(hHttpFile, dataArr, BUFSIZE, &dwBytesRead);
        if (dwBytesRead == 0) {
            data[contentlen] = '\x00';
            break;
        }
        else if (dwBytesRead < BUFSIZE) {
            dataArr[dwBytesRead] = '\x00';
        }
        if (!bRead)
            std::wcout << L"InternetReadFile Failed" << std::endl;
        else {
            memcpy_s(currentPtr, contentlen - copied, dataArr, dwBytesRead);
            currentPtr += dwBytesRead;
            copied += dwBytesRead;
        }
    }
    std::string content((char*)data);
    std::wcout << L"[+] Endpoint: " << endpoint << std::endl;
    std::wcout << L"[+] Content-Length: " << std::to_wstring(copied) << std::endl;
    std::cout << "[+] Content: " << content << std::endl;

    // process stuff here

    free(data);
    InternetCloseHandle(hInternet);
    InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    return content;
}

void pollKillSwitch() {
    // Perform an initial ping first
    if (!notify(L"The program has started successfully")) {
        return;
    }

    for (;;) {
        std::string content = sendRequest(L"POST", KILL_ENDPOINT, NULL);
        /* if(content.empty() && isFirstLoop){ */
        /*     Sleep(5000); */
        /*     return; */
        /* } */

        if (content.compare(0, 4, "true") == 0) {
            ExitProcess(0);
        }

        Sleep(200);
    }
}
