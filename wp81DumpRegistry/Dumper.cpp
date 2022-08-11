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

void Dump(HKEY rootKey, TCHAR *rootKeyName, boolean isFirst)
{
	if (!isFirst)
	{
		debug(",");
	}
	debug("{\n");
	debug("\t\"name\":\""); OutputDebugString(rootKeyName); debug("\"\n");

	Win32Api win32Api;

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

	TCHAR achKey[MAX_KEY_LENGTH]; // buffer for subkey name
	DWORD cbName = MAX_KEY_LENGTH; // size of name string

	TCHAR achValue[MAX_VALUE_NAME]; // buffer for value name
	DWORD cchValue = MAX_VALUE_NAME; // size of name string
	DWORD valueType;


	DWORD retCode = win32Api.RegQueryInfoKeyW(rootKey, achClass, &cchClassName, NULL, &cSubKeys, &cbMaxSubKey, &cchMaxClass, 
		&cValues, &cchMaxValue, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);

	PBYTE valueBuf = new BYTE[cbMaxValueData];
	DWORD valueBufSize = cbMaxValueData;

	if (cSubKeys)
	{
		debug("\t,\"keys\":[\n");

		for (DWORD i = 0; i<cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = win32Api.RegEnumKeyExW(rootKey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				HKEY subKey = {};
				retCode = win32Api.RegOpenKeyExW(rootKey, achKey, 0, KEY_READ, &subKey);
				if (retCode == ERROR_SUCCESS)
				{
					Dump(subKey, achKey, i==0);
				}
			}
		}
		debug("]\n");
	}


	if (cValues)
	{
		debug("\t,\"values\":[\n");

		for (DWORD i = 0, retCode = ERROR_SUCCESS; i<cValues; i++)
		{
			cchValue = MAX_VALUE_NAME;
			achValue[0] = '\0';
			retCode = win32Api.RegEnumValueW(rootKey, i,
				achValue,
				&cchValue,
				NULL,
				&valueType,
				valueBuf, 
				&valueBufSize);

			if (retCode == ERROR_SUCCESS)
			{
				if (i != 0)
				{
					debug(",");
				}
				debug("{\n");
				debug("\t\"name\":\""); OutputDebugString(achValue); debug("\"\n");
				debug("\t,\"value_type\":");

				if (REG_SZ == valueType)
				{
					debug("\"REG_SZ\"\n");
					debug("\t,\"value\":\"\"\n");
				} 
				else if (REG_DWORD == valueType)
				{
					debug("\"REG_DWORD\"\n");
					debug("\t,\"value\":\"%08X\"\n", *(PDWORD)valueBuf);
				}
				else if (REG_MULTI_SZ == valueType)
				{
					debug("\"REG_MULTI_SZ\"\n");
					debug("\t,\"value\":\"\"\n");
				}
				else
				{
					debug("\"unknown type\"\n");
				}

				debug("}\n");
			}
		}
		debug("]\n");
	}

	win32Api.RegCloseKey(rootKey);
	debug("}\n");
}
