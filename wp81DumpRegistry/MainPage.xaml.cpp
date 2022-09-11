//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "Dumper.h"
#include "Win32Api.h"

using namespace wp81DumpRegistry;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();
}

HANDLE hThread;
Windows::UI::Xaml::DispatcherTimer ^timer;

typedef struct MyData {
	HKEY rootKey;
	WCHAR *rootKeyName;
	WCHAR *folderPath;
} MYDATA, *PMYDATA;

DWORD WINAPI MyThreadFunction(LPVOID lpParam)
{
	PMYDATA data = (PMYDATA)lpParam;
	HKEY rootKey = data->rootKey;
	Dump2File(rootKey, data->rootKeyName, data->folderPath);
	return 0;
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	HKEY HKEY_CLASSES_ROOT = (HKEY)0x80000000;
	HKEY HKEY_CURRENT_USER = (HKEY)0x80000001;
	HKEY HKEY_LOCAL_MACHINE = (HKEY)0x80000002;
	HKEY HKEY_USERS = (HKEY)0x80000003;
	HKEY HKEY_PERFORMANCE_DATA = (HKEY)0x80000004;
	HKEY HKEY_CURRENT_CONFIG = (HKEY)0x80000005;
	HKEY HKEY_DYN_DATA = (HKEY)0x80000006;

	TextTest->Text = L"Dumping "; 
	TextTest->Text += L"HKEY_LOCAL_MACHINE";
	TextTest->Text += L" into folder ";
	TextTest->Text += L"C:\\Data\\USERS\\Public\\Documents";

	PMYDATA pData = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(MYDATA));
	pData->rootKey = HKEY_LOCAL_MACHINE;
	pData->rootKeyName = L"HKEY_LOCAL_MACHINE";
	pData->folderPath = L"C:\\Data\\USERS\\Public\\Documents";

	DWORD  dwThreadId;
	hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		MyThreadFunction,       // thread function name
		pData,          // argument to thread function 
		0,                      // use default creation flags 
		&dwThreadId);   // returns the thread identifier

	timer = ref new Windows::UI::Xaml::DispatcherTimer();
	TimeSpan ts;
	ts.Duration = 2000000; // A tick is equal to 100 nanoseconds, or one ten-millionth of a second.
	timer->Interval = ts;
	timer->Start();
	auto registrationtoken = timer->Tick += ref new EventHandler<Object^>(this, &MainPage::OnTick);
}

void MainPage::OnTick(Object^ sender, Object^ e) {
	TextTest->Text += L".";
	DWORD exitCode;
	if (hThread != NULL)
	{
		GetExitCodeThread(hThread, &exitCode);
		if (exitCode != STILL_ACTIVE)
		{
			timer->Stop();
			TextTest->Text += L"Done";
			CloseHandle(hThread);
			hThread = NULL;
		}	
	}
}