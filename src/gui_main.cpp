#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include "BankService.hpp"
#include "Utils.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")

namespace {

using namespace Banking;

// Encoding helpers
std::wstring toWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 1) return L"";
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    return result;
}

std::string toUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return "";
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

// Global Application State
BankService g_bank("data/bank_data.json");
Account* g_currentUser = nullptr;

HWND g_hMainWnd = nullptr;
HWND g_hLoginGroup = nullptr;
HWND g_hUserLabel = nullptr, g_hUserEdit = nullptr;
HWND g_hPassLabel = nullptr, g_hPassEdit = nullptr;
HWND g_hBtnLogin = nullptr;
HWND g_hBtnQuickUser1 = nullptr, g_hBtnQuickUser2 = nullptr, g_hBtnQuickAdmin = nullptr;

// Dashboard controls
HWND g_hCardGroup = nullptr;
HWND g_hCardOwner = nullptr;
HWND g_hCardNumber = nullptr;
HWND g_hCardIban = nullptr;
HWND g_hCardBalance = nullptr;
HWND g_hCardStatus = nullptr;

HWND g_hBtnDeposit = nullptr;
HWND g_hBtnWithdraw = nullptr;
HWND g_hBtnTransfer = nullptr;
HWND g_hBtnStatement = nullptr;
HWND g_hBtnLogout = nullptr;

HWND g_hTxLabel = nullptr;
HWND g_hTxListView = nullptr;

// Admin controls
HWND g_hAdminGroup = nullptr;
HWND g_hAdminReserves = nullptr;
HWND g_hAdminListView = nullptr;
HWND g_hBtnAdminFreeze = nullptr;
HWND g_hBtnAdminUnfreeze = nullptr;

HFONT g_hFontNormal = nullptr;
HFONT g_hFontBold = nullptr;
HFONT g_hFontBig = nullptr;
HFONT g_hFontTitle = nullptr;

enum ControlId {
    ID_BTN_LOGIN = 1001,
    ID_BTN_QUICK_U1,
    ID_BTN_QUICK_U2,
    ID_BTN_QUICK_ADMIN,
    ID_BTN_DEPOSIT,
    ID_BTN_WITHDRAW,
    ID_BTN_TRANSFER,
    ID_BTN_STATEMENT,
    ID_BTN_LOGOUT,
    ID_BTN_ADMIN_FREEZE,
    ID_BTN_ADMIN_UNFREEZE
};

void updateDashboardData();
void refreshTransactionsList();
void refreshAdminClientsList();

