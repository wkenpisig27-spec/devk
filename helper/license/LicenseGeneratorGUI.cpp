/**
 * LicenseGeneratorGUI.cpp
 * 
 * PKO License Key Generator - GUI Version
 * 
 * A Windows GUI application for generating license files.
 * Keep this tool PRIVATE - never distribute it!
 * 
 * Compile with:
 *   cl /EHsc LicenseGeneratorGUI.cpp /link crypt32.lib bcrypt.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib /SUBSYSTEM:WINDOWS
 */

#define _WIN32_WINNT 0x0600
#define UNICODE
#define _UNICODE

#include <Windows.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <wincrypt.h>
#include <bcrypt.h>

#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <ctime>
#include <fstream>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ============================================
// SECRET KEY - KEEP THIS PRIVATE!
// Must match the key in LicenseValidator.cpp
// 512-bit cryptographically random key
// ============================================
static const char* LICENSE_SECRET_KEY = "fc39b94bb83b5f61ac11c4bc1a36774270fa55e84adf4f1ef4d118a34559910b5ae2095a9734b4f0fa2909793764a20da14c7a524f4b8c6b7ab75c335a5c190c";
static const char* LICENSE_MAGIC = "PKOLIC1";

// ============================================
// CONTROL IDS
// ============================================
#define ID_EDIT_OWNER       1001
#define ID_COMBO_TYPE       1002
#define ID_EDIT_HWID        1003
#define ID_CHECK_ANYMACHINE 1004
#define ID_CHECK_NOEXPIRY   1005
#define ID_DATE_EXPIRY      1006
#define ID_EDIT_OUTPUT      1007
#define ID_BTN_BROWSE       1008
#define ID_BTN_GENERATE     1009
#define ID_BTN_EXIT         1010
#define ID_STATIC_STATUS    1011

// ============================================
// GLOBAL HANDLES
// ============================================
HWND g_hWnd = nullptr;
HWND g_hEditOwner = nullptr;
HWND g_hComboType = nullptr;
HWND g_hEditHWID = nullptr;
HWND g_hCheckAnyMachine = nullptr;
HWND g_hCheckNoExpiry = nullptr;
HWND g_hDateExpiry = nullptr;
HWND g_hEditOutput = nullptr;
HWND g_hBtnBrowse = nullptr;
HWND g_hBtnGenerate = nullptr;
HWND g_hStaticStatus = nullptr;

HFONT g_hFont = nullptr;
HBRUSH g_hBrushBg = nullptr;

// ============================================
// CRYPTO FUNCTIONS
// ============================================

static std::string ToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

static std::string Base64Encode(const std::string& data) {
    DWORD encodedLen = 0;
    CryptBinaryToStringA((BYTE*)data.c_str(), (DWORD)data.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                         nullptr, &encodedLen);
    
    std::string encoded(encodedLen, 0);
    CryptBinaryToStringA((BYTE*)data.c_str(), (DWORD)data.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                         &encoded[0], &encodedLen);
    
    while (!encoded.empty() && encoded.back() == 0) {
        encoded.pop_back();
    }
    return encoded;
}

static std::string HmacSha256(const std::string& key, const std::string& data) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    DWORD cbHashObject = 0;
    DWORD cbData = 0;
    DWORD cbHash = 32;
    std::vector<BYTE> pbHashObject;
    BYTE pbHash[32];

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) {
        return "";
    }

    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    pbHashObject.resize(cbHashObject);

    if (BCryptCreateHash(hAlg, &hHash, pbHashObject.data(), cbHashObject, 
                         (PBYTE)key.c_str(), (ULONG)key.length(), 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    if (BCryptHashData(hHash, (PBYTE)data.c_str(), (ULONG)data.length(), 0) != 0) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    if (BCryptFinishHash(hHash, pbHash, cbHash, 0) != 0) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    return ToHex(pbHash, 32);
}

static std::string XorObfuscate(const std::string& data, const std::string& key) {
    std::string result = data;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] ^= key[i % key.length()];
    }
    return result;
}

// ============================================
// LICENSE GENERATION
// ============================================

struct LicenseData {
    std::string owner;
    std::string productType;
    std::string hwid;
    time_t createdAt;
    time_t expiresAt;
};

