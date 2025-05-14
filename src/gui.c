#include "todo.h"

NOTIFYICONDATA nid;
HMENU hTrayMenu;
BOOL bSortOrder = TRUE;

void initTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
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
        case WM_CREATE: {
            hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

            RECT rc;
            GetClientRect(hwnd, &rc);
            int width = rc.right - rc.left;

            // ListView: Top, full width, height 300
            hListView = CreateWindow(
                WC_LISTVIEW, "",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
                10, 10, width - 20, 300,
                hwnd, (HMENU)(INT_PTR)ID_LISTVIEW, GetModuleHandle(NULL), NULL
            );
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);

            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = "ID"; lvc.cx = 40; ListView_InsertColumn(hListView, 0, &lvc);
            lvc.pszText = "Title"; lvc.cx = 150; ListView_InsertColumn(hListView, 1, &lvc);
            lvc.pszText = "Description"; lvc.cx = 200; ListView_InsertColumn(hListView, 2, &lvc);
            lvc.pszText = "Priority"; lvc.cx = 80; ListView_InsertColumn(hListView, 3, &lvc);
            lvc.pszText = "Status"; lvc.cx = 100; ListView_InsertColumn(hListView, 4, &lvc);
            lvc.pszText = "Created"; lvc.cx = 150; ListView_InsertColumn(hListView, 5, &lvc);

            int y = 320;
            int x = 10;
            int labelW = 50, editW = 120, gap = 10, btnW = 80, btnH = 28;

            HWND hStatic;
            hStatic = CreateWindow("STATIC", "Title:", WS_CHILD | WS_VISIBLE,
                                 x, y+5, labelW, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
            x += labelW;
            hTitleEdit = createModernEdit(hwnd, x, y, editW, btnH, ID_TITLE_EDIT);
            x += editW + gap;

            hStatic = CreateWindow("STATIC", "Description:", WS_CHILD | WS_VISIBLE,
                                 x, y+5, labelW+20, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
            x += labelW+20;
            hDescEdit = createModernEdit(hwnd, x, y, editW, btnH, ID_DESC_EDIT);
            x += editW + gap;

            hStatic = CreateWindow("STATIC", "Priority:", WS_CHILD | WS_VISIBLE,
                                 x, y+5, labelW, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
            x += labelW;
            hPriorityCombo = createModernComboBox(hwnd, x, y, 50, btnH, ID_PRIORITY_COMBO);
            for (int i = 1; i <= 5; i++) {
                char priority[10];
                sprintf(priority, "%d", i);
                SendMessage(hPriorityCombo, CB_ADDSTRING, 0, (LPARAM)priority);
            }
            SendMessage(hPriorityCombo, CB_SETCURSEL, 0, 0);
            x += 50 + gap;

            
            hAddButton = createModernButton(hwnd, "Add", x, y, btnW, btnH, ID_ADD_BUTTON);

            
            int actionY = y + btnH + gap;
            int actionX = 10;
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
                    int selectedIndex = pnmv->iItem;
                    if (selectedIndex >= 0 && selectedIndex < todoList.count) {
                        Todo* todo = &todoList.todos[selectedIndex];
                        SetWindowText(hTitleEdit, todo->title);
                        SetWindowText(hDescEdit, todo->description);
                        SendMessage(hPriorityCombo, CB_SETCURSEL, todo->priority - 1, 0);
                    }
                }
            }
            if (pnmh->code == LVN_COLUMNCLICK) {
                bSortOrder = !bSortOrder;
                NMLISTVIEW* pnmlv = (NMLISTVIEW*)lParam;
                ListView_SortItemsEx(hListView, SortTodo,  &pnmlv->iSubItem);
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
        
        case WM_DESTROY:
            removeTrayIcon();
            DeleteObject(hFont);
            DeleteObject(hBgBrush);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
} 