// Simple Win32 Input Dialog Helper
bool showPromptDialog(HWND hParent, const std::wstring& title, const std::wstring& prompt, std::wstring& outValue) {
    // We can create a simple modal dialog
    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770", title.c_str(),
        WS_VISIBLE | WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 400, 190, hParent, nullptr, GetModuleHandle(nullptr), nullptr
    );
    if (!hDlg) return false;

    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    SetWindowPos(hDlg, nullptr, rcParent.left + 80, rcParent.top + 100, 400, 190, SWP_NOZORDER);

    CreateWindowExW(0, L"STATIC", prompt.c_str(), WS_VISIBLE | WS_CHILD,
        20, 20, 340, 30, hDlg, nullptr, nullptr, nullptr);

    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", outValue.c_str(),
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        20, 55, 340, 26, hDlg, nullptr, nullptr, nullptr);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SetFocus(hEdit);

    HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"Підтвердити", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        150, 100, 100, 30, hDlg, (HMENU)IDOK, nullptr, nullptr);
    HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Скасувати", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        260, 100, 100, 30, hDlg, (HMENU)IDCANCEL, nullptr, nullptr);

    SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    EnableWindow(hParent, FALSE);

    bool confirmed = false;
    MSG msg;
    while (IsWindow(hDlg)) {
        if (GetMessage(&msg, nullptr, 0, 0)) {
            if (msg.message == WM_COMMAND) {
                if (LOWORD(msg.wParam) == IDOK) {
                    wchar_t buf[512] = {0};
                    GetWindowTextW(hEdit, buf, 511);
                    outValue = buf;
                    confirmed = true;
                    DestroyWindow(hDlg);
                    break;
                } else if (LOWORD(msg.wParam) == IDCANCEL) {
                    confirmed = false;
                    DestroyWindow(hDlg);
                    break;
                }
            } else if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                confirmed = false;
                DestroyWindow(hDlg);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    return confirmed;
}

void showLoginView(bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    ShowWindow(g_hLoginGroup, cmd);
    ShowWindow(g_hUserLabel, cmd);
    ShowWindow(g_hUserEdit, cmd);
    ShowWindow(g_hPassLabel, cmd);
    ShowWindow(g_hPassEdit, cmd);
    ShowWindow(g_hBtnLogin, cmd);
    ShowWindow(g_hBtnQuickUser1, cmd);
    ShowWindow(g_hBtnQuickUser2, cmd);
    ShowWindow(g_hBtnQuickAdmin, cmd);
}

void showClientDashboard(bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    ShowWindow(g_hCardGroup, cmd);
    ShowWindow(g_hCardOwner, cmd);
    ShowWindow(g_hCardNumber, cmd);
    ShowWindow(g_hCardIban, cmd);
    ShowWindow(g_hCardBalance, cmd);
    ShowWindow(g_hCardStatus, cmd);

    ShowWindow(g_hBtnDeposit, cmd);
    ShowWindow(g_hBtnWithdraw, cmd);
    ShowWindow(g_hBtnTransfer, cmd);
    ShowWindow(g_hBtnStatement, cmd);
    ShowWindow(g_hBtnLogout, cmd);

    ShowWindow(g_hTxLabel, cmd);
    ShowWindow(g_hTxListView, cmd);
}

void showAdminDashboard(bool show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    ShowWindow(g_hAdminGroup, cmd);
    ShowWindow(g_hAdminReserves, cmd);
    ShowWindow(g_hAdminListView, cmd);
    ShowWindow(g_hBtnAdminFreeze, cmd);
    ShowWindow(g_hBtnAdminUnfreeze, cmd);
    ShowWindow(g_hBtnLogout, cmd);
}

void updateDashboardData() {
    if (!g_currentUser) return;

    if (g_currentUser->isAdmin()) {
        showLoginView(false);
        showClientDashboard(false);
        showAdminDashboard(true);

        double total = g_bank.getTotalReserves();
        std::wstring resStr = L"Загальний депозитний портфель банку: " + toWide(Utils::formatMoney(total));
        SetWindowTextW(g_hAdminReserves, resStr.c_str());
        refreshAdminClientsList();
    } else {
        showLoginView(false);
        showAdminDashboard(false);
        showClientDashboard(true);

        std::wstring ownerStr = L"Власник: " + toWide(g_currentUser->getFullName());
        SetWindowTextW(g_hCardOwner, ownerStr.c_str());

        std::wstring cardStr = L"Номер картки: " + toWide(g_currentUser->getCardNumber());
        SetWindowTextW(g_hCardNumber, cardStr.c_str());

        std::wstring ibanStr = L"IBAN рахунку: " + toWide(g_currentUser->getAccountNumber());
        SetWindowTextW(g_hCardIban, ibanStr.c_str());

        std::string balText = "Баланс: " + Utils::formatMoney(g_currentUser->getBalance(), g_currentUser->getCurrency());
        SetWindowTextW(g_hCardBalance, toWide(balText).c_str());

        std::wstring statusStr = g_currentUser->isFrozen() ? L"Статус: [ ЗАБЛОКОВАНО ]" : L"Статус: [ АКТИВНА ]";
        SetWindowTextW(g_hCardStatus, statusStr.c_str());

        refreshTransactionsList();
    }
}

void refreshTransactionsList() {
    if (!g_currentUser || !g_hTxListView) return;
    ListView_DeleteAllItems(g_hTxListView);

    const auto& txs = g_currentUser->getTransactions();
    int index = 0;
    for (auto it = txs.rbegin(); it != txs.rend(); ++it) {
        const auto& tx = *it;
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        std::wstring timeW = toWide(tx.getTimestamp());
        lvi.pszText = (LPWSTR)timeW.c_str();
        ListView_InsertItem(g_hTxListView, &lvi);

        std::string typeLabel;
        std::string sign = "";
        switch (tx.getType()) {
            case TransactionType::DEPOSIT:
                typeLabel = "Поповнення"; sign = "+"; break;
            case TransactionType::WITHDRAW:
                typeLabel = "Зняття в ATM"; sign = "-"; break;
            case TransactionType::TRANSFER_OUT:
                typeLabel = "Переказ (вих)"; sign = "-"; break;
            case TransactionType::TRANSFER_IN:
                typeLabel = "Переказ (вхід)"; sign = "+"; break;
            default:
                typeLabel = "Операція"; break;
        }

        std::wstring typeW = toWide(typeLabel);
        ListView_SetItemText(g_hTxListView, index, 1, (LPWSTR)typeW.c_str());

        std::wstring amountW = toWide(sign + Utils::formatMoney(tx.getAmount()));
        ListView_SetItemText(g_hTxListView, index, 2, (LPWSTR)amountW.c_str());

        std::wstring balW = toWide(Utils::formatMoney(tx.getBalanceAfter()));
        ListView_SetItemText(g_hTxListView, index, 3, (LPWSTR)balW.c_str());

        std::wstring detailsW = toWide(tx.getCounterparty() + " (" + tx.getDescription() + ")");
        ListView_SetItemText(g_hTxListView, index, 4, (LPWSTR)detailsW.c_str());

        index++;
    }
}

void refreshAdminClientsList() {
    if (!g_hAdminListView) return;
    ListView_DeleteAllItems(g_hAdminListView);

    const auto& accounts = g_bank.getAllAccounts();
    int index = 0;
    for (const auto& acc : accounts) {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        std::wstring userW = toWide(acc.getUsername());
        lvi.pszText = (LPWSTR)userW.c_str();
        ListView_InsertItem(g_hAdminListView, &lvi);

        std::wstring nameW = toWide(acc.getFullName());
        ListView_SetItemText(g_hAdminListView, index, 1, (LPWSTR)nameW.c_str());

        std::wstring cardW = toWide(acc.getCardNumber());
        ListView_SetItemText(g_hAdminListView, index, 2, (LPWSTR)cardW.c_str());

        std::wstring balW = toWide(Utils::formatMoney(acc.getBalance()));
        ListView_SetItemText(g_hAdminListView, index, 3, (LPWSTR)balW.c_str());

        std::wstring statusW = acc.isFrozen() ? L"ЗАБЛОКОВАНО" : L"АКТИВНИЙ";
        ListView_SetItemText(g_hAdminListView, index, 4, (LPWSTR)statusW.c_str());

        index++;
    }
}

void handleLoginAttempt(const std::string& username, const std::string& password) {
    Account* acc = g_bank.login(username, password);
    if (!acc) {
        MessageBoxW(g_hMainWnd, L"Помилка: Невірний логін або пароль!", L"Авторизація", MB_OK | MB_ICONERROR);
        return;
    }
    g_currentUser = acc;
    updateDashboardData();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            g_hFontNormal = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_hFontBold   = CreateFontW(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_hFontBig    = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_hFontTitle  = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            // 1. LOGIN CONTROLS
            g_hLoginGroup = CreateWindowExW(0, L"BUTTON", L" Авторизація клієнта Bank Forma Tail ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 180, 80, 480, 360, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(g_hLoginGroup, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hUserLabel = CreateWindowExW(0, L"STATIC", L"Логін користувача:", WS_CHILD | WS_VISIBLE, 220, 130, 200, 20, hWnd, nullptr, nullptr, nullptr);
            g_hUserEdit  = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"user1", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 220, 155, 390, 28, hWnd, nullptr, nullptr, nullptr);

            g_hPassLabel = CreateWindowExW(0, L"STATIC", L"Пароль:", WS_CHILD | WS_VISIBLE, 220, 195, 200, 20, hWnd, nullptr, nullptr, nullptr);
            g_hPassEdit  = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1234", WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL, 220, 220, 390, 28, hWnd, nullptr, nullptr, nullptr);

            g_hBtnLogin = CreateWindowExW(0, L"BUTTON", L"Увійти в кабінет", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 220, 265, 390, 36, hWnd, (HMENU)ID_BTN_LOGIN, nullptr, nullptr);

            g_hBtnQuickUser1 = CreateWindowExW(0, L"BUTTON", L"Швидкий вхід: user1", WS_CHILD | WS_VISIBLE, 220, 320, 190, 30, hWnd, (HMENU)ID_BTN_QUICK_U1, nullptr, nullptr);
            g_hBtnQuickUser2 = CreateWindowExW(0, L"BUTTON", L"Швидкий вхід: user2", WS_CHILD | WS_VISIBLE, 420, 320, 190, 30, hWnd, (HMENU)ID_BTN_QUICK_U2, nullptr, nullptr);
            g_hBtnQuickAdmin = CreateWindowExW(0, L"BUTTON", L"Вхід як Адміністратор (admin)", WS_CHILD | WS_VISIBLE, 220, 360, 390, 30, hWnd, (HMENU)ID_BTN_QUICK_ADMIN, nullptr, nullptr);

            SendMessageW(g_hUserLabel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hUserEdit,  WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hPassLabel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hPassEdit,  WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hBtnLogin,  WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessageW(g_hBtnQuickUser1, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hBtnQuickUser2, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hBtnQuickAdmin, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

            // 2. CLIENT DASHBOARD CONTROLS
            g_hCardGroup = CreateWindowExW(0, L"BUTTON", L" Платіжна картка Visa Platinum | Bank Forma Tail ", WS_CHILD | BS_GROUPBOX, 20, 15, 820, 135, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(g_hCardGroup, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hCardOwner   = CreateWindowExW(0, L"STATIC", L"Власник: ...", WS_CHILD, 40, 42, 380, 22, hWnd, nullptr, nullptr, nullptr);
            g_hCardNumber  = CreateWindowExW(0, L"STATIC", L"Номер картки: ...", WS_CHILD, 40, 68, 380, 22, hWnd, nullptr, nullptr, nullptr);
            g_hCardIban    = CreateWindowExW(0, L"STATIC", L"IBAN: ...", WS_CHILD, 40, 94, 380, 22, hWnd, nullptr, nullptr, nullptr);
            g_hCardBalance = CreateWindowExW(0, L"STATIC", L"Баланс: 0.00 UAH", WS_CHILD, 460, 45, 360, 35, hWnd, nullptr, nullptr, nullptr);
            g_hCardStatus  = CreateWindowExW(0, L"STATIC", L"Статус: [ АКТИВНА ]", WS_CHILD, 460, 85, 360, 22, hWnd, nullptr, nullptr, nullptr);

            SendMessageW(g_hCardOwner,   WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hCardNumber,  WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessageW(g_hCardIban,    WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            SendMessageW(g_hCardBalance, WM_SETFONT, (WPARAM)g_hFontBig, TRUE);
            SendMessageW(g_hCardStatus,  WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            // Action Buttons
            g_hBtnDeposit   = CreateWindowExW(0, L"BUTTON", L"💵 Поповнити", WS_CHILD | BS_PUSHBUTTON, 20, 160, 150, 36, hWnd, (HMENU)ID_BTN_DEPOSIT, nullptr, nullptr);
            g_hBtnWithdraw  = CreateWindowExW(0, L"BUTTON", L"🏧 Зняти кошти", WS_CHILD | BS_PUSHBUTTON, 185, 160, 150, 36, hWnd, (HMENU)ID_BTN_WITHDRAW, nullptr, nullptr);
            g_hBtnTransfer  = CreateWindowExW(0, L"BUTTON", L"🔄 Переказ коштів", WS_CHILD | BS_PUSHBUTTON, 350, 160, 150, 36, hWnd, (HMENU)ID_BTN_TRANSFER, nullptr, nullptr);
            g_hBtnStatement = CreateWindowExW(0, L"BUTTON", L"📄 Виписка (.txt)", WS_CHILD | BS_PUSHBUTTON, 515, 160, 150, 36, hWnd, (HMENU)ID_BTN_STATEMENT, nullptr, nullptr);
            g_hBtnLogout    = CreateWindowExW(0, L"BUTTON", L"🚪 Вийти", WS_CHILD | BS_PUSHBUTTON, 680, 160, 160, 36, hWnd, (HMENU)ID_BTN_LOGOUT, nullptr, nullptr);

            SendMessageW(g_hBtnDeposit,   WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessageW(g_hBtnWithdraw,  WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessageW(g_hBtnTransfer,  WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessageW(g_hBtnStatement, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessageW(g_hBtnLogout,    WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            // Transactions ListView
            g_hTxLabel = CreateWindowExW(0, L"STATIC", L"Останні операції по рахунку:", WS_CHILD, 20, 210, 400, 20, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(g_hTxLabel, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hTxListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
                20, 235, 820, 350, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(g_hTxListView, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            ListView_SetExtendedListViewStyle(g_hTxListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            LVCOLUMNW lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

            lvc.cx = 150; lvc.pszText = (LPWSTR)L"Дата і час";
            ListView_InsertColumn(g_hTxListView, 0, &lvc);
            lvc.cx = 120; lvc.pszText = (LPWSTR)L"Тип операції";
            ListView_InsertColumn(g_hTxListView, 1, &lvc);
            lvc.cx = 110; lvc.pszText = (LPWSTR)L"Сума";
            ListView_InsertColumn(g_hTxListView, 2, &lvc);
            lvc.cx = 110; lvc.pszText = (LPWSTR)L"Залишок";
            ListView_InsertColumn(g_hTxListView, 3, &lvc);
            lvc.cx = 320; lvc.pszText = (LPWSTR)L"Контрагент / Опис";
            ListView_InsertColumn(g_hTxListView, 4, &lvc);

            // 3. ADMIN CONTROLS
            g_hAdminGroup = CreateWindowExW(0, L"BUTTON", L" Панель Адміністратора | Bank Forma Tail ", WS_CHILD | BS_GROUPBOX, 20, 15, 820, 85, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(g_hAdminGroup, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hAdminReserves = CreateWindowExW(0, L"STATIC", L"Капітал банку: ...", WS_CHILD, 40, 45, 500, 30, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(g_hAdminReserves, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

            g_hBtnAdminFreeze   = CreateWindowExW(0, L"BUTTON", L"❄️ Заблокувати", WS_CHILD | BS_PUSHBUTTON, 20, 110, 150, 36, hWnd, (HMENU)ID_BTN_ADMIN_FREEZE, nullptr, nullptr);
            g_hBtnAdminUnfreeze = CreateWindowExW(0, L"BUTTON", L"🔓 Розблокувати", WS_CHILD | BS_PUSHBUTTON, 185, 110, 150, 36, hWnd, (HMENU)ID_BTN_ADMIN_UNFREEZE, nullptr, nullptr);
            SendMessageW(g_hBtnAdminFreeze,   WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
            SendMessageW(g_hBtnAdminUnfreeze, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

            g_hAdminListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
                20, 160, 820, 425, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(g_hAdminListView, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            ListView_SetExtendedListViewStyle(g_hAdminListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            lvc.cx = 100; lvc.pszText = (LPWSTR)L"Логін";
            ListView_InsertColumn(g_hAdminListView, 0, &lvc);
            lvc.cx = 230; lvc.pszText = (LPWSTR)L"ПІБ Клієнта";
            ListView_InsertColumn(g_hAdminListView, 1, &lvc);
            lvc.cx = 180; lvc.pszText = (LPWSTR)L"Номер картки";
            ListView_InsertColumn(g_hAdminListView, 2, &lvc);
            lvc.cx = 140; lvc.pszText = (LPWSTR)L"Баланс";
            ListView_InsertColumn(g_hAdminListView, 3, &lvc);
            lvc.cx = 140; lvc.pszText = (LPWSTR)L"Статус";
            ListView_InsertColumn(g_hAdminListView, 4, &lvc);

            showLoginView(true);
            showClientDashboard(false);
            showAdminDashboard(false);
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case ID_BTN_LOGIN: {
                    wchar_t uBuf[128] = {0}, pBuf[128] = {0};
                    GetWindowTextW(g_hUserEdit, uBuf, 127);
                    GetWindowTextW(g_hPassEdit, pBuf, 127);
                    handleLoginAttempt(toUtf8(uBuf), toUtf8(pBuf));
                    break;
                }
                case ID_BTN_QUICK_U1: {
                    SetWindowTextW(g_hUserEdit, L"user1");
                    SetWindowTextW(g_hPassEdit, L"1234");
                    handleLoginAttempt("user1", "1234");
                    break;
                }
                case ID_BTN_QUICK_U2: {
                    SetWindowTextW(g_hUserEdit, L"user2");
                    SetWindowTextW(g_hPassEdit, L"1234");
                    handleLoginAttempt("user2", "1234");
                    break;
                }
                case ID_BTN_QUICK_ADMIN: {
                    SetWindowTextW(g_hUserEdit, L"admin");
                    SetWindowTextW(g_hPassEdit, L"admin123");
                    handleLoginAttempt("admin", "admin123");
                    break;
                }
                case ID_BTN_LOGOUT: {
                    g_currentUser = nullptr;
                    showClientDashboard(false);
                    showAdminDashboard(false);
                    showLoginView(true);
                    break;
                }
                case ID_BTN_DEPOSIT: {
                    if (!g_currentUser) break;
                    std::wstring val = L"500.00";
                    if (showPromptDialog(hWnd, L"Поповнення рахунку", L"Введіть суму для поповнення балансу (грн):", val)) {
                        try {
                            double amt = std::stod(val);
                            OperationResult res = g_bank.deposit(g_currentUser, amt, "Поповнення через касу");
                            if (res == OperationResult::SUCCESS) {
                                MessageBoxW(hWnd, (L"Рахунок успішно поповнено на " + val + L" грн!").c_str(), L"Успіх", MB_OK | MB_ICONINFORMATION);
                                updateDashboardData();
                            } else {
                                MessageBoxW(hWnd, toWide(getResultMessage(res)).c_str(), L"Помилка", MB_OK | MB_ICONERROR);
                            }
                        } catch (...) {
                            MessageBoxW(hWnd, L"Некоректна сума!", L"Помилка", MB_OK | MB_ICONWARNING);
                        }
                    }
                    break;
                }
                case ID_BTN_WITHDRAW: {
                    if (!g_currentUser) break;
                    std::wstring val = L"200.00";
                    if (showPromptDialog(hWnd, L"Зняття готівки", L"Введіть суму для зняття в банкоматі (грн):", val)) {
                        try {
                            double amt = std::stod(val);
                            OperationResult res = g_bank.withdraw(g_currentUser, amt, "Зняття в банкоматі ATM Forma Tail");
                            if (res == OperationResult::SUCCESS) {
                                MessageBoxW(hWnd, (L"Кошти видано успішно: " + val + L" грн!").c_str(), L"Успіх", MB_OK | MB_ICONINFORMATION);
                                updateDashboardData();
                            } else {
                                MessageBoxW(hWnd, toWide(getResultMessage(res)).c_str(), L"Помилка", MB_OK | MB_ICONERROR);
                            }
                        } catch (...) {
                            MessageBoxW(hWnd, L"Некоректна сума!", L"Помилка", MB_OK | MB_ICONWARNING);
                        }
                    }
                    break;
                }
                case ID_BTN_TRANSFER: {
                    if (!g_currentUser) break;
                    std::wstring target = L"user2";
                    if (!showPromptDialog(hWnd, L"Переказ коштів", L"Введіть логін, номер картки або IBAN отримувача:", target)) {
                        break;
                    }

                    std::wstring val = L"300.00";
                    if (showPromptDialog(hWnd, L"Сума переказу", (L"Введіть суму для переказу на " + target + L" (грн):").c_str(), val)) {
                        try {
                            double amt = std::stod(val);
                            OperationResult res = g_bank.transfer(g_currentUser, toUtf8(target), amt, "Приватний переказ");
                            if (res == OperationResult::SUCCESS) {
                                MessageBoxW(hWnd, (L"Переказ на суму " + val + L" грн успішно надіслано!").c_str(), L"Успіх", MB_OK | MB_ICONINFORMATION);
                                updateDashboardData();
                            } else {
                                MessageBoxW(hWnd, toWide(getResultMessage(res)).c_str(), L"Помилка", MB_OK | MB_ICONERROR);
                            }
                        } catch (...) {
                            MessageBoxW(hWnd, L"Некоректна сума!", L"Помилка", MB_OK | MB_ICONWARNING);
                        }
                    }
                    break;
                }
                case ID_BTN_STATEMENT: {
                    if (!g_currentUser) break;
                    std::string fileName = "statement_" + g_currentUser->getUsername() + ".txt";
                    if (g_bank.exportStatement(*g_currentUser, fileName)) {
                        std::wstring msg = L"Офіційну виписку успішно збережено у файл:\n" + toWide(fileName);
                        MessageBoxW(hWnd, msg.c_str(), L"Виписка", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBoxW(hWnd, L"Помилка запису файлу виписки!", L"Помилка", MB_OK | MB_ICONERROR);
                    }
                    break;
                }
                case ID_BTN_ADMIN_FREEZE: {
                    int sel = ListView_GetNextItem(g_hAdminListView, -1, LVNI_SELECTED);
                    if (sel < 0) {
                        MessageBoxW(hWnd, L"Оберіть клієнта зі списку!", L"Увага", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    wchar_t userBuf[128] = {0};
                    ListView_GetItemText(g_hAdminListView, sel, 0, userBuf, 127);
                    Account* acc = g_bank.findAccountByUsername(toUtf8(userBuf));
                    if (acc) {
                        g_bank.setAccountFrozen(acc->getAccountNumber(), true);
                        updateDashboardData();
                        MessageBoxW(hWnd, L"Рахунок клієнта успішно заблоковано!", L"Адмін", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                case ID_BTN_ADMIN_UNFREEZE: {
                    int sel = ListView_GetNextItem(g_hAdminListView, -1, LVNI_SELECTED);
                    if (sel < 0) {
                        MessageBoxW(hWnd, L"Оберіть клієнта зі списку!", L"Увага", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    wchar_t userBuf[128] = {0};
                    ListView_GetItemText(g_hAdminListView, sel, 0, userBuf, 127);
                    Account* acc = g_bank.findAccountByUsername(toUtf8(userBuf));
                    if (acc) {
                        g_bank.setAccountFrozen(acc->getAccountNumber(), false);
                        updateDashboardData();
                        MessageBoxW(hWnd, L"Рахунок клієнта успішно розблоковано!", L"Адмін", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
            }
            break;
        }

        case WM_DESTROY: {
            if (g_hFontNormal) DeleteObject(g_hFontNormal);
            if (g_hFontBold)   DeleteObject(g_hFontBold);
            if (g_hFontBig)    DeleteObject(g_hFontBig);
            if (g_hFontTitle)  DeleteObject(g_hFontTitle);
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    try {
        std::filesystem::create_directories("data");
    } catch (...) {}

    if (!g_bank.initialize()) {
        MessageBoxW(nullptr, L"Помилка ініціалізації сховища даних банку!", L"Критична помилка", MB_OK | MB_ICONERROR);
        return 1;
    }

    const wchar_t CLASS_NAME[] = L"BankFormaTailGuiWindow";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Bank Forma Tail - Графічна Банківська Система (Desktop GUI)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 880, 640,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) return 0;

    g_hMainWnd = hWnd;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
