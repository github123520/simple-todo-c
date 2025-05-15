#include "todo.h"

HWND hMainWindow;
HWND hListView;
HWND hTitleEdit;
HWND hDescEdit;
HWND hPriorityCombo;
HWND hSaveButton;
HWND hCancelButton;
HWND hAddButton;
HWND hEditButton;
HWND hDeleteButton;
HWND hCompleteButton;
TodoList todoList;
HFONT hFont;
HBRUSH hBgBrush;
HICON hTodoIcon;
int editingIndex = -1;

void initAppData(void) {
    char* appDataPath = getAppDataPath();
    if (appDataPath == NULL) {
        MessageBox(NULL, "Failed to get AppData path!", "Error", MB_ICONERROR);
        exit(1);
    }

    
    BOOL created = CreateDirectory(appDataPath, NULL);
    DWORD error = GetLastError();
    
    if (!created && error != ERROR_ALREADY_EXISTS) {
        char errorMsg[256];
        sprintf(errorMsg, "Failed to create directory: %s (Error: %lu)", appDataPath, error);
        MessageBox(NULL, errorMsg, "Error", MB_ICONERROR);
        free(appDataPath);
        exit(1);
    }
    
    free(appDataPath);
}

char* getAppDataPath(void) {
    char* appDataPath = getenv("APPDATA");
    if (appDataPath == NULL) {
        MessageBox(NULL, "APPDATA environment variable not found!", "Error", MB_ICONERROR);
        return NULL;
    }
    
    
    size_t pathLen = strlen(appDataPath) + strlen("\\TodoApp") + 1;
    char* fullPath = malloc(pathLen);
    if (fullPath == NULL) {
        MessageBox(NULL, "Memory allocation failed!", "Error", MB_ICONERROR);
        return NULL;
    }
    
    
    strcpy(fullPath, appDataPath);
    strcat(fullPath, "\\TodoApp");
    
    return fullPath;
}

void loadTodos(TodoList* list) {
    char* appDataPath = getAppDataPath();
    if (appDataPath == NULL) {
        list->count = 0;
        return;
    }

    
    initAppData();
    
    char filePath[MAX_PATH];
    sprintf(filePath, "%s\\%s", appDataPath, DATA_FILE);
    
    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        list->count = 0;
        free(appDataPath);
        return;
    }
    
    fread(list, sizeof(TodoList), 1, file);
    fclose(file);
    free(appDataPath);
}

void saveTodos(TodoList* list) {
    char* appDataPath = getAppDataPath();
    if (appDataPath == NULL) {
        MessageBox(NULL, "Failed to get AppData path!", "Error", MB_ICONERROR);
        return;
    }

    
    initAppData();
    
    char filePath[MAX_PATH];
    sprintf(filePath, "%s\\%s", appDataPath, DATA_FILE);
    
    FILE* file = fopen(filePath, "wb");
    if (file == NULL) {
        char errorMsg[256];
        sprintf(errorMsg, "Failed to save file: %s", filePath);
        MessageBox(NULL, errorMsg, "Error", MB_ICONERROR);
        free(appDataPath);
        return;
    }
    
    fwrite(list, sizeof(TodoList), 1, file);
    fclose(file);
    free(appDataPath);
}

void refreshListView(void) {
    ListView_DeleteAllItems(hListView);
    
    for (int i = 0; i < todoList.count; i++) {
        Todo* todo = &todoList.todos[i];
        char idStr[10];
        char priorityStr[10];
        char statusStr[20];
        char createdStr[30];
        
        sprintf(idStr, "%d", todo->id);
        sprintf(priorityStr, "%d", todo->priority);
        strcpy(statusStr, todo->completed ? "Completed" : "Pending");
        strftime(createdStr, sizeof(createdStr), "%Y-%m-%d %H:%M", localtime(&todo->created_at));
        
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_STATE;
        lvi.iItem = i;
        lvi.state = 0;
        lvi.stateMask = 0;
        
        lvi.iSubItem = 0;
        lvi.pszText = idStr;
        ListView_InsertItem(hListView, &lvi);
        
        ListView_SetItemText(hListView, i, 1, todo->title);
        ListView_SetItemText(hListView, i, 2, todo->description);
        ListView_SetItemText(hListView, i, 3, priorityStr);
        ListView_SetItemText(hListView, i, 4, statusStr);
        ListView_SetItemText(hListView, i, 5, createdStr);
    }
}

void addTodo(void) {
    if (todoList.count >= MAX_TODOS) {
        MessageBox(NULL, "Maximum number of todos reached!", "Error", MB_ICONERROR);
        return;
    }
    
    char title[MAX_TITLE_LENGTH];
    char description[MAX_DESC_LENGTH];
    int priority;
    
    GetWindowText(hTitleEdit, title, MAX_TITLE_LENGTH);
    GetWindowText(hDescEdit, description, MAX_DESC_LENGTH);
    priority = SendMessage(hPriorityCombo, CB_GETCURSEL, 0, 0) + 1;
    
    if (strlen(title) == 0) {
        MessageBox(NULL, "Please enter a title!", "Error", MB_ICONERROR);
        return;
    }
    
    Todo* todo = &todoList.todos[todoList.count];
    todo->id = todoList.count + 1;
    strcpy(todo->title, title);
    strcpy(todo->description, description);
    todo->priority = priority;
    todo->completed = 0;
    todo->created_at = time(NULL);
    todo->completed_at = 0;
    
    todoList.count++;
    saveTodos(&todoList);
    refreshListView();
    
    SetWindowText(hTitleEdit, "");
    SetWindowText(hDescEdit, "");
    SendMessage(hPriorityCombo, CB_SETCURSEL, 0, 0);
}

