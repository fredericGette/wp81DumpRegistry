#include "pch.h"
#include "Dumper.h"
#include "Win32Api.h"

void debug(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[100];
	vsnprintf_s(buffer, sizeof(buffer), format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

void Dump(HKEY rootKey)
{
	Win32Api win32Api;

	TCHAR achKey[255]; // buffer for subkey name
	DWORD cbName = 255; // size of name string
	TCHAR achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD cchClassName = 0; // size of class string
	DWORD cSubKeys = 0; // number of subkeys 
	DWORD cbMaxSubKey; // longest subkey size
	DWORD cchMaxClass; // longest class string 
	DWORD cValues; // number of values for key
	DWORD cchMaxValue; // longest value name
	DWORD cbMaxValueData; // longest value data
	DWORD cbSecurityDescriptor; // size of security descriptor
	FILETIME ftLastWriteTime; // last write time
	DWORD retCode = win32Api.RegQueryInfoKeyW(rootKey, achClass, &cchClassName, NULL, &cSubKeys, &cbMaxSubKey, &cchMaxClass, &cValues, &cchMaxValue, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);

	if (cSubKeys)
	{
		debug("\nNumber of subkeys: %d\n", cSubKeys);

		for (DWORD i = 0; i<cSubKeys; i++)
		{
			cbName = 255;
			retCode = win32Api.RegEnumKeyExW(rootKey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				debug("(%d)", i + 1); OutputDebugString(achKey); debug("\n");
			}
		}
	}

	TCHAR achValue[16383];
	DWORD cchValue = 16383;

	if (cValues)
	{
		debug("\nNumber of values: %d\n", cValues);


		for (DWORD i = 0, retCode = ERROR_SUCCESS; i<cValues; i++)
		{
			cchValue = 16383;
			achValue[0] = '\0';
			retCode = win32Api.RegEnumValueW(rootKey, i,
				achValue,
				&cchValue,
				NULL,
				NULL,
				NULL,
				NULL);

			if (retCode == ERROR_SUCCESS)
			{
				debug("(%d)", i + 1); OutputDebugString(achValue); debug("\n");
			}
		}
	}
}