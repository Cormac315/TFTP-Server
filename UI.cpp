#include <wx/wxprec.h>
#include <wx/textfile.h>
#include <cstdlib>
#include <string>
#include <thread>
#include <fstream>
#include <unordered_map>
#include <iostream>
#include <filesystem>
#include "UI.h"
#include "Server.h"
#include "shared.h"
#include "api.h"
#include <wx/msgdlg.h>
#include <wx/taskbar.h>
#include <wx/menu.h>
///////////////////////////////////////////////////////////////////////////
// ȫ�ִ���ָ��
main* g_mainFrame = nullptr;
main* mainFrame = nullptr;
settings* settingsWindow = nullptr;
setting_saved_information* settingsSavedWindow = nullptr;
MyTaskBarIcon* taskBarIcon = nullptr;
///////////////////////////////////////////////////////////////////////////
// ������ȫ�ֱ���
wxString s_MaxThreadCount = wxString::Format(wxT("%d"), MaxThreadCount);
wxString s_MAX_RETRIES = wxString::Format(wxT("%d"), MAX_RETRIES);
bool b_console_log_switch = console_log_switch;
wxString s_console_log_dir = wxString::FromUTF8(console_log_dir.c_str());
bool b_error_log_switch = error_log_switch;
wxString s_error_log_dir = wxString::FromUTF8(error_log_dir.c_str());
bool is_console_mode = false;
wxString s_PORT = wxString::Format(wxT("%d"), PORT);
//wxString s_TIMEOUT = wxString::Format(wxT("%d"), TIMEOUT); // �ݲ�����
int selectedInterfaceIdx = -1;
int TableRows = 16;
bool Remember_Minimize = false;
std::vector<std::pair<std::string, std::string>> interfaces = GetNetworkInterfaces();
///////////////////////////////////////////////////////////////////////////
enum
{
	PU_RESTORE = 10001, // �ָ�
	PU_EXIT // �˳�
};
///////////////////////////////////////////////////////////////////////////
main::main(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	g_mainFrame = this; // ��ʼ��ȫ��ָ��
	this->SetSizeHints(wxSize(1250, 600), wxDefaultSize);
	this->SetBackgroundColour(wxColour(239, 235, 235));

	// ���ô���ͼ��
	wxIcon icon;
	icon.LoadFile("./assets/cormac.ico", wxBITMAP_TYPE_ICO);
	this->SetIcon(icon);

	wxFlexGridSizer* main_grid;
	main_grid = new wxFlexGridSizer(4, 2, 0, 0);
	main_grid->AddGrowableCol(1);
	main_grid->AddGrowableRow(1);
	main_grid->SetFlexibleDirection(wxBOTH);
	main_grid->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	main_grid->SetMinSize(wxSize(1250, 600));
	wxGridSizer* Switches;
	Switches = new wxGridSizer(1, 2, 0, 0);

	wxString Server_SwitchChoices[] = { _("����(��������˿ڿ�����)"), _("�ر�") };
	int Server_SwitchNChoices = sizeof(Server_SwitchChoices) / sizeof(wxString);
	Server_Switch = new wxRadioBox(this, wxID_ANY, _("������״̬"), wxDefaultPosition, wxDefaultSize, Server_SwitchNChoices, Server_SwitchChoices, 2, wxRA_SPECIFY_ROWS);
	Server_Switch->SetSelection(1);
	Switches->Add(Server_Switch, 0, wxALL | wxEXPAND, 5);

	wxBoxSizer* buttons;
	buttons = new wxBoxSizer(wxVERTICAL);

	settingsButton = new wxButton(this, wxID_ANY, _("����"), wxDefaultPosition, wxDefaultSize, 0);
	buttons->Add(settingsButton, 0, wxALL | wxEXPAND, 5);

	console_mode = new wxButton(this, wxID_ANY, _("������ģʽ"), wxDefaultPosition, wxDefaultSize, 0);
	buttons->Add(console_mode, 0, wxALL | wxEXPAND, 5);

	check_update = new wxButton(this, wxID_ANY, _("������"), wxDefaultPosition, wxDefaultSize, 0);
	buttons->Add(check_update, 0, wxALL | wxEXPAND, 5);

	version = new wxStaticText(this, wxID_ANY, _("��ǰ�汾:1.2"), wxDefaultPosition, wxDefaultSize, 0);
	version->Wrap(-1);
	version->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

	buttons->Add(version, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);


	Switches->Add(buttons, 1, wxEXPAND, 5);


	main_grid->Add(Switches, 1, wxEXPAND, 5);

	wxGridSizer* grid_server_dir;
	grid_server_dir = new wxGridSizer(2, 1, 0, 0);

	wxGridSizer* gSizer7;
	gSizer7 = new wxGridSizer(2, 1, 0, 0);

	text_server_dir = new wxStaticText(this, wxID_ANY, _("�������ļ�Ŀ¼(֧�����·��)"), wxDefaultPosition, wxDefaultSize, 0);
	text_server_dir->Wrap(-1);
	gSizer7->Add(text_server_dir, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	server_dir = new wxDirPickerCtrl(this, wxID_ANY, wxT("./File"), _("������Ŀ¼"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE);
	gSizer7->Add(server_dir, 0, wxALL | wxEXPAND, 5);

	grid_server_dir->Add(gSizer7, 1, wxEXPAND, 5);

	wxGridSizer* grid_sever_ip;
	grid_sever_ip = new wxGridSizer(2, 1, 0, 0);

	text_server_ip = new wxStaticText(this, wxID_ANY, _("������ͨ�Žӿ�"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
	text_server_ip->Wrap(-1);
	grid_sever_ip->Add(text_server_ip, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	selectedInterfaceIdx = 0;
	if (interfaces.empty()) {
		console_log->AppendText("��ȡ����ʧ�ܣ�ʹ��Ĭ�Ͻӿ�");
	}
	interfaces.insert(interfaces.begin(),std::make_pair(std::string("Default"), std::string("All")));

	wxArrayString choices;
	for (const auto& iface : interfaces) {
		choices.Add(wxString::FromUTF8(iface.first + " - " + iface.second));
	}
	choice_server_ip = new wxChoice(this, wxID_ANY, wxPoint(-1, -1), wxDefaultSize, choices, 0);
	choice_server_ip->SetSelection(0);
	grid_sever_ip->Add(choice_server_ip, 0, wxALL | wxEXPAND, 5);

	grid_server_dir->Add(grid_sever_ip, 1, wxEXPAND, 5);

	main_grid->Add(grid_server_dir, 1, wxEXPAND, 5);

	wxGridSizer* console_log_grid;
	console_log_grid = new wxGridSizer(1, 1, 0, 0);

	console_log = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	console_log_grid->Add(console_log, 0, wxALL | wxEXPAND, 5);

	main_grid->Add(console_log_grid, 1, wxEXPAND, 5);

	wxGridSizer* information_grid;
	information_grid = new wxGridSizer(1, 1, 0, 0);

	information_table = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);


	// Grid
	information_table->CreateGrid(TableRows, 6);
	information_table->EnableEditing(false);
	information_table->EnableGridLines(true);
	information_table->SetGridLineColour(wxColour(0, 0, 0));
	information_table->EnableDragGridSize(false);
	information_table->SetMargins(0, 0);

	// Columns
	information_table->SetColSize(0, 100);
	information_table->SetColSize(1, 150);
	information_table->SetColSize(2, 155);
	information_table->SetColSize(3, 185);
	information_table->SetColSize(4, 110);
	information_table->SetColSize(5, 70);
	information_table->EnableDragColMove(false);
	information_table->EnableDragColSize(true);
	information_table->SetColLabelValue(0, _("�ͻ���ip"));
	information_table->SetColLabelValue(1, _("�ļ�����"));
	information_table->SetColLabelValue(2, _("��ʼʱ��"));
	information_table->SetColLabelValue(3, _("״̬"));
	information_table->SetColLabelValue(4, _("������(KB/S)"));
	information_table->SetColLabelValue(5, _("���Դ���"));
	information_table->SetColLabelValue(6, wxEmptyString);
	information_table->SetColLabelValue(7, _("��ʼʱ�䡮"));
	information_table->SetColLabelSize(30);
	information_table->SetColLabelAlignment(wxALIGN_CENTER, wxALIGN_CENTER);

	// Rows
	information_table->AutoSizeRows();
	information_table->EnableDragRowSize(true);
	information_table->SetRowLabelSize(60);
	information_table->SetRowLabelAlignment(wxALIGN_CENTER, wxALIGN_CENTER);

	// Label Appearance

	// Cell Defaults
	information_table->SetDefaultCellAlignment(wxALIGN_CENTER, wxALIGN_TOP);
	information_grid->Add(information_table, 0, wxALL |wxEXPAND, 5);


	main_grid->Add(information_grid, 0 , wxALL | wxEXPAND, 5);


	this->SetSizer(main_grid);
	this->Layout();

	this->Centre(wxBOTH);

	// ���¼�
	settingsButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(main::OnSettingsButtonClick), NULL, this);
	check_update->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(main::OnCheckUpdateButtonClick), NULL, this);
	Server_Switch->Connect(wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler(main::OnServerSwitch), NULL, this); 
	console_mode->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(main::OnConsoleModeButtonClick), NULL, this);
	server_dir->Connect(wxEVT_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(main::OnServerDirChanged), NULL, this);
	version->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(main::OnVersionClick), NULL, this);
	version->Bind(wxEVT_ENTER_WINDOW, &main::OnMouseEnter, this);
	version->Bind(wxEVT_LEAVE_WINDOW, &main::OnMouseLeave, this);
	choice_server_ip->Connect(wxEVT_CHOICE, wxCommandEventHandler(main::OnServerIPChanged), NULL, this);
	this->Bind(wxEVT_CLOSE_WINDOW, &main::OnClose, this); // �رմ����¼�
	loadSettings("settings.conf"); // ��ȡ�����ļ�
}


