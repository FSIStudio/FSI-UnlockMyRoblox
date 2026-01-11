/*

FSI STUDIO PRODUCT - FSI UnlockMyRoblox V1 - Open-Source

EN:
!!! Please do not create your own program that exactly compiles 50% the functionality of the FSI UnlockMyRoblox !!!
Ignoring this message will have consequences!
OPEN-SOURCE designed for informational purposes only.

RU:
!!! Пожалуйста, не копируйте 50% функционала FSI UnlockMyRoblox в свой проект/приложение !!!
Игнорирование данного сообщения приведет к последствиям!
OPEN-SOURCE расчитан для ознакомления.

*/


//---------------------------------------------------------------------------
#include <vcl.h>
#include <ShellAPI.h>
#pragma hdrstop

#include "Unit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;

//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner) : TForm(Owner)
{
	//=== INITIALIZATION IN FORMCREATE ===
}
//---------------------------------------------------------------------------
__fastcall TMainForm::~TMainForm()
{
	if (processManager) {
		processManager->Stop();
	}
	delete methodManager;
	delete processManager;
	delete configManager;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCreate(TObject *Sender)
{
	//=== GET PATH ===
	appPath = ExtractFilePath(Application->ExeName);
	dataPath = appPath + "Data\\";

	//=== AUTO-CREATE ON EMPTY ===
	if (!DirectoryExists(dataPath)) {
		CreateDir(dataPath);
	}

	//=== MANAGERS INIT ===
	methodManager = new MethodManager(appPath);
	processManager = new ProcessManager(dataPath);
	configManager = new ConfigManager(dataPath);

	isMethodRunning = false;
	searchIndex = 0;

	//=== TIMER SETTINGS ===
	StatusCheckTimer->Interval = 60000; //--- 1 MINUTE
	StatusCheckTimer->Enabled = true;
	SearchTimer->Enabled = false;
	SearchTimer->Interval = 5000; //--- 5 SECONDS

	//=== LINK POPUPMENU WITH TRAYICON ===
	TrayIcon->PopupMenu = TrayMenu;
	TrayIcon->Visible = false;
	TrayIcon->Hint = "FSI UnlockMyRoblox";

	//=== FILLING LIST ===
	PopulateMethodsList();

	//=== CONFIG LOAD ===
	LoadConfiguration();

	//=== STATUSES INIT ===
	UpdateStatusLabel();
	UpdateDNSLabel();
	UpdateRobloxLabel();
	UpdateUI();

	if (ParamCount() > 0) {
		String param = ParamStr(1);
		if (param.LowerCase() == "/minimized") {
			Application->ShowMainForm = false;
			TrayIcon->Visible = true;
			if (config.Autostart) {
				Sleep(2000);
				StartMethodInternal();
			}
		}
	}
}
//=== TRAY PROC ===
void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action)
{
	Action = caNone;
	Hide();
	TrayIcon->Visible = true;
	ShowTrayNotification("FSI UnlockMyRoblox", "Программа свернута в трей");
}
//=== CONFIG LOAD ===
void TMainForm::LoadConfiguration()
{
	config = configManager->LoadConfig();

	ManualMethodSelection->Checked = config.ManualMethodSelection;
	HideCMD->Checked = config.HideCMD;

	bool autostartInRegistry = configManager->CheckAutostartInRegistry();
	bool trayAutostartInRegistry = configManager->CheckTrayAutostartInRegistry();

	Autostart->Checked = autostartInRegistry;
	TrayAutostart->Checked = trayAutostartInRegistry;

	config.Autostart = autostartInRegistry;
	config.TrayAutostart = trayAutostartInRegistry;

	methodManager->SetCurrentMethod(config.SelectedMethod);

	for (int i = 0; i < ManualMethods->Items->Count; i++) {
		if (ManualMethods->Items->Strings[i] == config.SelectedMethod) {
			ManualMethods->ItemIndex = i;
			break;
		}
	}

	UpdateMethodLabel();
	UpdateUI();
}
//=== CONFIG SAVE ===
void TMainForm::SaveConfiguration()
{
	config.SelectedMethod = methodManager->GetCurrentMethod();
	config.ManualMethodSelection = ManualMethodSelection->Checked;
	config.Autostart = Autostart->Checked;
	config.TrayAutostart = TrayAutostart->Checked;

	configManager->SaveConfig(config);

	configManager->iniFile->UpdateFile();
}
//=== LIST FILL ===
void TMainForm::PopulateMethodsList()
{
	ManualMethods->Clear();
	std::vector<String> methods = methodManager->GetAllMethods();

	for (const String& method : methods) {
		ManualMethods->Items->Add(method);
	}

	if (ManualMethods->Items->Count > 0) {
		ManualMethods->ItemIndex = 0;
	}
}
//=== UI UPD ===
void TMainForm::UpdateUI()
{
	bool isSearching = SearchTimer->Enabled;
	bool manualSelection = ManualMethodSelection->Checked;

	ManualMethodSelection->Enabled = !isSearching && !isMethodRunning;
	FindOptimalMethod->Enabled = !isSearching && !manualSelection && !isMethodRunning;
	StartMethod->Enabled = !isSearching;
	ManualMethods->Enabled = manualSelection && !isSearching && !isMethodRunning;
	Autostart->Enabled = !isSearching && !isMethodRunning;
	TrayAutostart->Enabled = !isSearching && !isMethodRunning && Autostart->Checked;
	HideCMD->Enabled = !isSearching && !isMethodRunning;

	if (isMethodRunning) {
		StartMethod->Caption = "Остановить";
	} else {
		StartMethod->Caption = "Запустить";
	}

	TrayStartMethod->Enabled = !isMethodRunning && !isSearching;
	TrayStopMethod->Enabled = isMethodRunning;
}
//=== METHOD SELECTION ===
void TMainForm::UpdateMethodLabel()
{
	SelectedMethod->Caption = "Выбран метод: " + methodManager->GetCurrentMethod();
}
//=== STATUS PROC ===
void TMainForm::UpdateStatusLabel()
{
	if (isMethodRunning) {
		MethodStatus->Caption = "Статус: Запущен";
		MethodStatus->Font->Color = clGreen;
	} else {
		MethodStatus->Caption = "Статус: Не запущен";
		MethodStatus->Font->Color = clRed;
	}
}
//=== DNS CHECK ===
void TMainForm::UpdateDNSLabel()
{
	bool isDNSSet = DNSManager::IsGoogleDNSSet();

	if (isDNSSet) {
		GoogleDNSCheck->Caption = "Выставлен ли DNS (Google): Да";
		GoogleDNSCheck->Font->Color = clGreen;
	} else {
		GoogleDNSCheck->Caption = "Выставлен ли DNS (Google): Нет";
		GoogleDNSCheck->Font->Color = clRed;
	}
}
//=== RBLX CHECK ===
void TMainForm::UpdateRobloxLabel()
{
	bool isWorking = methodManager->CheckRobloxConnection();

	if (isWorking) {
		RobloxWorkCheck->Caption = "Работает ли Roblox: Да";
		RobloxWorkCheck->Font->Color = clGreen;
	} else {
		RobloxWorkCheck->Caption = "Работает ли Roblox: Нет";
		RobloxWorkCheck->Font->Color = clRed;
	}
}
//=== METHOS STARTUP ===
void TMainForm::StartMethodInternal()
{
	DNSManager::SetGoogleDNS();
	Sleep(1000);

	String cmd = methodManager->GetCommandLine(methodManager->GetCurrentMethod());

	if (cmd.IsEmpty()) {
		ShowMessage("Ошибка: метод не найден!");
		return;
	}

	String fullCmd = "cmd.exe /c \"" + cmd + "\"";

	if (processManager->Start(fullCmd, HideCMD->Checked)) {
		Sleep(2000);
		isMethodRunning = true;
		UpdateStatusLabel();
		UpdateDNSLabel();
		UpdateUI();

		String methodName = methodManager->GetCurrentMethod();
		ShowTrayNotification("Метод запущен", "Метод \"" + methodName + "\" успешно запущен!");

		if (Visible) {
			ShowMessage("Метод \"" + methodName + "\" успешно запущен!");
		}
	} else {
		ShowMessage("Ошибка запуска метода! Проверьте:\n1. Запущена ли программа от администратора\n2. Существуют ли файлы в папке Data\\bin\n3. Правильно ли указаны пути");
	}
}
//---------------------------------------------------------------------------
void TMainForm::StopMethodInternal()
{
	if (processManager->Stop()) {
		isMethodRunning = false;
		UpdateStatusLabel();
		UpdateUI();

		ShowTrayNotification("Метод остановлен", "Метод был успешно остановлен");

		if (Visible) {
			ShowMessage("Метод остановлен");
		}
	} else {
		ShowMessage("Ошибка остановки метода");
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FindOptimalMethodClick(TObject *Sender)
{
	searchMethods = methodManager->GetAllMethods();
	searchIndex = 0;

	FindMethodProgress->Position = 0;
	FindMethodProgress->Max = 100;

	SearchTimer->Enabled = true;
	UpdateUI();

	ShowMessage("Начат поиск оптимального метода. Это может занять некоторое время...");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ManualMethodSelectionClick(TObject *Sender)
{
	if (ManualMethodSelection->Checked) {
		if (ManualMethods->ItemIndex < 0 && ManualMethods->Items->Count > 0) {
			ManualMethods->ItemIndex = 0;
			String selectedMethod = ManualMethods->Items->Strings[0];
			methodManager->SetCurrentMethod(selectedMethod);
		}
	}

	UpdateUI();
	UpdateMethodLabel();
	SaveConfiguration();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ManualMethodsClick(TObject *Sender)
{
	if (ManualMethods->ItemIndex >= 0) {
		String selectedMethod = ManualMethods->Items->Strings[ManualMethods->ItemIndex];
		methodManager->SetCurrentMethod(selectedMethod);
		UpdateMethodLabel();

		config.SelectedMethod = selectedMethod;
		configManager->SaveConfig(config);
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::StartMethodClick(TObject *Sender)
{
	if (isMethodRunning) {
		StopMethodInternal();
	} else {
		StartMethodInternal();
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::AutostartClick(TObject *Sender)
{
	configManager->SetAutostart(Autostart->Checked, TrayAutostart->Checked);
	UpdateUI();
	SaveConfiguration();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TrayAutostartClick(TObject *Sender)
{
	if (Autostart->Checked) {
		configManager->SetAutostart(true, TrayAutostart->Checked);
	}
	SaveConfiguration();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::StatusCheckTimerTimer(TObject *Sender)
{
	UpdateDNSLabel();
	UpdateRobloxLabel();

	if (isMethodRunning && !processManager->IsRunning()) {
		isMethodRunning = false;
		UpdateStatusLabel();
		UpdateUI();
	}
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::SearchTimerTimer(TObject *Sender)
{
	if (searchIndex >= searchMethods.size()) {
		SearchTimer->Enabled = false;
		FindMethodProgress->Position = 100;
		UpdateUI();

		if (methodManager->CheckRobloxConnection()) {
			ShowMessage("Поиск завершен! Найден рабочий метод: " +
				methodManager->GetCurrentMethod());
		} else {
			ShowMessage("Поиск завершен. К сожалению, ни один метод не сработал. "
				"Попробуйте запустить программу с правами администратора или проверьте подключение к интернету.");
		}

        //FSI STUDIO PRODUCT - FSI UnlockMyRoblox V1 - Open-Source

		UpdateMethodLabel();
		SaveConfiguration();
		return;
	}

	String currentMethod = searchMethods[searchIndex];
	methodManager->SetCurrentMethod(currentMethod);

	if (processManager->IsRunning()) {
		processManager->Stop();
		Sleep(1000);
	}

	String cmd = methodManager->GetCommandLine(currentMethod);
	if (!cmd.IsEmpty()) {
		String fullCmd = "cmd.exe /c \"" + cmd + "\"";
		processManager->Start(fullCmd);
		Sleep(3000);

		if (methodManager->CheckRobloxConnection()) {
			processManager->Stop();
			SearchTimer->Enabled = false;
			FindMethodProgress->Position = 100;
			UpdateUI();

			ShowMessage("Найден рабочий метод: " + currentMethod);
			UpdateMethodLabel();
			SaveConfiguration();
			return;
		}
	}

	searchIndex++;
	FindMethodProgress->Position = (searchIndex * 100) / searchMethods.size();
}
//---------------------------------------------------------------------------
//=== TRAY HANDLERS ===
//---------------------------------------------------------------------------
void __fastcall TMainForm::TrayIconDblClick(TObject *Sender)
{
	TrayIcon->Visible = false;
	Show();
	Application->BringToFront();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TrayOpenClick(TObject *Sender)
{
	TrayIconDblClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TrayStartMethodClick(TObject *Sender)
{
	StartMethodInternal();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TrayStopMethodClick(TObject *Sender)
{
	StopMethodInternal();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TrayExitClick(TObject *Sender)
{
	StopMethodInternal();
	SaveConfiguration();
	TrayIcon->Visible = false;
	Application->Terminate();
}
//---------------------------------------------------------------------------
void TMainForm::ShowTrayNotification(const String& title, const String& msg)
{
	TrayIcon->BalloonTitle = title;
	TrayIcon->BalloonHint = msg;
	TrayIcon->ShowBalloonHint();
}

void TMainForm::HideCMDClick(TObject *Sender)
{
}
void __fastcall TMainForm::fsistudioru1Click(TObject *Sender)
{
	ShellExecute(0, L"open", L"https://fsistudio.ru", NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::Image1Click(TObject *Sender)
{
    ShellExecute(0, L"open", L"https://www.donationalerts.com/r/fsi_studio", NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FSILogoClick(TObject *Sender)
{
    ShellExecute(0, L"open", L"https://fsistudio.ru", NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------

