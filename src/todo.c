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
int editingIndex = -1;

void initAppData(void) {
    char* appDataPath = getAppDataPath();
    if (!CreateDirectory(appDataPath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, "Failed to create AppData directory!", "Error", MB_ICONERROR);
        exit(1);
    }
    free(appDataPath);
}

char* getAppDataPath(void) {
    char* appDataPath = getenv("APPDATA");
    if (appDataPath == NULL) {
        MessageBox(NULL, "APPDATA environment variable not found!", "Error", MB_ICONERROR);
        exit(1);
    }
    
    char* fullPath = malloc(strlen(appDataPath) + 20);
    sprintf(fullPath, "%s\\TodoApp", appDataPath);
    return fullPath;
}

void loadTodos(TodoList* list) {
    char* appDataPath = getAppDataPath();
    char filePath[MAX_PATH];
    sprintf(filePath, "%s\\%s", appDataPath, DATA_FILE);
    
    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        list->count = 0;
        return;
    }
    
    fread(list, sizeof(TodoList), 1, file);
    fclose(file);
    free(appDataPath);
}

void saveTodos(TodoList* list) {
    char* appDataPath = getAppDataPath();
    char filePath[MAX_PATH];
    sprintf(filePath, "%s\\%s", appDataPath, DATA_FILE);
    
    FILE* file = fopen(filePath, "wb");
    if (file == NULL) {
        MessageBox(NULL, "Failed to save file!", "Error", MB_ICONERROR);
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
    
    for (int i = selectedIndex; i < todoList.count - 1; i++) {
        todoList.todos[i] = todoList.todos[i + 1];
    }
    todoList.count--;
    
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