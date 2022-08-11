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

TCHAR* escape(TCHAR* buffer, DWORD bufferSize) {
	int i;
	TCHAR* dest = (TCHAR*)calloc(bufferSize * 2, sizeof(TCHAR));
	TCHAR* ptr = dest;
	for (i = 0; i<bufferSize; i++) {
		TCHAR src = buffer[i];
			if (src == '\\' || src == '\"') {
				*ptr++ = '\\';
			}
			*ptr++ = src;
	}
	return dest;
}

// TODO: replace "/" by "//" in keys and values to be compliant with JSON spec.
void Dump(HKEY rootKey, TCHAR *rootKeyName, boolean isFirst)
{
	if (!isFirst)
	{
		debug(",");
	}
	debug("{\n");
	debug("\t\"name\":\""); OutputDebugString(rootKeyName); debug("\"\n");

	Win32Api win32Api;

	TCHAR ClassName[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD ClassNameSize = 0; // size of class string
	DWORD NbSubKeys = 0; // number of subkeys 
	DWORD MaxSubKeyNameLength; // longest subkey size
	DWORD MaxClassNameLength; // longest class string 
	DWORD NbValues; // number of values for key
	DWORD MaxValueNameLength; // longest value name
	DWORD MaxValueDataLength; // longest value data
	DWORD SecurityDescriptorSize; // size of security descriptor
	FILETIME LastWriteTime; // last write time

	TCHAR SubKeyName[MAX_KEY_LENGTH]; // buffer for subkey name
	DWORD SubKeyNameSize = MAX_KEY_LENGTH; // size of name string

	TCHAR ValueName[MAX_VALUE_NAME]; // buffer for value name
	DWORD ValueNameSize = MAX_VALUE_NAME; // size of name string
	DWORD ValueType;


	DWORD retCode = win32Api.RegQueryInfoKeyW(rootKey, ClassName, &ClassNameSize, NULL, &NbSubKeys, &MaxSubKeyNameLength, &MaxClassNameLength, 
		&NbValues, &MaxValueNameLength, &MaxValueDataLength, &SecurityDescriptorSize, &LastWriteTime);

	PBYTE ValueData = new BYTE[MaxValueDataLength];
	DWORD ValueDataSize = MaxValueDataLength;

	if (NbSubKeys)
	{
		debug("\t,\"keys\":[\n");

		for (DWORD i = 0; i<NbSubKeys; i++)
		{
			SubKeyNameSize = MAX_KEY_LENGTH;
			retCode = win32Api.RegEnumKeyExW(rootKey, i,
				SubKeyName,
				&SubKeyNameSize,
				NULL,
				NULL,
				NULL,
				&LastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				HKEY subKey = {};
				retCode = win32Api.RegOpenKeyExW(rootKey, SubKeyName, 0, KEY_READ, &subKey);
				if (retCode == ERROR_SUCCESS)
				{
					Dump(subKey, SubKeyName, i==0);
				}
			}
		}
		debug("]\n");
	}


	if (NbValues)
	{
		debug("\t,\"values\":[\n");

		for (DWORD i = 0, retCode = ERROR_SUCCESS; i<NbValues; i++)
		{
			ValueNameSize = MAX_VALUE_NAME;
			ValueName[0] = '\0';
			retCode = win32Api.RegEnumValueW(rootKey, i,
				ValueName,
				&ValueNameSize,
				NULL,
				&ValueType,
				ValueData, 
				&ValueDataSize);

			if (retCode == ERROR_SUCCESS)
			{
				if (i != 0)
				{
					debug(",");
				}
				debug("{\n");
				TCHAR* escapedValueName = escape((TCHAR*)ValueName, ValueNameSize);
				debug("\t\"name\":\""); OutputDebugString(escapedValueName); debug("\"\n");
				debug("\t,\"value_type\":");

				if (REG_SZ == ValueType)
				{
					debug("\"REG_SZ\"\n");
					TCHAR* escapedValueData = escape((TCHAR*)ValueData, ValueDataSize);
					debug("\t,\"value\":\""); OutputDebugString((LPCWSTR)escapedValueData); debug("\"\n");
				} 
				else if (REG_DWORD == ValueType)
				{
					debug("\"REG_DWORD\"\n");
					debug("\t,\"value\":\"0x%08X\"\n", *(PDWORD)ValueData);
				}
				else if (REG_MULTI_SZ == ValueType)
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
