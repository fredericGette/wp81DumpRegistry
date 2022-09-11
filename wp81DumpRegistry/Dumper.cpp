#include "pch.h"
#include "Dumper.h"
#include "Win32Api.h"

Win32Api win32Api;

void debug(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[100];
	vsnprintf_s(buffer, sizeof(buffer), format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

void write2File(HANDLE hFile, WCHAR* format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR buffer[10000];
	_vsnwprintf_s(buffer, sizeof(buffer), format, args);

	DWORD dwBytesToWrite = wcslen(buffer) * sizeof(WCHAR);
	DWORD dwBytesWritten = 0;
	win32Api.WriteFile(
		hFile,           // open file handle
		buffer,      // start of data to write
		dwBytesToWrite,  // number of bytes to write
		&dwBytesWritten, // number of bytes that were written
		NULL);            // no overlapped structure

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

void dump(HKEY rootKey, WCHAR *rootKeyName, boolean isFirst, HANDLE hFile)
{
	if (!isFirst)
	{
		debug(",");
		write2File(hFile, L",");
	}
	debug("{\n");
	write2File(hFile, L"{\n");
	debug("\t\"name\":\""); OutputDebugString(rootKeyName); debug("\"\n");
	write2File(hFile, L"\t\"name\":\"%ls\"\n", rootKeyName);

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
		write2File(hFile, L"\t,\"keys\":[\n");

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
					dump(subKey, SubKeyName, i==0, hFile);
				}
			}
		}
		debug("]\n");
		write2File(hFile, L"]\n");
	}


	if (NbValues)
	{
		debug("\t,\"values\":[\n");
		write2File(hFile, L"\t,\"values\":[\n");

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
					write2File(hFile, L",");
				}
				debug("{\n");
				write2File(hFile, L"{\n");
				WCHAR* escapedValueName = escape((WCHAR*)ValueName, ValueNameSize);
				debug("\t\"name\":\""); OutputDebugStringW(escapedValueName); debug("\"\n");
				write2File(hFile, L"\t\"name\":\"%ls\"\n", escapedValueName);
				debug("\t,\"value_type\":");
				write2File(hFile, L"\t,\"value_type\":");

				if (REG_SZ == ValueType)
				{
					debug("\"REG_SZ\"\n");
					write2File(hFile, L"\"REG_SZ\"\n");
					WCHAR* escapedValueData = escape((WCHAR*)ValueData, ValueDataSize/sizeof(WCHAR)); // ValueDataSize is a number of BYTE
					debug("\t,\"value\":\""); OutputDebugStringW(escapedValueData); debug("\"\n");
					write2File(hFile, L"\t,\"value\":\"%ls\"\n", escapedValueData);
				} 
				else if (REG_DWORD == ValueType)
				{
					debug("\"REG_DWORD\"\n");
					write2File(hFile, L"\"REG_DWORD\"\n");
					debug("\t,\"value\":\"0x%08X\"\n", *(PDWORD)ValueData);
					write2File(hFile, L"\t,\"value\":\"0x%08X\"\n", *(PDWORD)ValueData);
				}
				else if (REG_MULTI_SZ == ValueType)
				{
					debug("\"REG_MULTI_SZ\"\n");
					write2File(hFile, L"\"REG_MULTI_SZ\"\n");
					debug("\t,\"value\":[\n");
					write2File(hFile, L"\t,\"value\":[\n");
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
							write2File(hFile, L",");
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
						write2File(hFile, L"\t\"%ls\"\n", escapedValue);
					} while (*c != L'\0');
					debug("]\n");
					write2File(hFile, L"]\n");
				}
				else if (REG_EXPAND_SZ == ValueType)
				{
					debug("\"REG_EXPAND_SZ\"\n");
					write2File(hFile, L"\"REG_EXPAND_SZ\"\n");
					WCHAR* escapedValueData = escape((WCHAR*)ValueData, ValueDataSize/sizeof(WCHAR)); // ValueDataSize is a number of BYTE
					debug("\t,\"value\":\""); OutputDebugStringW(escapedValueData); debug("\"\n");
					write2File(hFile, L"\t,\"value\":\"%ls\"\n", escapedValueData);
				}
				else if (REG_QWORD == ValueType)
				{
					debug("\"REG_QWORD\"\n");
					write2File(hFile, L"\"REG_QWORD\"\n");
					unsigned long long value = *ValueData;
					debug("\t,\"value\":\"0x%016llx\"\n", value);
					write2File(hFile, L"\t,\"value\":\"0x%016llx\"\n", value);
				}
				else if (REG_NONE == ValueType)
				{
					debug("\"REG_NONE\"\n");
					write2File(hFile, L"\"REG_NONE\"\n");
				}
				else if (REG_BINARY == ValueType)
				{
					debug("\"REG_BINARY\"\n");
					write2File(hFile, L"\"REG_BINARY\"\n");
					debug("\t,\"value_size_in_bytes\":\"%dbytes\"\n", ValueDataSize);
					write2File(hFile, L"\t,\"value_size_in_bytes\":\"%dbytes\"\n", ValueDataSize);
					debug("\t,\"value\":\"");
					write2File(hFile, L"\t,\"value\":\"");
					DWORD offset = 0;
					while (offset < ValueDataSize && offset < MAX_BINARY_DISPLAY)
					{
						debug("%02X ", *(ValueData+offset));
						write2File(hFile, L"%02X ", *(ValueData + offset));
						offset++;
					}
					debug("\"\n");
					write2File(hFile, L"\"\n");
					debug("\t,\"value_char\":\"");
					write2File(hFile, L"\t,\"value_char\":\"");
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
							write2File(hFile, L"\\");
						}
						debug("%c", c);
						write2File(hFile, L"%c", c);
						offset++;
					}
					debug("\"\n");
					write2File(hFile, L"\"\n");
				}
				else 
				{
					debug("\"unknown type %d\"\n", ValueType);
					write2File(hFile, L"\"unknown type %d\"\n", ValueType);
					debug("\t,\"value_size_in_bytes\":\"%dbytes\"\n", ValueDataSize);
					write2File(hFile, L"\t,\"value_size_in_bytes\":\"%dbytes\"\n", ValueDataSize);
					debug("\t,\"value\":\"");
					write2File(hFile, L"\t,\"value\":\"");
					DWORD offset = 0;
					while (offset < ValueDataSize && offset < MAX_BINARY_DISPLAY)
					{
						debug("%02X ", *(ValueData + offset));
						write2File(hFile, L"%02X ", *(ValueData + offset));
						offset++;
					}
					debug("\"\n");
					write2File(hFile, L"\"\n");
					debug("\t,\"value_char\":\"");
					write2File(hFile, L"\t,\"value_char\":\"");
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
							write2File(hFile, L"\\");
						}
						debug("%c", c);
						write2File(hFile, L"%c", c);
						offset++;
					}
					debug("\"\n");
					write2File(hFile, L"\"\n");
				}

				debug("}\n");
				write2File(hFile, L"}\n");
			}
		}
		debug("]\n");
		write2File(hFile, L"]\n");
	}

	win32Api.RegCloseKey(rootKey);
	debug("}\n");
	write2File(hFile, L"}\n");
}

void Dump2File(HKEY rootKey, WCHAR *rootKeyName, WCHAR* folderPath)
{
	std::wstring filePath(folderPath);
	filePath += std::wstring(L"\\registryDump_");
	filePath += std::wstring(rootKeyName);
	filePath += std::wstring(L".json");

	HANDLE hFile;
	hFile = win32Api.CreateFileW(filePath.c_str(),                // name of the write
		GENERIC_WRITE,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		CREATE_ALWAYS,          // always create new file 
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template
	if (hFile == INVALID_HANDLE_VALUE)
	{
		debug("Error creating file [%s] : %d", filePath.c_str(), GetLastError());
		return;
	}

	dump(rootKey, rootKeyName, true, hFile);

	win32Api.CloseHandle(hFile);
}
