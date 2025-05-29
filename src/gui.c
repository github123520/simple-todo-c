#include "todo.h"
#include "utils/search.h"
#include <math.h>
#include <string.h>

NOTIFYICONDATA nid;
HMENU hTrayMenu;
BOOL bSortOrder = TRUE;
UINT dpi = 96;

typedef UINT (WINAPI *_GetDpiForWindowFunc)(HWND);

HWND hTitleLabel;
HWND hDescLabel;
HWND hPriorityLabel;

int _GetDpiForWindow(HWND hwnd) {
    HMODULE hUser32 = GetModuleHandle("user32.dll");
    if (hUser32) {
        _GetDpiForWindowFunc _getDpiForWindow = (_GetDpiForWindowFunc)GetProcAddress(hUser32, "GetDpiForWindow");
        if (_getDpiForWindow) {
            return _getDpiForWindow(hwnd);
        }
    }
    return 96;
}

int ScaleForDpi(int value) {
    return MulDiv(value, dpi, 96);
}

void initTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hTodoIcon;
    strcpy(nid.szTip, "Todo Application");
    
    Shell_NotifyIcon(NIM_ADD, &nid);
    
    hTrayMenu = CreatePopupMenu();
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_SHOW, "Show");
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_HIDE, "Hide");
    AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
}

void removeTrayIcon(void) {
    Shell_NotifyIcon(NIM_DELETE, &nid);
    DestroyMenu(hTrayMenu);
}

void showTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, "Show");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_HIDE, "Hide");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    
    UINT autoStartState = isAutoStartEnabled() ? MF_CHECKED : MF_UNCHECKED;
    AppendMenu(hMenu, MF_STRING | autoStartState, ID_TRAY_AUTOSTART, "Start with Windows");
    
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
    
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
        pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

HWND createModernButton(HWND hParent, const char* text, int x, int y, int width, int height, int id) {
    HWND hButton = CreateWindow(
        "BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, width, height,
        hParent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL
    );
    
    SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    return hButton;
}

HWND createModernEdit(HWND hParent, int x, int y, int width, int height, int id) {
    HWND hEdit = CreateWindow(
        "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        x, y, width, height,
        hParent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL
    );
    
    SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    return hEdit;
}

