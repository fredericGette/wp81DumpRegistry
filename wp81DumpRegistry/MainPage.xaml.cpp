//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "Dumper.h"

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

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter

	HKEY HKEY_CLASSES_ROOT = (HKEY)0x80000000;
	HKEY HKEY_CURRENT_USER = (HKEY)0x80000001;
	HKEY HKEY_LOCAL_MACHINE = (HKEY)0x80000002;
	HKEY HKEY_USERS = (HKEY)0x80000003;
	HKEY HKEY_PERFORMANCE_DATA = (HKEY)0x80000004;
	HKEY HKEY_CURRENT_CONFIG = (HKEY)0x80000005;
	HKEY HKEY_DYN_DATA = (HKEY)0x80000006;

	Dump(HKEY_DYN_DATA);
}