// ��ȡ�����ļ�������ȫ�ֱ���
void main::loadSettings(const std::string& configFilePath) {
	if (configFilePath.empty()) return;

	std::ifstream configFile(configFilePath);
	if (!configFile.is_open()) {
		std::cerr << "�޷��������ļ�: " << configFilePath << std::endl;
		return;
	}

	std::unordered_map<std::string, std::string> settings;
	std::string line;
	while (std::getline(configFile, line)) {
		size_t delimiterPos = line.find('=');
		if (delimiterPos != std::string::npos) {
			std::string key = line.substr(0, delimiterPos);
			std::string value = line.substr(delimiterPos + 1);
			settings[key] = value;
		}
	}

	configFile.close();

	// ����ȫ�ֱ���
	if (settings.find("max_thread") != settings.end()) {
		MaxThreadCount = std::stoi(settings["max_thread"]);
		s_MaxThreadCount = wxString::Format(wxT("%d"), MaxThreadCount);
	}
	if (settings.find("max_retransfer") != settings.end()) {
		MAX_RETRIES = std::stoi(settings["max_retransfer"]);
		s_MAX_RETRIES = wxString::Format(wxT("%d"), MAX_RETRIES);
	}
	//if (settings.find("timeout") != settings.end()) {
	//	TIMEOUT = std::stoi(settings["timeout"]);
	//}
	if (settings.find("server_listen_port") != settings.end()) {
		PORT = std::stoi(settings["server_listen_port"]);
		s_PORT = wxString::Format(wxT("%d"), PORT);
	}
	if (settings.find("console_log_switch") != settings.end()) {
		console_log_switch = settings["console_log_switch"] == "1";
		b_console_log_switch = console_log_switch;
	}
	if (settings.find("console_log_dir") != settings.end()) {
		console_log_dir = settings["console_log_dir"];
		s_console_log_dir = wxString::FromUTF8(console_log_dir.c_str());
	}
	if (settings.find("error_log_switch") != settings.end()) {
		error_log_switch = settings["error_log_switch"] == "1";
		b_error_log_switch = error_log_switch;
	}
	if (settings.find("error_log_dir") != settings.end()) {
		error_log_dir = settings["error_log_dir"];
		s_error_log_dir = wxString::FromUTF8(error_log_dir.c_str());
	}
}

