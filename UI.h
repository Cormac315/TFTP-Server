#pragma once
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/filepicker.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/grid.h>
#include <wx/frame.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/taskbar.h>
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class main
///////////////////////////////////////////////////////////////////////////////
class main : public wxFrame
{
private:
	
public:
	wxRadioBox* Server_Switch;
	wxButton* settingsButton;
	wxButton* console_mode;
	wxButton* check_update;
	wxStaticText* version;
	wxStaticText* text_server_dir;
	wxDirPickerCtrl* server_dir;
	wxStaticText* text_server_ip;
	wxChoice* choice_server_ip;
	wxTextCtrl* console_log;
	wxGrid* information_table;

	int currentRow = 0; // 表格当前行索引

	// 事件函数
	void OnClose(wxCloseEvent& event); // 关闭主窗口
	void OnSettingsButtonClick(wxCommandEvent& event);
	void OnCheckUpdateButtonClick(wxCommandEvent& event);
	void OnServerSwitch(wxCommandEvent& event);
	void OnConsoleModeButtonClick(wxCommandEvent& event);
	void OnServerDirChanged(wxFileDirPickerEvent& event);
	void OnVersionClick(wxMouseEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void loadSettings(const std::string& configFilePath);
	
	// 维护表格信息
	void AddClientInfo(const std::string& ip, const std::string& file, const std::string& startTime);
	void UpdateClientStatus(int row, const std::string& status, const std::string& throughput = "", int timeoutCount = 0);
	void TableStatus(int row, bool success);

	main(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("TFTP服务器 by Cormac"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(1225, 600), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

	~main();

};

///////////////////////////////////////////////////////////////////////////////
/// Class settings
///////////////////////////////////////////////////////////////////////////////
class settings : public wxFrame
{
private:
	bool SaveSettingsToFile();	

public:
	wxStaticText* global_var;
	wxStaticText* max_thread_text;
	wxTextCtrl* max_thread;
	wxStaticText* max_retransfer_text;
	wxTextCtrl* max_retransfer;
	wxStaticText* timeout_text;
	wxTextCtrl* timeout;
	wxStaticText* server_listen_port_text;
	wxTextCtrl* server_listen_port;
	wxStaticLine* listen_port;
	wxStaticText* log_record;
	wxStaticText* console_log_switch_text;
	wxCheckBox* console_log_switch;
	wxDirPickerCtrl* console_log_dir;
	wxStaticText* error_log_swtich_text;
	wxCheckBox* error_log_swtich;
	wxDirPickerCtrl* error_log_dir;
	wxButton* save_settings;

	// 事件函数
	void SettingsSavedButtonClick(wxCommandEvent& event);
	void OnPORTchanged(wxCommandEvent& event);
	settings(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("设置"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(389, 422), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

	~settings();

};

///////////////////////////////////////////////////////////////////////////////
/// Class setting_saved_information
///////////////////////////////////////////////////////////////////////////////
class setting_saved_information : public wxDialog
{
private:

protected:
	wxStaticText* setting_saved_information_text;
	wxStaticText* setting_saved_notification;
	wxButton* confim;

	// 事件函数
	void SettingsSavedConfirmed(wxCommandEvent& event);

public:

	setting_saved_information(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("设置成功！"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(401, 256), long style = wxDEFAULT_DIALOG_STYLE);

	~setting_saved_information();

};

extern main* mainFrame;
extern settings* settingsWindow;
extern setting_saved_information* settingsSavedWindow;

extern main* g_mainFrame; // 主窗口

// 托盘图标
class MyTaskBarIcon : public wxTaskBarIcon
{
public:
	MyTaskBarIcon() {}
	void OnLeftButtonDClick(wxTaskBarIconEvent&);
	void OnMenuRestore(wxCommandEvent&);
	void OnMenuExit(wxCommandEvent&);
	virtual wxMenu* CreatePopupMenu() override;
};




class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};