std::string GenerateLicense(const LicenseData& data) {
    std::stringstream ss;
    ss << LICENSE_MAGIC << "|";
    ss << data.owner << "|";
    ss << data.productType << "|";
    ss << data.hwid << "|";
    ss << data.createdAt << "|";
    ss << data.expiresAt;
    
    std::string dataToSign = ss.str();
    std::string signature = HmacSha256(LICENSE_SECRET_KEY, dataToSign);
    
    ss << "|" << signature;
    std::string fullData = ss.str();
    
    std::string obfuscated = XorObfuscate(fullData, LICENSE_SECRET_KEY);
    return Base64Encode(obfuscated);
}

// ============================================
// HELPER FUNCTIONS
// ============================================

std::wstring StringToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wide(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wide[0], size);
    return wide;
}

std::string WideToString(const std::wstring& wide) {
    if (wide.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &str[0], size, nullptr, nullptr);
    return str;
}

std::wstring GetWindowTextStr(HWND hWnd) {
    int len = GetWindowTextLengthW(hWnd);
    if (len == 0) return L"";
    std::wstring text(len + 1, 0);
    GetWindowTextW(hWnd, &text[0], len + 1);
    text.resize(len);
    return text;
}

void SetStatus(const std::wstring& msg, bool isError = false) {
    SetWindowTextW(g_hStaticStatus, msg.c_str());
}

// ============================================
// UI FUNCTIONS
// ============================================

void UpdateHWIDState() {
    BOOL anyMachine = (SendMessage(g_hCheckAnyMachine, BM_GETCHECK, 0, 0) == BST_CHECKED);
    EnableWindow(g_hEditHWID, !anyMachine);
    if (anyMachine) {
        SetWindowTextW(g_hEditHWID, L"*");
    }
}

void UpdateExpiryState() {
    BOOL noExpiry = (SendMessage(g_hCheckNoExpiry, BM_GETCHECK, 0, 0) == BST_CHECKED);
    EnableWindow(g_hDateExpiry, !noExpiry);
}

void BrowseOutputFile() {
    wchar_t filename[MAX_PATH] = L"license.lic";
    
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFilter = L"License Files (*.lic)\0*.lic\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"lic";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameW(&ofn)) {
        SetWindowTextW(g_hEditOutput, filename);
    }
}