// ���ð�ť����¼�
void main::OnSettingsButtonClick(wxCommandEvent& event)
{
	loadSettings("settings.conf"); // ��ȡ�����ļ�
    settings* settingsWindow = new settings(nullptr, wxID_ANY, _("����"));
    settingsWindow->Show(true);

}

// �����°�ť����¼�
void main::OnCheckUpdateButtonClick(wxCommandEvent& event)
{
	std::string filePath = "Version.exe";
	if (!std::filesystem::exists(filePath)) {
		wxMessageBox(_("�汾������Version.exe������"), _("����"), wxOK | wxICON_ERROR);
		return;
	}
    std::string version = "1.2"; // ��ǰ�汾��
    std::string command = "Version.exe " + version;
    system(command.c_str());
    
}

// ������״̬��ť
void main::OnServerSwitch(wxCommandEvent& event)
{
	if (Server_Switch->GetSelection() == 0) 
	{	
		running = true;
		std::thread serverThread(StartServer); // �����������߳�
		serverThread.detach(); 
	}
	if (Server_Switch->GetSelection() == 1)
	{
		running = false;
		is_console_mode = false;
	}
}

// ����̨�������
BOOL WINAPI ConsoleHandler(DWORD signal) {
	if (signal == CTRL_CLOSE_EVENT) {
		wxMessageBox("�������쳣��ֹ����ʹ��/stop����رշ�����", "�Ƿ�����", wxICON_ERROR);
		return TRUE; // ��ֹ�ر��¼�
	}
	return FALSE;
}

