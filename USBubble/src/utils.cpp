#include "keyboard.h"
#include "utils.h"

#include <sstream>
#include <iomanip>

#define BUFSIZE 512
#define KILL_ENDPOINT L"/api/can-kms"
#define UNBLOCK_ENDPOINT L"/api/report/unblock"
#define USER_AGENT L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/95.0.4638.54 Safari/537.36"

#define SERVER_NAME L"df.d0minik.me"
#define SERVER_PORT 5000


std::wstring key;

void MyOutputDebugStringW(const wchar_t* fmt, ...) {
	// TODO: Check if program in debug mode or release mode
	va_list argp;
	va_start(argp, fmt);
	wchar_t dbg_out[4096];
	vswprintf_s(dbg_out, fmt, argp);
	va_end(argp);
	OutputDebugStringW(dbg_out);
}

std::wstring getAPIKey() {
	return getEnvVar(L"USBUBBLE_API_KEY", L"");
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

	std::wstring key = getAPIKey();
	if (key.empty()) {
		return;
	}

	(*input)[L"key"] = key;
}

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
	std::string out = sendRequest(L"POST", SERVER_NAME, SERVER_PORT, notifyEndpoint, &requestBody);
	return out == "true";
}

/*
 * Examples
 * verb (string): GET/POST
 * endpoint (string): /api/something
 * requestBody (map pointer): {"key": "msg"}
 */
std::string sendRequest(std::wstring verb, std::wstring server_name, int server_port, std::wstring endpoint,
	std::map<std::wstring, std::wstring>* requestBody) {
	// Setup
	addKeyToRequestBody(requestBody);
	std::string szRequestBody = mapToJsonString(*requestBody);

	HINTERNET hInternet = InternetOpenW(USER_AGENT,
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	HINTERNET hConnect = InternetConnectW(hInternet, server_name.c_str(),
		server_port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

	// GET Request
	HINTERNET hHttpFile = HttpOpenRequestW(hConnect, verb.c_str(), endpoint.c_str(),
		NULL, NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_CACHE_WRITE, 0);

	std::wstring headers = L"Content-Type: application/json";
	if (!HttpSendRequestW(hHttpFile, headers.c_str(), headers.size(),
		(LPVOID)szRequestBody.c_str(), szRequestBody.size())) {
		MyOutputDebugStringW(L"HttpSendRequest Failed: %d\n", GetLastError());
		return "";
	}

	DWORD contentlen = 0;
	DWORD size_f = sizeof(contentlen);
	if (!(HttpQueryInfoW(hHttpFile,
		HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentlen,
		&size_f, NULL) && contentlen > 0)) {
		MyOutputDebugStringW(L"HttpQueryInfo Failed %d\n", GetLastError());
		return "";
	}

	BYTE* data = (BYTE*)malloc(contentlen + 1);
	if (data == NULL) {
		return "";
	}
	BYTE* currentPtr = data;
	DWORD copied = 0;
	for (;;) {
		BYTE dataArr[BUFSIZE] = { 0 };
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
			MyOutputDebugStringW(L"InternetReadFile Failed");
		else {
			memcpy_s(currentPtr, contentlen - copied, dataArr, dwBytesRead);
			currentPtr += dwBytesRead;
			copied += dwBytesRead;
		}
	}
	std::string content((char*)data);
	MyOutputDebugStringW(L"[+] Endpoint: %ls\n", endpoint.c_str());
	MyOutputDebugStringW(L"[+] Content-Length: %d\n", copied);
	MyOutputDebugStringW(L"[+] Content: %S\n", content.c_str());

	// process stuff here

	free(data);
	InternetCloseHandle(hInternet);
	InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
	return content;
}

// Polls whether to unblock device
std::mutex pollKillSwitchMutex;
void pollKillSwitch() {
	if (!pollKillSwitchMutex.try_lock()) {
		return;
	}
	for (;;) {
		// Polls to server to ask whether it can kill itself
		std::string content = sendRequest(L"POST", SERVER_NAME, SERVER_PORT, KILL_ENDPOINT, NULL);

		if (content.compare(0, 4, "true") == 0) {
			// Notifies the server that it has performed the function
			std::string content = sendRequest(L"POST", SERVER_NAME, SERVER_PORT, UNBLOCK_ENDPOINT, NULL);

			// Resets all the indicators
			maliciousIndicatorsMutex.lock();
			for (auto const& it : maliciousIndicators) {
				maliciousIndicators[it.first] = 0;
			}
			maliciousIndicatorsMutex.unlock();
		}

		Sleep(200);
	}
}
