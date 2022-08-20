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

WCHAR* escape(WCHAR* buffer, DWORD bufferSize) {
	WCHAR* dest = (WCHAR*)calloc(bufferSize * 2 + 1, sizeof(WCHAR)); // +1 allow space for a secure /0 in case of bufferSize = 0
	WCHAR* ptr = dest;
	for (DWORD i = 0; i<bufferSize; i++) {
		WCHAR src = buffer[i];
		if (src == L'\\' || src == L'\"') {
			*ptr++ = L'\\';
		}
		if (src != L'\0' && src < L' ') {
			src = L' ';
		}

		*ptr++ = src;
	}
	*ptr++ = L'\0'; // secure /0 in case of bufferSize = 0
	return dest;
}

// TODO: replace "/" by "//" in keys and values to be compliant with JSON spec.
void Dump(HKEY rootKey, WCHAR *rootKeyName, boolean isFirst)
{
	if (!isFirst)
	{
		debug(",");
	}
	debug("{\n");
	debug("\t\"name\":\""); OutputDebugString(rootKeyName); debug("\"\n");

	Win32Api win32Api;

	WCHAR ClassName[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD ClassNameSize = 0; // size of class string
	DWORD NbSubKeys = 0; // number of subkeys 
	DWORD MaxSubKeyNameLength; // longest subkey size
	DWORD MaxClassNameLength; // longest class string 
	DWORD NbValues; // number of values for key
	DWORD MaxValueNameLength; // longest value name
	DWORD MaxValueDataLength; // longest value data
	DWORD SecurityDescriptorSize; // size of security descriptor
	FILETIME LastWriteTime; // last write time

	WCHAR SubKeyName[MAX_KEY_LENGTH]; // buffer for subkey name
	DWORD SubKeyNameSize = MAX_KEY_LENGTH; // size of name string

	WCHAR ValueName[MAX_VALUE_NAME]; // buffer for value name
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
			ValueName[0] = L'\0';
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
				WCHAR* escapedValueName = escape((WCHAR*)ValueName, ValueNameSize);
				debug("\t\"name\":\""); OutputDebugStringW(escapedValueName); debug("\"\n");
				debug("\t,\"value_type\":");

				if (REG_SZ == ValueType)
				{
					debug("\"REG_SZ\"\n");
					WCHAR* escapedValueData = escape((WCHAR*)ValueData, ValueDataSize/sizeof(WCHAR)); // ValueDataSize is a number of BYTE
					debug("\t,\"value\":\""); OutputDebugStringW(escapedValueData); debug("\"\n");
				} 
				else if (REG_DWORD == ValueType)
				{
					debug("\"REG_DWORD\"\n");
					debug("\t,\"value\":\"0x%08X\"\n", *(PDWORD)ValueData);
				}
				else if (REG_MULTI_SZ == ValueType)
				{
					debug("\"REG_MULTI_SZ\"\n");
					debug("\t,\"value\":[\n");
					WCHAR* c = (WCHAR*)ValueData;
					WCHAR* value = nullptr;
					DWORD valueSize = 0;
					WCHAR* escapedValue = nullptr;
					boolean isFirstString = true;
					do
					{
						if (isFirstString) 
						{ 
							isFirstString = false;
						}
						else
						{
							debug(",");
						}
						value = c;
						valueSize = 0;
						while (*c != L'\0')
						{
							c++;
							valueSize++;
						}
						c++; // skip \0
						escapedValue = escape(value, valueSize);
						debug("\t\""); OutputDebugStringW(escapedValue); debug("\"\n");
					} while (*c != L'\0');
					debug("]\n");
				}
				else if (REG_EXPAND_SZ == ValueType)
				{
					debug("\"REG_EXPAND_SZ\"\n");
					WCHAR* escapedValueData = escape((WCHAR*)ValueData, ValueDataSize/sizeof(WCHAR)); // ValueDataSize is a number of BYTE
					debug("\t,\"value\":\""); OutputDebugStringW(escapedValueData); debug("\"\n");
				}
				else if (REG_QWORD == ValueType)
				{
					debug("\"REG_QWORD\"\n");
					unsigned long long value = *ValueData;
					debug("\t,\"value\":\"0x%016llx\"\n", value);
				}
				else if (REG_NONE == ValueType)
				{
					debug("\"REG_NONE\"\n");
				}
				else if (REG_BINARY == ValueType)
				{
					debug("\"REG_BINARY\"\n");
					debug("\t,\"value_size_in_bytes\":\"%dbytes\"\n", ValueDataSize);
					debug("\t,\"value\":\"", ValueDataSize);
					DWORD offset = 0;
					while (offset < ValueDataSize && offset < MAX_BINARY_DISPLAY)
					{
						debug("%02X ", *(ValueData+offset));
						offset++;
					}
					debug("\"\n");
					debug("\t,\"value_char\":\"", ValueDataSize);
					offset = 0;
					char c = 0;
					while (offset < ValueDataSize && offset < MAX_BINARY_DISPLAY)
					{
						c = *(ValueData + offset);
						if (c < ' ')
						{
							c = '.';
						}
						if (c == '\\' || c == '\"')
						{
							debug("\\");
						}
						debug("%c", c);
						offset++;
					}
					debug("\"\n");
				}
				else 
				{
					debug("\"unknown type %d\"\n", ValueType);
					debug("\t,\"value_size_in_bytes\":\"%dbytes\"\n", ValueDataSize);
					debug("\t,\"value\":\"", ValueDataSize);
					DWORD offset = 0;
					while (offset < ValueDataSize && offset < MAX_BINARY_DISPLAY)
					{
						debug("%02X ", *(ValueData + offset));
						offset++;
					}
					debug("\"\n");
					debug("\t,\"value_char\":\"", ValueDataSize);
					offset = 0;
					char c = 0;
					while (offset < ValueDataSize && offset < MAX_BINARY_DISPLAY)
					{
						c = *(ValueData + offset);
						if (c < ' ')
						{
							c = '.';
						}
						if (c == '\\' || c == '\"')
						{
							debug("\\");
						}
						debug("%c", c);
						offset++;
					}
					debug("\"\n");
				}

				debug("}\n");
			}
		}
		debug("]\n");
	}

	win32Api.RegCloseKey(rootKey);
	debug("}\n");
}