// ������ģʽ��ť����¼�
void main::OnConsoleModeButtonClick(wxCommandEvent& event)
{
	if (Server_Switch->GetSelection() == 1) {
		OpenConsoleWindow();
		running = true;
		is_console_mode = true;

		if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
			std::cerr << "�޷����ÿ���̨���ƴ������" << std::endl;
			exit(1);
		}

		std::thread serverThread(StartServer); // �����������߳�
		serverThread.detach();
		Server_Switch->SetSelection(0);
	}
	else {
		wxMessageBox("��⵽�������������У����ȹرշ�����", "����", wxICON_ERROR);
	}	
}

// ������Ŀ¼ѡ���¼�
void main::OnServerDirChanged(wxFileDirPickerEvent& event)
{
	if (!running)
	{
		ServerRootDirectory = server_dir->GetPath().ToStdString();
		console_log->AppendText("������Ŀ¼�Ѹ���Ϊ��" + ServerRootDirectory + "\n");
	}
	else {
		wxMessageBox("�������ڷ���������ʱ�޸�Ŀ¼�����ȹرշ�����", "�Ƿ�����", wxICON_ERROR);
		server_dir->SetPath(ServerRootDirectory);
	}
}

// ά�������Ϣ
void main::AddClientInfo(const std::string& ip, const std::string& file, const std::string& startTime) {
	if (currentRow >= TableRows) {
		information_table->AppendRows(1); // ��̬���һ��
		information_table->ScrollLines(currentRow); // ���������һ��
	}

	information_table->SetCellValue(currentRow, 0, ip);
	information_table->SetCellValue(currentRow, 1, file);
	information_table->SetCellValue(currentRow, 2, startTime);
	information_table->SetCellValue(currentRow, 3, "������");
	information_table->SetCellValue(currentRow, 4, "���ڼ���");
	information_table->SetCellValue(currentRow, 5, "0");

	currentRow++;
}

