#ifndef TODO_H
#define TODO_H

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uxtheme.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_TODOS 100
#define MAX_TITLE_LENGTH 100
#define MAX_DESC_LENGTH 500
#define DATA_FILE "todos.dat"

#define ID_ADD_BUTTON 1
#define ID_EDIT_BUTTON 2
#define ID_DELETE_BUTTON 3
#define ID_COMPLETE_BUTTON 4
#define ID_LISTVIEW 5
#define ID_TITLE_EDIT 6
#define ID_DESC_EDIT 7
#define ID_PRIORITY_COMBO 8
#define ID_SAVE_BUTTON 9
#define ID_CANCEL_BUTTON 10
#define ID_SEARCH_EDIT 11

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ICON 100
#define ID_TRAY_EXIT 101
#define ID_TRAY_SHOW 102
#define ID_TRAY_HIDE 103
#define ID_TRAY_AUTOSTART 104

#define COLOR_BG RGB(240, 240, 240)
#define COLOR_ACCENT RGB(0, 120, 215)
#define COLOR_TEXT RGB(51, 51, 51)
#define COLOR_BUTTON_BG RGB(0, 120, 215)
#define COLOR_BUTTON_TEXT RGB(255, 255, 255)
#define COLOR_BUTTON_HOVER RGB(0, 100, 195)
#define COLOR_BUTTON_PRESSED RGB(0, 80, 175)

#define IDI_ICON1 400

typedef struct {
    int id;
    char title[MAX_TITLE_LENGTH];
    char description[MAX_DESC_LENGTH];
    int priority;
    int completed;
    time_t created_at;
    time_t completed_at;
} Todo;

typedef struct {
    Todo todos[MAX_TODOS];
    int count;
} TodoList;

extern HWND hMainWindow;
extern HWND hListView;
extern HWND hTitleEdit;
extern HWND hDescEdit;
extern HWND hPriorityCombo;
extern HWND hSaveButton;
extern HWND hCancelButton;
extern HWND hAddButton;
extern HWND hEditButton;
extern HWND hDeleteButton;
extern HWND hCompleteButton;
extern TodoList todoList;
extern HFONT hFont;
extern HBRUSH hBgBrush;
extern NOTIFYICONDATA nid;
extern HMENU hTrayMenu;
extern int editingIndex;
extern HICON hTodoIcon;

void initAppData(void);
char* getAppDataPath(void);
void loadTodos(TodoList* list);
void saveTodos(TodoList* list);
void refreshListView(void);
void addTodo(void);
void startEditTodo(void);
void saveEditTodo(void);
void cancelEditTodo(void);
void deleteTodo(void);
void completeTodo(void);
HWND createModernButton(HWND parent, const char* text, int x, int y, int width, int height, int id);
HWND createModernEdit(HWND parent, int x, int y, int width, int height, int id);
HWND createModernComboBox(HWND parent, int x, int y, int width, int height, int id);
void initTrayIcon(HWND hwnd);
void removeTrayIcon(void);
void showTrayMenu(HWND hwnd);
void setAutoStart(BOOL enable);
BOOL isAutoStartEnabled(void);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
Todo* findTodoById(int id);

#endif 