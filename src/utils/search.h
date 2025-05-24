#ifndef SEARCH_H
#define SEARCH_H

#include "../todo.h"

extern int ScaleForDpi(int value);

void filterTodosBySearch(const char* searchText);
BOOL todoMatchesSearch(const Todo* todo, const char* searchText);
void initializeSearchBar(HWND hwnd, HFONT hFont, int width);
void handleSearchInput(HWND hSearchEdit);

extern HWND hSearchEdit;

#endif 