void main::UpdateClientStatus(int row, const std::string& status, const std::string& throughput, int timeoutCount) {
	information_table->SetCellValue(row, 3, status);
	if (!throughput.empty()) {
		information_table->SetCellValue(row, 4, throughput);
	}
	information_table->SetCellValue(row, 5, std::to_string(timeoutCount));
}

void main::TableStatus(int row, bool success)
{
	// ���õ�Ԫ����ɫ
	if (success)
	{
		for (size_t i = 0; i <= 5; i++)information_table->SetCellBackgroundColour(row, i, wxColour(144, 238, 144)); // ǳ��ɫ
	}
	else
	{
		for (size_t i = 0; i <= 5; i++)information_table->SetCellBackgroundColour(row, i , wxColour(255, 182, 193)); // ��ɫ
	}

	// ˢ�±��
	information_table->ForceRefresh();
}

void main::OnVersionClick(wxMouseEvent& event)
{
	wxMessageBox(_("1.2���£�\n- ���ѡ������ӿڹ���\n- �Ż��׽��ְ�ʧ�ܴ�����Ϣ\n���¼ƻ���\n- ��̬������ʱʱ�䣬���ҿ����ֶ�����\n- ��������ACK�ķ���"), _("������Ϣ"), wxOK | wxICON_INFORMATION, this);
}

void main::OnMouseEnter(wxMouseEvent& event)
{
    wxFont font = version->GetFont();
    font.SetUnderlined(true);
    version->SetFont(font);
    event.Skip();
}

void main::OnMouseLeave(wxMouseEvent& event)
{
	wxFont font = version->GetFont();
	font.SetUnderlined(false);
	version->SetFont(font);
	event.Skip();
}

void main::OnClose(wxCloseEvent& event)
{
	int response;
	// ѯ���û��Ƿ���С��������
	if (Remember_Minimize) response = wxID_YES;
	else {
		wxMessageDialog dialog(this, _("����������ѿ������⽫ʹ���ں�̨���С�\nѡ��\"��\"֮�󣬱������ж�����ȡ��С�������̡�"), _("�Ƿ���С�������̣�"), wxYES_NO | wxCANCEL | wxICON_QUESTION);
		response = dialog.ShowModal();
	}

	if (response == wxID_YES)
	{
		Remember_Minimize = true;
		if (!taskBarIcon)
		{
			taskBarIcon = new MyTaskBarIcon();
			wxIcon icon;
			icon.LoadFile("./assets/ShortCut.ico", wxBITMAP_TYPE_ICO);
			taskBarIcon->SetIcon(icon, _("TFTP������"));

			// ������ͼ����¼�
			taskBarIcon->Bind(wxEVT_TASKBAR_LEFT_DCLICK, &MyTaskBarIcon::OnLeftButtonDClick, taskBarIcon);
			taskBarIcon->Bind(wxEVT_MENU, &MyTaskBarIcon::OnMenuRestore, taskBarIcon, PU_RESTORE);
			taskBarIcon->Bind(wxEVT_MENU, &MyTaskBarIcon::OnMenuExit, taskBarIcon, PU_EXIT);
		}
		this->Hide();
	}
	else if (response == wxID_NO)
	{
		if (running) wxMessageBox("���ȹرշ�����", "����", wxICON_ERROR);
		else {
			taskBarIcon->Destroy();
			Destroy();
			exit(0);
		}
	}
	// ����û�ѡ��ȡ������ִ���κβ���
}

void main::OnServerIPChanged(wxCommandEvent& event)
{
	if (running) {
		wxMessageBox("����������ʱ��ֹ���Ĵ�ѡ����ȹرշ�������", "�Ƿ�����", wxICON_ERROR);
		choice_server_ip->SetSelection(selectedInterfaceIdx);
	}
	else {
		selectedInterfaceIdx = choice_server_ip->GetSelection(); // ��ȡѡ�������
	}
}


main::~main()
{
	// Disconnect Events
	settingsButton->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(main::OnSettingsButtonClick), NULL, this);

}