HWND createModernComboBox(HWND hParent, int x, int y, int width, int height, int id) {
    HWND hCombo = CreateWindow(
        "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, width, height,
        hParent, (HMENU)(INT_PTR)id, GetModuleHandle(NULL), NULL
    );
    
    SendMessage(hCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
    return hCombo;
}

int CALLBACK SortTodo(LPARAM lParam1, LPARAM lParam2, LPARAM lPrarmExtra) {
    int* targetSortingColumn = (int*)lPrarmExtra;
    char buf1[MAX_DESC_LENGTH], buf2[MAX_DESC_LENGTH];
    ZeroMemory(buf1, MAX_DESC_LENGTH);
    ZeroMemory(buf2, MAX_DESC_LENGTH);
    ListView_GetItemText(hListView, lParam1, *targetSortingColumn, buf1, MAX_DESC_LENGTH);
    ListView_GetItemText(hListView, lParam2, *targetSortingColumn, buf2, MAX_DESC_LENGTH);

    return bSortOrder ? strcmp(buf1, buf2) >= 0: strcmp(buf1, buf2) <= 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    switch (uMsg) {
        case WM_DPICHANGED: {
           
            dpi = HIWORD(wParam);
            RECT* newRect = (RECT*)lParam;
            SetWindowPos(hwnd, NULL, newRect->left, newRect->top,
                        newRect->right - newRect->left,
                        newRect->bottom - newRect->top,
                        SWP_NOZORDER | SWP_NOACTIVATE);
            
            DeleteObject(hFont);
            hFont = CreateFont(ScaleForDpi(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hTitleEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hDescEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hPriorityCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hAddButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hEditButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hSaveButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hCancelButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hDeleteButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hCompleteButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        
        case WM_CREATE: {
            
            dpi = _GetDpiForWindow(hwnd);
            hFont = CreateFont(ScaleForDpi(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

            RECT rc;
            GetClientRect(hwnd, &rc);
            int width = rc.right - rc.left;

            initializeSearchBar(hwnd, hFont, width);

            hListView = CreateWindow(
                WC_LISTVIEW, "",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
                ScaleForDpi(10), ScaleForDpi(48), width - ScaleForDpi(20), ScaleForDpi(350),
                hwnd, (HMENU)(INT_PTR)ID_LISTVIEW, GetModuleHandle(NULL), NULL
            );
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);

            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = "ID"; lvc.cx = ScaleForDpi(40); ListView_InsertColumn(hListView, 0, &lvc);
            lvc.pszText = "Title"; lvc.cx = ScaleForDpi(150); ListView_InsertColumn(hListView, 1, &lvc);
            lvc.pszText = "Description"; lvc.cx = ScaleForDpi(200); ListView_InsertColumn(hListView, 2, &lvc);
            lvc.pszText = "Priority"; lvc.cx = ScaleForDpi(80); ListView_InsertColumn(hListView, 3, &lvc);
            lvc.pszText = "Status"; lvc.cx = ScaleForDpi(100); ListView_InsertColumn(hListView, 4, &lvc);
            lvc.pszText = "Created"; lvc.cx = ScaleForDpi(150); ListView_InsertColumn(hListView, 5, &lvc);

            int y = ScaleForDpi(358);
            int x = ScaleForDpi(10);
            int labelW = ScaleForDpi(90), editW = ScaleForDpi(120), gap = ScaleForDpi(10);
            int btnW = ScaleForDpi(80), btnH = ScaleForDpi(28);
            int labelH = ScaleForDpi(18);

            hTitleLabel = CreateWindow("STATIC", "Title:", WS_CHILD | WS_VISIBLE,
                                 x, y, labelW, labelH, hwnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hTitleLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            hTitleEdit = createModernEdit(hwnd, x, y+labelH, editW, btnH, ID_TITLE_EDIT);
            x += editW + gap;

            hDescLabel = CreateWindow("STATIC", "Description:", WS_CHILD | WS_VISIBLE,
                                 x, y, labelW, labelH, hwnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hDescLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            hDescEdit = createModernEdit(hwnd, x, y+labelH, editW, btnH, ID_DESC_EDIT);
            x += editW + gap;

            hAddButton = createModernButton(hwnd, "Add", x, y+labelH, btnW, btnH, ID_ADD_BUTTON);
            x += btnW + gap;

            hPriorityLabel = CreateWindow("STATIC", "Priority:", WS_CHILD | WS_VISIBLE,
                                 x, y, labelW, labelH, hwnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hPriorityLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            hPriorityCombo = createModernComboBox(hwnd, x + labelW + gap, y+labelH, ScaleForDpi(50), btnH, ID_PRIORITY_COMBO);
            for (int i = 1; i <= 5; i++) {
                char priority[10];
                sprintf(priority, "%d", i);
                SendMessage(hPriorityCombo, CB_ADDSTRING, 0, (LPARAM)priority);
            }
            SendMessage(hPriorityCombo, CB_SETCURSEL, 0, 0);
            x += ScaleForDpi(50) + gap;

            int actionY = y + labelH + btnH + gap;
            int actionX = ScaleForDpi(10);
            hEditButton = createModernButton(hwnd, "Edit", actionX, actionY, btnW, btnH, ID_EDIT_BUTTON);
            actionX += btnW + gap;
            hSaveButton = createModernButton(hwnd, "Save", actionX, actionY, btnW, btnH, ID_SAVE_BUTTON);
            actionX += btnW + gap;
            hCancelButton = createModernButton(hwnd, "Cancel", actionX, actionY, btnW, btnH, ID_CANCEL_BUTTON);
            actionX += btnW + gap;
            hDeleteButton = createModernButton(hwnd, "Delete", actionX, actionY, btnW, btnH, ID_DELETE_BUTTON);
            actionX += btnW + gap;
            hCompleteButton = createModernButton(hwnd, "Complete", actionX, actionY, btnW, btnH, ID_COMPLETE_BUTTON);

            ShowWindow(hAddButton, SW_SHOW);
            ShowWindow(hEditButton, SW_SHOW);
            ShowWindow(hDeleteButton, SW_SHOW);
            ShowWindow(hCompleteButton, SW_SHOW);
            ShowWindow(hSaveButton, SW_HIDE);
            ShowWindow(hCancelButton, SW_HIDE);

            EnableWindow(hTitleEdit, TRUE);
            EnableWindow(hDescEdit, TRUE);
            EnableWindow(hPriorityCombo, TRUE);

            initTrayIcon(hwnd);
            loadTodos(&todoList);
            refreshListView();
            break;
        }
        
        case WM_COMMAND: {
            
            switch (LOWORD(wParam)) {
                case ID_ADD_BUTTON:
                    addTodo();
                    break;
                case ID_EDIT_BUTTON:
                    startEditTodo();
                    break;
                case ID_SAVE_BUTTON:
                    saveEditTodo();
                    break;
                case ID_CANCEL_BUTTON:
                    cancelEditTodo();
                    break;
                case ID_DELETE_BUTTON:
                    deleteTodo();
                    break;
                case ID_COMPLETE_BUTTON:
                    completeTodo();
                    break;
                case ID_TRAY_EXIT: 
                    removeTrayIcon();
                    PostQuitMessage(0);
                    break;
                case ID_TRAY_SHOW: 
                    ShowWindow(hwnd, SW_SHOW); 
                    break;
                case ID_TRAY_HIDE: 
                    ShowWindow(hwnd, SW_HIDE); 
                    break;
                case ID_TRAY_AUTOSTART:
                    setAutoStart(!isAutoStartEnabled());
                    break;
                case ID_SEARCH_EDIT:
                    if (HIWORD(wParam) == EN_SETFOCUS) {
                        char text[MAX_TITLE_LENGTH];
                        GetWindowText(hSearchEdit, text, MAX_TITLE_LENGTH);
                        if (strcmp(text, "Search todos...") == 0) {
                            SetWindowText(hSearchEdit, "");
                        }
                    } else if (HIWORD(wParam) == EN_KILLFOCUS) {
                        char text[MAX_TITLE_LENGTH];
                        GetWindowText(hSearchEdit, text, MAX_TITLE_LENGTH);
                        if (strlen(text) == 0) {
                            SetWindowText(hSearchEdit, "Search todos...");
                            refreshListView();
                        }
                    } else if (HIWORD(wParam) == EN_CHANGE) {
                        handleSearchInput(hSearchEdit);
                    }
                    break;
            }
            break;
        }
        
        case WM_TRAYICON: {
            
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP:
                    showTrayMenu(hwnd);
                    break;
                case WM_LBUTTONDBLCLK:
                    ShowWindow(hwnd, IsWindowVisible(hwnd) ? SW_HIDE : SW_SHOW);
                    break;
            }
            break;
        }
        
        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED) {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }

            RECT rc;
            GetClientRect(hwnd, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            MoveWindow(hSearchEdit, ScaleForDpi(10), ScaleForDpi(10), 
                      width - ScaleForDpi(20), ScaleForDpi(28), TRUE);

            int listViewHeight = ScaleForDpi(350);
            MoveWindow(hListView, ScaleForDpi(10), ScaleForDpi(48), 
                      width - ScaleForDpi(20), listViewHeight, TRUE);

            int y = ScaleForDpi(410);
            int x = ScaleForDpi(10);
            int labelW = ScaleForDpi(90), gap = ScaleForDpi(10);
            int minEditW = ScaleForDpi(80), minBtnW = ScaleForDpi(60);
            int btnH = ScaleForDpi(28), labelH = ScaleForDpi(18);
            
            int availableW = width - ScaleForDpi(20);
            int editW = (availableW - labelW*3 - ScaleForDpi(50) - minBtnW*6 - gap*6);
            if (editW < minEditW*2) editW = minEditW*2;
            int singleEditW = editW/2;
            int btnW = (availableW - (labelW*3 + ScaleForDpi(50) + singleEditW*2 + gap*6))/6;
            if (btnW < minBtnW) btnW = minBtnW;

            MoveWindow(hTitleLabel, x, y, labelW, labelH, TRUE);
            MoveWindow(hTitleEdit, x, y+labelH, singleEditW, btnH, TRUE);
            x += singleEditW + gap;

            MoveWindow(hDescLabel, x, y, labelW, labelH, TRUE);
            MoveWindow(hDescEdit, x, y+labelH, singleEditW, btnH, TRUE);
            x += singleEditW + gap;

            MoveWindow(hAddButton, x, y+labelH, btnW, btnH, TRUE);
            x += btnW + gap;

            MoveWindow(hPriorityLabel, x, y, labelW, labelH, TRUE);
            MoveWindow(hPriorityCombo, x + labelW + gap, y+labelH, ScaleForDpi(50), btnH, TRUE);
            x += ScaleForDpi(50) + gap;

            int actionY = y + labelH + btnH + gap;
            int actionX = ScaleForDpi(10);
            MoveWindow(hEditButton, actionX, actionY, btnW, btnH, TRUE);
            actionX += btnW + gap;
            MoveWindow(hSaveButton, actionX, actionY, btnW, btnH, TRUE);
            actionX += btnW + gap;
            MoveWindow(hCancelButton, actionX, actionY, btnW, btnH, TRUE);
            actionX += btnW + gap;
            MoveWindow(hDeleteButton, actionX, actionY, btnW, btnH, TRUE);
            actionX += btnW + gap;
            MoveWindow(hCompleteButton, actionX, actionY, btnW, btnH, TRUE);

            break;
        }
        
        case WM_CLOSE: {
            
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        
        case WM_NOTIFY: {
            
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->code == LVN_ITEMCHANGED && pnmh->idFrom == ID_LISTVIEW) {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                if (pnmv->uNewState & LVIS_SELECTED) {
                    LVITEM lvi = {0};
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = pnmv->iItem;
                    ListView_GetItem(hListView, &lvi);
                    int todoId = (int)lvi.lParam;
                    
                    Todo* todo = findTodoById(todoId);
                    if (todo) {
                        SetWindowText(hTitleEdit, todo->title);
                        SetWindowText(hDescEdit, todo->description);
                        SendMessage(hPriorityCombo, CB_SETCURSEL, todo->priority - 1, 0);
                    }
                }
            }
            if (pnmh->code == LVN_COLUMNCLICK) {
                bSortOrder = !bSortOrder;
                NMLISTVIEW* pnmlv = (NMLISTVIEW*)lParam;
                ListView_SortItemsEx(hListView, SortTodo, &pnmlv->iSubItem);
                return TRUE;
            }
            if (pnmh->code == NM_DBLCLK && pnmh->idFrom == ID_LISTVIEW) {
                startEditTodo();
            }
            break;
        }
        
        case WM_CTLCOLORSTATIC: {
            
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, COLOR_TEXT);
            SetBkColor(hdcStatic, COLOR_BG);
            return (LRESULT)hBgBrush;
        }
        
        case WM_CTLCOLOREDIT: {
           
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, COLOR_TEXT);
            SetBkColor(hdcEdit, RGB(255, 255, 255));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
        
        case WM_CTLCOLORLISTBOX: {
            
            HDC hdcListBox = (HDC)wParam;
            SetTextColor(hdcListBox, COLOR_TEXT);
            SetBkColor(hdcListBox, RGB(255, 255, 255));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
        
        case WM_DESTROY: {
           
            removeTrayIcon();
            DeleteObject(hFont);
            DeleteObject(hBgBrush);
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
} 