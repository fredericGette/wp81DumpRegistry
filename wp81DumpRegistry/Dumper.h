#pragma once

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define MAX_BINARY_DISPLAY 100

void Dump2File(HKEY rootKey, WCHAR *rootKeyName, WCHAR* folderPath);