settings::settings(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxSize(390, 410), wxDefaultSize);
	this->SetBackgroundColour(wxColour(239, 235, 235));

	// ���ô���ͼ��
	wxIcon icon;
	icon.LoadFile("./assets/settings.ico", wxBITMAP_TYPE_ICO);
	this->SetIcon(icon);

	wxBoxSizer* setting_box;
	setting_box = new wxBoxSizer(wxVERTICAL);

	setting_box->SetMinSize(wxSize(390, 410));
	global_var = new wxStaticText(this, wxID_ANY, _("ȫ�ֱ���"), wxDefaultPosition, wxDefaultSize, 0);
	global_var->Wrap(-1);
	setting_box->Add(global_var, 0, wxALL, 5);

	wxGridSizer* max_thread_grid;
	max_thread_grid = new wxGridSizer(0, 2, 0, 0);

	max_thread_text = new wxStaticText(this, wxID_ANY, _("֧�ֵ�����߳���"), wxDefaultPosition, wxDefaultSize, 0);
	max_thread_text->Wrap(-1);
	max_thread_grid->Add(max_thread_text, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

	max_thread = new wxTextCtrl(this, wxID_ANY, s_MaxThreadCount, wxDefaultPosition, wxDefaultSize, 0);
	max_thread_grid->Add(max_thread, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);


	setting_box->Add(max_thread_grid, 0, wxEXPAND, 5);

	wxGridSizer* max_retransfer_grid;
	max_retransfer_grid = new wxGridSizer(0, 2, 0, 0);

	max_retransfer_text = new wxStaticText(this, wxID_ANY, _("����ش�����"), wxDefaultPosition, wxDefaultSize, 0);
	max_retransfer_text->Wrap(-1);
	max_retransfer_grid->Add(max_retransfer_text, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

	max_retransfer = new wxTextCtrl(this, wxID_ANY, s_MAX_RETRIES, wxDefaultPosition, wxDefaultSize, 0);
	max_retransfer_grid->Add(max_retransfer, 0, wxALL, 5);


	setting_box->Add(max_retransfer_grid, 0, wxEXPAND, 5);

	wxGridSizer* timeout_grid;
	timeout_grid = new wxGridSizer(0, 2, 0, 0);

	timeout_text = new wxStaticText(this, wxID_ANY, _("��ʱʱ��(ms) - �ݲ�֧�ָ���"), wxDefaultPosition, wxDefaultSize, 0);
	timeout_text->Wrap(-1);
	timeout_grid->Add(timeout_text, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

	timeout = new wxTextCtrl(this, wxID_ANY, _("200"), wxDefaultPosition, wxDefaultSize, 0);
	timeout->SetEditable(false);
	timeout_grid->Add(timeout, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);


	setting_box->Add(timeout_grid, 0, wxEXPAND, 5);

	wxGridSizer* listen_port_grid;
	listen_port_grid = new wxGridSizer(0, 2, 0, 0);

	server_listen_port_text = new wxStaticText(this, wxID_ANY, _("TFTP�����������˿�"), wxDefaultPosition, wxDefaultSize, 0);
	server_listen_port_text->Wrap(-1);
	listen_port_grid->Add(server_listen_port_text, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

	server_listen_port = new wxTextCtrl(this, wxID_ANY, s_PORT, wxDefaultPosition, wxDefaultSize, 0);
	listen_port_grid->Add(server_listen_port, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);


	setting_box->Add(listen_port_grid, 0, wxEXPAND, 5);

	listen_port = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
	setting_box->Add(listen_port, 0, wxEXPAND | wxALL, 5);

	log_record = new wxStaticText(this, wxID_ANY, _("��־��¼"), wxDefaultPosition, wxDefaultSize, 0);
	log_record->Wrap(-1);
	setting_box->Add(log_record, 0, wxALL, 5);

	wxGridSizer* console_log_switch_grid;
	console_log_switch_grid = new wxGridSizer(0, 2, 0, 0);

	console_log_switch_text = new wxStaticText(this, wxID_ANY, _("�������������¼����"), wxDefaultPosition, wxDefaultSize, 0);
	console_log_switch_text->Wrap(-1);
	console_log_switch_grid->Add(console_log_switch_text, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

	console_log_switch = new wxCheckBox(this, wxID_ANY, _("���·�ѡ�񱣴�·��"), wxDefaultPosition, wxDefaultSize, 0);
	console_log_switch->SetValue(b_console_log_switch);
	console_log_switch_grid->Add(console_log_switch, 0, wxALL, 5);


	setting_box->Add(console_log_switch_grid, 0, wxEXPAND, 5);

	wxGridSizer* console_log_dir_grid;
	console_log_dir_grid = new wxGridSizer(1, 1, 0, 0);

	console_log_dir = new wxDirPickerCtrl(this, wxID_ANY, s_console_log_dir, _("ѡ���ļ���"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE);
	console_log_dir_grid->Add(console_log_dir, 1, wxALL | wxEXPAND, 5);


	setting_box->Add(console_log_dir_grid, 0, wxEXPAND, 5);

	wxGridSizer* error_log_swtich_grid;
	error_log_swtich_grid = new wxGridSizer(0, 2, 0, 0);

	error_log_swtich_text = new wxStaticText(this, wxID_ANY, _("�������쳣�����¼����"), wxDefaultPosition, wxDefaultSize, 0);
	error_log_swtich_text->Wrap(-1);
	error_log_swtich_grid->Add(error_log_swtich_text, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

	error_log_swtich = new wxCheckBox(this, wxID_ANY, _("���·�ѡ�񱣴�·��"), wxDefaultPosition, wxDefaultSize, 0);
	error_log_swtich->SetValue(b_error_log_switch);
	error_log_swtich_grid->Add(error_log_swtich, 0, wxALL, 5);


	setting_box->Add(error_log_swtich_grid, 0, wxEXPAND, 5);

	wxGridSizer* error_log_dir_grid;
	error_log_dir_grid = new wxGridSizer(1, 1, 0, 0);

	error_log_dir = new wxDirPickerCtrl(this, wxID_ANY, s_error_log_dir, _("ѡ���ļ���"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE);
	error_log_dir_grid->Add(error_log_dir, 1, wxALL | wxEXPAND, 5);


	setting_box->Add(error_log_dir_grid, 0, wxEXPAND, 5);

	save_settings = new wxButton(this, wxID_ANY, _("��������"), wxDefaultPosition, wxDefaultSize, 0);
	setting_box->Add(save_settings, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);


	this->SetSizer(setting_box);
	this->Layout();

	this->Centre(wxBOTH);

	// ���¼�
	save_settings->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(settings::SettingsSavedButtonClick), NULL, this);
	server_listen_port->Connect(wxEVT_TEXT, wxCommandEventHandler(settings::OnPORTchanged), NULL, this);
}

settings::~settings()
{
}

// ���ñ��水ť����¼�
void settings::SettingsSavedButtonClick(wxCommandEvent& event)
{
	if (SaveSettingsToFile()) {
		mainFrame->loadSettings("settings.conf");
		settingsSavedWindow = new setting_saved_information(nullptr, wxID_ANY, _("���óɹ���"));
		settingsSavedWindow->Show(true);
	}
}

bool settings::SaveSettingsToFile()
{
	wxTextFile file(wxT("settings.conf"));
	if (!file.Exists())
	{
		file.Create();
	}
	else
	{
		file.Open();
		file.Clear();
	}
	if (max_thread->GetValue() == "" || max_retransfer->GetValue() == "" || server_listen_port->GetValue() == "")
	{
		wxMessageBox("ȫ�ֱ�������Ϊ��ֵ", "����", wxICON_ERROR);
		return false;
	}

	file.AddLine(wxT("max_thread=") + max_thread->GetValue());
	file.AddLine(wxT("max_retransfer=") + max_retransfer->GetValue());
	file.AddLine(wxT("timeout=") + timeout->GetValue());
	file.AddLine(wxT("server_listen_port=") + server_listen_port->GetValue());
	file.AddLine(wxString::Format(wxT("console_log_switch=%d"), console_log_switch->GetValue() ? 1 : 0));
	file.AddLine(wxT("console_log_dir=") + console_log_dir->GetPath());
	file.AddLine(wxString::Format(wxT("error_log_switch=%d"), error_log_swtich->GetValue() ? 1 : 0));
	file.AddLine(wxT("error_log_dir=") + error_log_dir->GetPath());

	file.Write();
	file.Close();
	return true;
}

void settings::OnPORTchanged(wxCommandEvent& event)
{
	if (running)
	{
		wxMessageBox("�������ڷ���������ʱ�޸Ķ˿ڣ����ȹرշ�����", "�Ƿ�����", wxICON_ERROR);
		server_listen_port->SetValue(s_PORT);
	}
	else {
		wxString port = server_listen_port->GetValue();
		if (port.ToInt(&PORT) && PORT >= 0 && PORT <= 65535)
		{
			server_listen_port->SetBackgroundColour(wxColour(255, 255, 255));
		}
		else
		{
			server_listen_port->SetBackgroundColour(wxColour(255, 0, 0));
		}
	}

}

setting_saved_information::setting_saved_information(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxSize(400, 250), wxDefaultSize);

	wxGridSizer* setting_saved_grid;
	setting_saved_grid = new wxGridSizer(3, 1, 0, 0);

	setting_saved_grid->SetMinSize(wxSize(380, 230));
	setting_saved_information_text = new wxStaticText(this, wxID_ANY, _("����ɹ�"), wxDefaultPosition, wxDefaultSize, 0);
	setting_saved_information_text->Wrap(-1);
	setting_saved_information_text->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("����")));
	setting_saved_information_text->SetForegroundColour(wxColour(81, 174, 97));

	setting_saved_grid->Add(setting_saved_information_text, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_BOTTOM, 5);

	setting_saved_notification = new wxStaticText(this, wxID_ANY, _("�벻Ҫɾ������Ŀ¼�µ������ļ�"), wxDefaultPosition, wxDefaultSize, 0);
	setting_saved_notification->Wrap(-1);
	setting_saved_grid->Add(setting_saved_notification, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

	confim = new wxButton(this, wxID_ANY, _("ȷ��"), wxDefaultPosition, wxDefaultSize, 0);
	setting_saved_grid->Add(confim, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);


	this->SetSizer(setting_saved_grid);
	this->Layout();

	this->Centre(wxBOTH);
	// ���¼�
	confim->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(setting_saved_information::SettingsSavedConfirmed), NULL, this);
}

void setting_saved_information::SettingsSavedConfirmed(wxCommandEvent& event)
{
	settingsSavedWindow->Close();
}

setting_saved_information::~setting_saved_information()
{
}




void MyTaskBarIcon::OnLeftButtonDClick(wxTaskBarIconEvent&)
{
	if (g_mainFrame)
	{
		g_mainFrame->Show(true);
		g_mainFrame->Iconize(false);
	}
}

void MyTaskBarIcon::OnMenuRestore(wxCommandEvent&)
{
	if (g_mainFrame)
	{
		g_mainFrame->Show(true);
		g_mainFrame->Iconize(false);
	}
}

void MyTaskBarIcon::OnMenuExit(wxCommandEvent&)
{
	if (running) {
		wxMessageBox("���ȹرշ�����", "����", wxICON_ERROR);
		return;
	}
	if (g_mainFrame)
	{
		g_mainFrame->Destroy();
		this->Destroy();
		exit(0);
	}
}

wxMenu* MyTaskBarIcon::CreatePopupMenu()
{
	wxMenu* menu = new wxMenu;
	menu->Append(PU_RESTORE, _("�������"));
	menu->Append(PU_EXIT, _("�رճ���"));
	return menu;
}


// ʵ�� MyApp ��
wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	mainFrame = new main(nullptr, wxID_ANY, _("TFTP������ by Cormac@HUST CSE"));
	mainFrame->Show(true);
	return true;
}

