#include "todo.h"
#include "utils/search.h"

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
volatile BOOL g_errorBoxOpen = FALSE;

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
    char searchText[MAX_TITLE_LENGTH];
    GetWindowText(hSearchEdit, searchText, MAX_TITLE_LENGTH);
    
    if (strlen(searchText) > 0 && strcmp(searchText, "Search todos...") != 0) {
        filterTodosBySearch(searchText);
        return;
    }
    
    ListView_DeleteAllItems(hListView);
    
    for (int i = 0; i < todoList.count; i++) {
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

void showErrorBox(const char* message, const char* title) {
    if (g_errorBoxOpen) return;
    g_errorBoxOpen = TRUE;
    MessageBox(NULL, message, title, MB_ICONERROR);
    g_errorBoxOpen = FALSE;
}

void addTodo(void) {
    if (todoList.count >= MAX_TODOS) {
        showErrorBox("Maximum number of todos reached!", "Error");
        return;
    }
    
    char title[MAX_TITLE_LENGTH];
    char description[MAX_DESC_LENGTH];
    int priority;
    
    GetWindowText(hTitleEdit, title, MAX_TITLE_LENGTH);
    GetWindowText(hDescEdit, description, MAX_DESC_LENGTH);
    priority = SendMessage(hPriorityCombo, CB_GETCURSEL, 0, 0) + 1;
    
    if (strlen(title) == 0) {
        showErrorBox("Please enter a title!", "Error");
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

Todo* findTodoById(int id) {
    for (int i = 0; i < todoList.count; i++) {
        if (todoList.todos[i].id == id) {
            return &todoList.todos[i];
        }
    }
    return NULL;
}

void startEditTodo(void) {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        showErrorBox("Please select a todo to edit!", "Error");
        return;
    }
    if (editingIndex != -1) {
        showErrorBox("Please finish editing the current todo first!", "Error");
        return;
    }
    
    LVITEM lvi = {0};
    lvi.mask = LVIF_PARAM;
    lvi.iItem = selectedIndex;
    ListView_GetItem(hListView, &lvi);
    int todoId = (int)lvi.lParam;
    
    Todo* todo = findTodoById(todoId);
    if (!todo) {
        showErrorBox("Selected todo not found!", "Error");
        return;
    }
    
    editingIndex = todoId;
    
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
    
    Todo* todo = findTodoById(editingIndex);
    if (!todo) {
        showErrorBox("Todo not found!", "Error");
        return;
    }
    
    char title[MAX_TITLE_LENGTH];
    char description[MAX_DESC_LENGTH];
    
    GetWindowText(hTitleEdit, title, MAX_TITLE_LENGTH);
    GetWindowText(hDescEdit, description, MAX_DESC_LENGTH);
    int priority = SendMessage(hPriorityCombo, CB_GETCURSEL, 0, 0) + 1;
    
    if (strlen(title) == 0) {
        showErrorBox("Title cannot be empty!", "Error");
        return;
    }
    
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
        showErrorBox("Please select a todo to delete!", "Error");
        return;
    }
    
    while (selectedIndex != -1) {
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = selectedIndex;
        ListView_GetItem(hListView, &lvi);
        int todoId = (int)lvi.lParam;
        
        for (int i = 0; i < todoList.count; i++) {
            if (todoList.todos[i].id == todoId) {
                for (int j = i; j < todoList.count - 1; j++) {
                    todoList.todos[j] = todoList.todos[j + 1];
                }
                todoList.count--;
                break;
            }
        }
        
        selectedIndex = ListView_GetNextItem(hListView, selectedIndex, LVNI_SELECTED);
    }
    
    saveTodos(&todoList);
    refreshListView();
}

void completeTodo(void) {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        showErrorBox("Please select a todo to complete!", "Error");
        return;
    }
    
    while (selectedIndex != -1) {
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = selectedIndex;
        ListView_GetItem(hListView, &lvi);
        int todoId = (int)lvi.lParam;
        
        Todo* todo = findTodoById(todoId);
        if (todo) {
            todo->completed = 1;
            todo->completed_at = time(NULL);
        }
        
        selectedIndex = ListView_GetNextItem(hListView, selectedIndex, LVNI_SELECTED);
    }
    
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