void DoGenerate() {
    // Get owner
    std::wstring ownerW = GetWindowTextStr(g_hEditOwner);
    if (ownerW.empty()) {
        MessageBoxW(g_hWnd, L"Please enter a server name.", L"Validation Error", MB_ICONWARNING);
        SetFocus(g_hEditOwner);
        return;
    }
    
    // Get product type
    int typeIndex = (int)SendMessage(g_hComboType, CB_GETCURSEL, 0, 0);
    std::string productType = "full";
    if (typeIndex == 1) productType = "server";
    else if (typeIndex == 2) productType = "client";
    
    // Get HWID
    std::wstring hwidW = GetWindowTextStr(g_hEditHWID);
    if (hwidW.empty()) {
        hwidW = L"*";
    }
    
    // Get expiry
    time_t expiresAt = 0;
    BOOL noExpiry = (SendMessage(g_hCheckNoExpiry, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (!noExpiry) {
        SYSTEMTIME st;
        SendMessage(g_hDateExpiry, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
        
        struct tm tm = {};
        tm.tm_year = st.wYear - 1900;
        tm.tm_mon = st.wMonth - 1;
        tm.tm_mday = st.wDay;
        tm.tm_hour = 23;
        tm.tm_min = 59;
        tm.tm_sec = 59;
        expiresAt = mktime(&tm);
    }
    
    // Get output file
    std::wstring outputW = GetWindowTextStr(g_hEditOutput);
    if (outputW.empty()) {
        outputW = L"license.lic";
    }
    
    // Build license data
    LicenseData data;
    data.owner = WideToString(ownerW);
    data.productType = productType;
    data.hwid = WideToString(hwidW);
    data.createdAt = time(nullptr);
    data.expiresAt = expiresAt;
    
    // Generate license
    std::string license = GenerateLicense(data);
    
    // Save to file
    std::string outputPath = WideToString(outputW);
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        MessageBoxW(g_hWnd, L"Failed to write to output file!", L"Error", MB_ICONERROR);
        return;
    }
    
    file << license;
    file.close();
    
    // Success message
    std::wstringstream msg;
    msg << L"License generated successfully!\n\n";
    msg << L"Server: " << ownerW << L"\n";
    msg << L"Type: " << StringToWide(productType) << L"\n";
    msg << L"HWID: " << hwidW << L"\n";
    if (expiresAt == 0) {
        msg << L"Expires: Never\n";
    } else {
        wchar_t dateStr[64];
        struct tm* tm = localtime(&expiresAt);
        wcsftime(dateStr, 64, L"%Y-%m-%d", tm);
        msg << L"Expires: " << dateStr << L"\n";
    }
    msg << L"\nSaved to: " << outputW;
    
    MessageBoxW(g_hWnd, msg.str().c_str(), L"Success", MB_ICONINFORMATION);
    SetStatus(L"License generated: " + outputW);
    
    // Ask to open folder
    int result = MessageBoxW(g_hWnd, L"Open the output folder?", L"License Generated", MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES) {
        // Get folder path and select the file
        std::wstring folder = outputW;
        size_t lastSlash = folder.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            folder = folder.substr(0, lastSlash);
        }
        ShellExecuteW(nullptr, L"explore", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

// ============================================
// WINDOW PROCEDURE
// ============================================

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Create font
        g_hFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        
        g_hBrushBg = CreateSolidBrush(RGB(245, 245, 250));
        
        int y = 20;
        int labelWidth = 120;
        int inputWidth = 300;
        int x = 20;
        int spacing = 35;
        
        // Server Name
        CreateWindowW(L"STATIC", L"Server Name:", WS_VISIBLE | WS_CHILD,
                      x, y + 3, labelWidth, 20, hWnd, nullptr, nullptr, nullptr);
        g_hEditOwner = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                       WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                       x + labelWidth, y, inputWidth, 24, hWnd, (HMENU)ID_EDIT_OWNER, nullptr, nullptr);
        y += spacing;
        
        // Product Type
        CreateWindowW(L"STATIC", L"Product Type:", WS_VISIBLE | WS_CHILD,
                      x, y + 3, labelWidth, 20, hWnd, nullptr, nullptr, nullptr);
        g_hComboType = CreateWindowW(L"COMBOBOX", L"",
                                     WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                                     x + labelWidth, y, inputWidth, 200, hWnd, (HMENU)ID_COMBO_TYPE, nullptr, nullptr);
        SendMessage(g_hComboType, CB_ADDSTRING, 0, (LPARAM)L"Full (All Features)");
        SendMessage(g_hComboType, CB_ADDSTRING, 0, (LPARAM)L"Server Only");
        SendMessage(g_hComboType, CB_ADDSTRING, 0, (LPARAM)L"Client Only");
        SendMessage(g_hComboType, CB_SETCURSEL, 0, 0);
        y += spacing;
        
        // HWID
        CreateWindowW(L"STATIC", L"Hardware ID:", WS_VISIBLE | WS_CHILD,
                      x, y + 3, labelWidth, 20, hWnd, nullptr, nullptr, nullptr);
        g_hEditHWID = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                      WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                      x + labelWidth, y, inputWidth - 130, 24, hWnd, (HMENU)ID_EDIT_HWID, nullptr, nullptr);
        g_hCheckAnyMachine = CreateWindowW(L"BUTTON", L"Any Machine",
                                           WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                           x + labelWidth + inputWidth - 120, y + 2, 120, 20, hWnd, (HMENU)ID_CHECK_ANYMACHINE, nullptr, nullptr);
        SendMessage(g_hCheckAnyMachine, BM_SETCHECK, BST_CHECKED, 0);
        UpdateHWIDState();
        y += spacing;
        
        // Expiry Date
        CreateWindowW(L"STATIC", L"Expires On:", WS_VISIBLE | WS_CHILD,
                      x, y + 3, labelWidth, 20, hWnd, nullptr, nullptr, nullptr);
        g_hDateExpiry = CreateWindowW(DATETIMEPICK_CLASS, L"",
                                      WS_VISIBLE | WS_CHILD | WS_TABSTOP | DTS_SHORTDATECENTURYFORMAT,
                                      x + labelWidth, y, 150, 24, hWnd, (HMENU)ID_DATE_EXPIRY, nullptr, nullptr);
        g_hCheckNoExpiry = CreateWindowW(L"BUTTON", L"Never Expires",
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                                         x + labelWidth + 160, y + 2, 130, 20, hWnd, (HMENU)ID_CHECK_NOEXPIRY, nullptr, nullptr);
        SendMessage(g_hCheckNoExpiry, BM_SETCHECK, BST_CHECKED, 0);
        UpdateExpiryState();
        
        // Set default date to 1 year from now
        SYSTEMTIME st;
        GetLocalTime(&st);
        st.wYear += 1;
        SendMessage(g_hDateExpiry, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
        y += spacing;
        
        // Output File - Default to executable directory
        wchar_t exePath[MAX_PATH] = {0};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring defaultOutputPath(exePath);
        size_t lastSlash = defaultOutputPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            defaultOutputPath = defaultOutputPath.substr(0, lastSlash + 1);
        }
        defaultOutputPath += L"license.lic";
        
        CreateWindowW(L"STATIC", L"Output File:", WS_VISIBLE | WS_CHILD,
                      x, y + 3, labelWidth, 20, hWnd, nullptr, nullptr, nullptr);
        g_hEditOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", defaultOutputPath.c_str(),
                                        WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                        x + labelWidth, y, inputWidth - 80, 24, hWnd, (HMENU)ID_EDIT_OUTPUT, nullptr, nullptr);
        g_hBtnBrowse = CreateWindowW(L"BUTTON", L"Browse...",
                                     WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                     x + labelWidth + inputWidth - 70, y, 70, 24, hWnd, (HMENU)ID_BTN_BROWSE, nullptr, nullptr);
        y += spacing + 20;
        
        // Separator
        CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
                      x, y, labelWidth + inputWidth, 2, hWnd, nullptr, nullptr, nullptr);
        y += 15;
        
        // Buttons
        g_hBtnGenerate = CreateWindowW(L"BUTTON", L"Generate License",
                                       WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                       x + labelWidth, y, 150, 32, hWnd, (HMENU)ID_BTN_GENERATE, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Exit",
                      WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                      x + labelWidth + 160, y, 80, 32, hWnd, (HMENU)ID_BTN_EXIT, nullptr, nullptr);
        y += 45;
        
        // Status bar
        g_hStaticStatus = CreateWindowW(L"STATIC", L"Ready. Enter license details and click Generate.",
                                        WS_VISIBLE | WS_CHILD | SS_LEFT,
                                        x, y, labelWidth + inputWidth, 20, hWnd, (HMENU)ID_STATIC_STATUS, nullptr, nullptr);
        
        // Apply font to all controls
        EnumChildWindows(hWnd, [](HWND child, LPARAM lParam) -> BOOL {
            SendMessage(child, WM_SETFONT, (WPARAM)lParam, TRUE);
            return TRUE;
        }, (LPARAM)g_hFont);
        
        return 0;
    }
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CHECK_ANYMACHINE:
            UpdateHWIDState();
            break;
        case ID_CHECK_NOEXPIRY:
            UpdateExpiryState();
            break;
        case ID_BTN_BROWSE:
            BrowseOutputFile();
            break;
        case ID_BTN_GENERATE:
            DoGenerate();
            break;
        case ID_BTN_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)g_hBrushBg;
    }
    
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_hBrushBg);
        return 1;
    }
    
    case WM_DESTROY:
        if (g_hFont) DeleteObject(g_hFont);
        if (g_hBrushBg) DeleteObject(g_hBrushBg);
        PostQuitMessage(0);
        return 0;
    
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================
// MAIN ENTRY POINT
// ============================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    // Initialize Common Controls
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_DATE_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PKOLicenseGenerator";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // Calculate window size
    int windowWidth = 480;
    int windowHeight = 320;
    
    // Center on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    // Create window
    g_hWnd = CreateWindowExW(0, L"PKOLicenseGenerator", L"PKO License Generator",
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                             x, y, windowWidth, windowHeight,
                             nullptr, nullptr, hInstance, nullptr);
    
    if (!g_hWnd) {
        MessageBoxW(nullptr, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(g_hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return (int)msg.wParam;
}
