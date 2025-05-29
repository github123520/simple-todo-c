#include "search.h"
#include <string.h>

extern int ScaleForDpi(int value);
HWND hSearchEdit;

void initializeSearchBar(HWND hwnd, HFONT hFont, int width) {
    hSearchEdit = CreateWindow(
        "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        ScaleForDpi(10), ScaleForDpi(10), width - ScaleForDpi(20), ScaleForDpi(28),
        hwnd, (HMENU)(INT_PTR)ID_SEARCH_EDIT, GetModuleHandle(NULL), NULL
    );
    SendMessage(hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SetWindowText(hSearchEdit, "Search todos...");
}

void handleSearchInput(HWND hSearchEdit) {
    char text[MAX_TITLE_LENGTH];
    GetWindowText(hSearchEdit, text, MAX_TITLE_LENGTH);
    
    if (strcmp(text, "Search todos...") != 0) {
        filterTodosBySearch(text);
    }
}

void filterTodosBySearch(const char* searchText) {
    ListView_DeleteAllItems(hListView);
    
    char lowerSearchText[MAX_TITLE_LENGTH];
    strcpy(lowerSearchText, searchText);
    _strlwr(lowerSearchText);
    
    for (int i = 0; i < todoList.count; i++) {
        if (todoMatchesSearch(&todoList.todos[i], lowerSearchText)) {
            char idStr[10], priorityStr[10], statusStr[20], createdStr[50];
            sprintf(idStr, "%d", todoList.todos[i].id);
            sprintf(priorityStr, "%d", todoList.todos[i].priority);
            strcpy(statusStr, todoList.todos[i].completed ? "Completed" : "Pending");
            
            struct tm* timeinfo = localtime(&todoList.todos[i].created_at);
            strftime(createdStr, sizeof(createdStr), "%Y-%m-%d %H:%M", timeinfo);
            
            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = idStr;
            lvi.lParam = todoList.todos[i].id;
            
            int idx = ListView_InsertItem(hListView, &lvi);
            ListView_SetItemText(hListView, idx, 1, todoList.todos[i].title);
            ListView_SetItemText(hListView, idx, 2, todoList.todos[i].description);
            ListView_SetItemText(hListView, idx, 3, priorityStr);
            ListView_SetItemText(hListView, idx, 4, statusStr);
            ListView_SetItemText(hListView, idx, 5, createdStr);
        }
    }
}

BOOL todoMatchesSearch(const Todo* todo, const char* searchText) {
    if (!searchText || strlen(searchText) == 0) {
        return TRUE;
    }
    
    char lowerTitle[MAX_TITLE_LENGTH];
    char lowerDesc[MAX_DESC_LENGTH];
    
    strcpy(lowerTitle, todo->title);
    strcpy(lowerDesc, todo->description);
    _strlwr(lowerTitle);
    _strlwr(lowerDesc);
    
    return strstr(lowerTitle, searchText) || strstr(lowerDesc, searchText);
} 