void startEditTodo(void) {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        MessageBox(NULL, "Please select a todo to edit!", "Error", MB_ICONERROR);
        return;
    }
    
    if (editingIndex != -1) {
        MessageBox(NULL, "Please finish editing the current todo first!", "Error", MB_ICONERROR);
        return;
    }
    
    Todo* todo = &todoList.todos[selectedIndex];
    editingIndex = selectedIndex;
    
    SetWindowText(hTitleEdit, todo->title);
    SetWindowText(hDescEdit, todo->description);
    SendMessage(hPriorityCombo, CB_SETCURSEL, todo->priority - 1, 0);
    
    EnableWindow(hTitleEdit, TRUE);
    EnableWindow(hDescEdit, TRUE);
    EnableWindow(hPriorityCombo, TRUE);
    
    ShowWindow(hSaveButton, SW_SHOW);
    ShowWindow(hCancelButton, SW_SHOW);
    
    SetFocus(hTitleEdit);
}

void saveEditTodo(void) {
    if (editingIndex == -1) {
        return;
    }
    
    char title[MAX_TITLE_LENGTH];
    char description[MAX_DESC_LENGTH];
    
    GetWindowText(hTitleEdit, title, MAX_TITLE_LENGTH);
    GetWindowText(hDescEdit, description, MAX_DESC_LENGTH);
    int priority = SendMessage(hPriorityCombo, CB_GETCURSEL, 0, 0) + 1;
    
    if (strlen(title) == 0) {
        MessageBox(NULL, "Title cannot be empty!", "Error", MB_ICONERROR);
        return;
    }
    
    Todo* todo = &todoList.todos[editingIndex];
    strcpy(todo->title, title);
    strcpy(todo->description, description);
    todo->priority = priority;
    
    saveTodos(&todoList);
    refreshListView();
    
    SetWindowText(hTitleEdit, "");
    SetWindowText(hDescEdit, "");
    SendMessage(hPriorityCombo, CB_SETCURSEL, 0, 0);
    
    EnableWindow(hTitleEdit, FALSE);
    EnableWindow(hDescEdit, FALSE);
    EnableWindow(hPriorityCombo, FALSE);
    
    ShowWindow(hSaveButton, SW_HIDE);
    ShowWindow(hCancelButton, SW_HIDE);
    
    editingIndex = -1;
    EnableWindow(hTitleEdit, TRUE);
    EnableWindow(hDescEdit, TRUE);
    EnableWindow(hPriorityCombo, TRUE);
}

void cancelEditTodo(void) {
    if (editingIndex == -1) {
        return;
    }
    
    SetWindowText(hTitleEdit, "");
    SetWindowText(hDescEdit, "");
    SendMessage(hPriorityCombo, CB_SETCURSEL, 0, 0);
    
    EnableWindow(hTitleEdit, FALSE);
    EnableWindow(hDescEdit, FALSE);
    EnableWindow(hPriorityCombo, FALSE);
    
    ShowWindow(hSaveButton, SW_HIDE);
    ShowWindow(hCancelButton, SW_HIDE);
    
    editingIndex = -1;
    
    
    EnableWindow(hTitleEdit, TRUE);
    EnableWindow(hDescEdit, TRUE);
    EnableWindow(hPriorityCombo, TRUE);
}

void deleteTodo(void) {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        MessageBox(NULL, "Please select a todo to delete!", "Error", MB_ICONERROR);
        return;
    }
    char szTaskId[10];
    ZeroMemory(szTaskId, 10);
    int iTaskToRemve[MAX_TODOS];
    int iCntToRemove = 0;
    
    ListView_GetItemText(hListView, selectedIndex, 0, szTaskId, 10);
    while (selectedIndex != -1) {
        iTaskToRemve[iCntToRemove++] = atoi(szTaskId);
        selectedIndex = ListView_GetNextItem(hListView, selectedIndex, LVNI_SELECTED);
        ListView_GetItemText(hListView, selectedIndex, 0, szTaskId, 10);
    }

    for (int i = 0; i < iCntToRemove; i++) {
        for (int j = 0; j < todoList.count;j++) {
            // find real id to delete and delete task
            if (todoList.todos[j].id == iTaskToRemve[i]) {
                for (int k = j; k < todoList.count - 1; k++) {
                    todoList.todos[k] = todoList.todos[k + 1];
                }
                todoList.count--;
                break;
            }
        }
    }
    
    saveTodos(&todoList);
    refreshListView();
}

void completeTodo(void) {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        MessageBox(NULL, "Please select a todo to complete!", "Error", MB_ICONERROR);
        return;
    }
    
    Todo* todo = &todoList.todos[selectedIndex];
    todo->completed = 1;
    todo->completed_at = time(NULL);
    
    saveTodos(&todoList);
    refreshListView();
}

void setAutoStart(BOOL enable) {
    HKEY hKey;
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        
        if (enable) {
            RegSetValueEx(hKey, "TodoApp", 0, REG_SZ, 
                (BYTE*)exePath, strlen(exePath) + 1);
        } else {
            RegDeleteValue(hKey, "TodoApp");
        }
        
        RegCloseKey(hKey);
    }
}

BOOL isAutoStartEnabled(void) {
    HKEY hKey;
    char value[MAX_PATH];
    DWORD valueSize = sizeof(value);
    BOOL result = FALSE;
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        
        if (RegQueryValueEx(hKey, "TodoApp", NULL, NULL, 
            (BYTE*)value, &valueSize) == ERROR_SUCCESS) {
            result = TRUE;
        }
        
        RegCloseKey(hKey);
    }
    
    return result;
} 