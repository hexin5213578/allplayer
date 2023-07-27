
// MFC_DIALOG_2Dlg.cpp : 实现文件
//

#include "stdafx.h"

#include "MFC_DIALOG_2.h"
#include "MFC_DIALOG_2Dlg.h"
#include "afxdialogex.h"
#include <afxinet.h>
#include <afx.h>
#include <math.h>
#include <dbghelp.h>
#include <string>
#include "allplayer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define RECORD_SEARCH_RECT_HEIGHT  300

//全局变量 记录已登录账号信息
CString g_strLoginedAccount;
CString g_strLoginedPassword;
CString g_strLoginedIP;
CString g_strLoginedPort;
CString g_strLoginedToken;
BOOL g_bDialogTypeChecked;  //当前窗口类型记录
int LEFT_ACTION_BAR_WIDTH;
RecordNodeData g_curtRecordSubsection;  //当前录像分段
long g_iBusinessID;
double g_dRecordScale;  //录像倍速

int GenerateMiniDump(PEXCEPTION_POINTERS pExceptionPointers)
{
	// 定义函数指针
	typedef BOOL(WINAPI* MiniDumpWriteDumpT)(
		HANDLE,
		DWORD,
		HANDLE,
		MINIDUMP_TYPE,
		PMINIDUMP_EXCEPTION_INFORMATION,
		PMINIDUMP_USER_STREAM_INFORMATION,
		PMINIDUMP_CALLBACK_INFORMATION
		);
	// 从 "DbgHelp.dll" 库中获取 "MiniDumpWriteDump" 函数
	MiniDumpWriteDumpT pfnMiniDumpWriteDump = NULL;
	HMODULE hDbgHelp = LoadLibrary(_T("DbgHelp.dll"));
	if (NULL == hDbgHelp)
	{
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	pfnMiniDumpWriteDump = (MiniDumpWriteDumpT)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

	if (NULL == pfnMiniDumpWriteDump)
	{
		FreeLibrary(hDbgHelp);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	// 创建 dmp 文件件
	TCHAR szFileName[MAX_PATH] = { 0 };
	TCHAR* szVersion = _T("DemoDump_");
	SYSTEMTIME stLocalTime;
	GetLocalTime(&stLocalTime);
	wsprintf(szFileName, L"%s%04d%02d%02d-%02d%02d%02d.dmp",
		szVersion, stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
		stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);
	HANDLE hDumpFile = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if (INVALID_HANDLE_VALUE == hDumpFile)
	{
		FreeLibrary(hDbgHelp);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	// 写入 dmp 文件
	MINIDUMP_EXCEPTION_INFORMATION expParam;
	expParam.ThreadId = GetCurrentThreadId();
	expParam.ExceptionPointers = pExceptionPointers;
	expParam.ClientPointers = FALSE;
	pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
		hDumpFile, MiniDumpWithDataSegs, (pExceptionPointers ? &expParam : NULL), NULL, NULL);
	// 释放文件
	CloseHandle(hDumpFile);
	FreeLibrary(hDbgHelp);
	return EXCEPTION_EXECUTE_HANDLER;
}

//调用
LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo)
{
	// 这里做一些异常的过滤或提示
	if (IsDebuggerPresent())
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	return GenerateMiniDump(lpExceptionInfo);
}


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFC_DIALOG_2Dlg 对话框

void test_status_callback(long id, long type, long status, const char* info)
{
	//TRACE("test_status_callback lBusiness: %d, busType: %d, status: %d, info: \n", lBusiness, busType, status);
	//std::cout << "test_status_callback" << std::endl;
	/*CString strTip;
	strTip.Format(_T("test_status_callback lBusiness: %d, busType: %d, status: %d, info: %s \n"), lBusiness, busType, status, info);
	OutputDebugString(strTip);*/
	std::string callBackString;
	callBackString += std::to_string(id);
	callBackString += std::to_string(type);
	callBackString += std::to_string(status);
	callBackString += info;
	printf("recevie from biz[%d], status: [%ld].", id, status);
}

static void test_data_callback(long id, long type, long current, long total) 
{
	printf("recevie from biz[%d], [%ld]-[%ld].", id, current, total);
	//if (BUSINESS_TYPE_NETRECORD_START == busType || BUSINESS_TYPE_DOWNLOAD_START == busType  || BUSINESS_TYPE_URL_START == busType)
	//{
	//	CString strVal;
	//	strVal.Format(_T("%d/%d/%d/%d"), lBusiness, busType, current, total);
	//	auto app = AfxGetApp();

	//	if (app) {
	//		HWND hWnd = (HWND)((CFrameWnd*)app->m_pMainWnd->m_hWnd);
	//		if (NULL != hWnd) {
	//			//::SendMessage(hWnd, WM_PLAY_DATA_MSG, (WPARAM)(strVal.AllocSysString()), 0);
	//			::PostMessage(hWnd, WM_PLAY_DATA_MSG, (WPARAM)(strVal.AllocSysString()), 0);
	//		}
	//	}
	//}
}

#include <locale>
#include <codecvt>

// convert string to wstring
inline std::wstring to_wide_string(const std::string& input)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(input);
}
// convert wstring to string 
inline std::string to_byte_string(const std::wstring& input)
{
	//std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(input);
}

static void SetWaterMark()
{
	cJSON* root = nullptr, * cbusinessType = nullptr;
	int32_t businessType = TYPE_SET_WATERMARK;
	char cbuf[1024] = { 0 };
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	_snprintf_s(cbuf, 96, "%d", businessType);
	cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	cJSON* text = cJSON_CreateString(to_byte_string(L"宋赣\n@allcam2022").c_str());
	cJSON_AddItemToObject(root, "Text", text);

	cJSON* fontFile = cJSON_CreateString("allplayer.ttf");
	cJSON_AddItemToObject(root, "FontFile", fontFile);

	cJSON* fontcolor = cJSON_CreateString("0xf05b72");
	cJSON_AddItemToObject(root, "FontColor", fontcolor);

	cJSON* fontsize = cJSON_CreateNumber(40);
	cJSON_AddItemToObject(root, "FontSize", fontsize);

	cJSON* alpha = cJSON_CreateNumber(0.75);
	cJSON_AddItemToObject(root, "Alpha", alpha);

	cJSON* position = cJSON_CreateNumber(3);
	cJSON_AddItemToObject(root, "Position", position);

	cJSON* localTime = cJSON_CreateNumber(1);
	cJSON_AddItemToObject(root, "LocalTime", localTime);

	cJSON* layout = cJSON_CreateNumber(1);
	cJSON_AddItemToObject(root, "Layout", layout);

	cJSON* renderOn = cJSON_CreateNumber(1);
	cJSON_AddItemToObject(root, "RenderOn", renderOn);

	cJSON* writeOn = cJSON_CreateNumber(0);
	cJSON_AddItemToObject(root, "WriteOn", writeOn);
	
	result = cJSON_Print(root);
	char* szExperience = (char*)malloc(1024);
	memset(szExperience, 0, 1024);
	ap_lib_excute(-1, (char*)result, szExperience);
	cJSON_Delete(root);
	free(result);
	free(szExperience);
}


CMFC_DIALOG_2Dlg::CMFC_DIALOG_2Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFC_DIALOG_2Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pSplitScreenWindow = nullptr;
	//m_strLoginedAccount = "";
	//m_strLoginedPassword = "";
	//m_strLoginedIP = "0.0.0.0";
	//m_strLoginedPort = L"10000";
	//m_strLoginedToken = "";
	//全局变量 记录已登录账号信息
	g_iBusinessID = 0;
	g_bDialogTypeChecked = false;
	g_curtRecordSubsection.setCameraId(L"");
	LEFT_ACTION_BAR_WIDTH = 250;
	g_dRecordScale = 1;
	m_iCurtMultiBusinessID = -1;
	m_bIsCycling = false;

	string path = "test.log";
	int32_t level = 6;
	ap_lib_init((char*)path.c_str(), level);
	ap_lib_reg_status_callback(test_status_callback);
	ap_lib_reg_data_callback(test_data_callback);

	//SetWaterMark();
}

CMFC_DIALOG_2Dlg::~CMFC_DIALOG_2Dlg()
{
	ap_lib_unit();
}

void CMFC_DIALOG_2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_SPLIT_TYPE, m_splitWndSelectComboBox);
	DDX_Control(pDX, IDC_DEVICE_TREE, m_deviceTree);
	DDX_Control(pDX, IDC_BUTTON_LOGIN, m_loginBtn);
	DDX_Control(pDX, IDC_STATIC_ACCOUNT, m_accountLab);
	DDX_Control(pDX, IDC_EDIT_ACCOUNT, m_accountEdit);
	DDX_Control(pDX, IDC_STATIC_PASSWORD, m_passwordLab);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_passwordEdit);
	DDX_Control(pDX, IDC_STATIC_CLIENT_STATUS, m_appStatusLab);

	DDX_Control(pDX, IDC_STATIC_IP, m_ipLab);
	DDX_Control(pDX, IDC_EDIT_IP, m_ipEdit);
	DDX_Control(pDX, IDC_STATIC_PORT, m_portLab);
	DDX_Control(pDX, IDC_EDIT_PORT, m_portEdit);

	DDX_Control(pDX, IDC_STATIC_STREAM_TYPE, m_streamTypeLab);
	DDX_Control(pDX, IDC_COMBO_STREAM_TYPE, m_streamTypeComboBox);
	//m_mediaInfoBtn
	DDX_Control(pDX, IDC_BUTTON_MEDIA_INFO, m_mediaInfoBtn);

	DDX_Control(pDX, IDC_CHECK_RECORD, m_dialogTypeCheckBtn);

	DDX_Control(pDX, IDC_DATETIME_BEGIN, m_recordBeginTime);
	DDX_Control(pDX, IDC_DATETIME_END, m_recordEndTime);
	DDX_Control(pDX, IDC_COMBO_RECORD_TYPE, m_recordTypeComboBox);
	DDX_Control(pDX, IDC_BUTTON_SEARCH_RECORD, m_recordSearchBtn);
	DDX_Control(pDX, IDC_LIST_RECORD, m_recordList);

	DDX_Control(pDX, IDC_STATIC_RECORD_DEVICE_NAME, m_recordDevName);
	//m_recordScaleComboBox
	DDX_Control(pDX, IDC_COMBO_RECORD_SCALE, m_recordScaleComboBox);
	DDX_Control(pDX, IDC_STATIC_RECORD_BEGIN_TIME, m_recordBeginTimeLab);
	DDX_Control(pDX, IDC_SLIDER_RECORD, m_recordPlaySlider);
	DDX_Control(pDX, IDC_STATIC_RECORD_END_TIME, m_recordEndTimeLab);
	DDX_Control(pDX, IDC_BUTTON_SELECT_RECORD_SUBSECTION, m_selectRecordSubsection);
	//IDC_STATIC_PLAY_TIME
	DDX_Control(pDX, IDC_STATIC_PLAY_TIME, m_recordPlayTimeLab);
	DDX_Control(pDX, IDC_BUTTON_AUDIO_TALK, m_audioTalkBtn);

	DDX_Control(pDX, IDC_EDIT_VERIFICATION, m_verification);
	DDX_Control(pDX, IDC_BUTTON_MULTI_PLAY, m_multiPlayBtn);
	DDX_Control(pDX, IDC_EDIT_MULTI_PLAY_URL, m_multiPlayUrlEdit);
	//IDC_BUTTON_CYCLE
	DDX_Control(pDX, IDC_BUTTON_CYCLE, m_cycleBtn);

	//IDC_BUTTON_SELECT_RECORD_SUBSECTION

	m_ipEdit.SetWindowTextW(L"124.71.184.166");  //139.9.49.88    172.16.21.5
	//m_ipEdit.SetWindowTextW(L"172.16.21.5");  //124.70.162.47     
	m_portEdit.SetWindowTextW(L"10000");
	m_accountEdit.SetWindowTextW(L"admin");
	m_passwordEdit.SetWindowTextW(L"123456");

	//m_ipEdit.SetWindowTextW(L"183.207.208.105");  //172.16.21.4
	//m_portEdit.SetWindowTextW(L"10000");
	//m_accountEdit.SetWindowTextW(L"yuyinTest");
	//m_passwordEdit.SetWindowTextW(L"Smp@2017");

	m_appStatusLab.SetWindowTextW(L"未登录");


	long dwStyle = m_recordList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	dwStyle |= LVS_EX_GRIDLINES;
	//dwStyle |= LVS_EX_CHECKBOXES;
	m_recordList.SetExtendedStyle(dwStyle);

	//m_recordList.InsertItem(0, _T("第0行"));
	//m_recordList.SetItemText(0, 1, _T("0行1列"));
	//m_recordList.InsertItem(1, _T("第1行"));
	//m_recordList.SetItemText(1, 1, _T("1行1列"));

	m_recordList.InsertColumn(0, _T("序号"), 0, 36);
	m_recordList.InsertColumn(1, _T("开始时间"), 1, 102);
	m_recordList.InsertColumn(2, _T("结束时间"), 2, 102);

	//for (int i = 0; i < 20; i++)
	//{
	//	m_recordList.InsertItem(i, _T("05/16 10:10:10"));
	//	m_recordList.SetItemText(i, 1, _T("05/17 10:10:10"));
	//}

	InitClientView();
}

BEGIN_MESSAGE_MAP(CMFC_DIALOG_2Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_TYPE, OnSplitWndSelectChange)
	ON_CBN_SELCHANGE(IDC_COMBO_RECORD_SCALE, OnRecordScaleSelectChange)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, OnLoginBtnClicked) //登录按钮点击响应函数
	ON_BN_CLICKED(IDC_CHECK_RECORD, OnDialogTypeChecked) //窗口类型点击响应函数
	ON_BN_CLICKED(IDC_BUTTON_SEARCH_RECORD, OnRecordSearchBtnClicked)  //录像查询按钮点击响应函数

	ON_NOTIFY(NM_DBLCLK, IDC_DEVICE_TREE, &CMFC_DIALOG_2Dlg::OnNMDblclkDeviceTree)


	ON_BN_CLICKED(IDC_BUTTON_SELECT_RECORD_SUBSECTION, &CMFC_DIALOG_2Dlg::OnBnClickedButtonSelectRecordSubsection)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_RECORD, &CMFC_DIALOG_2Dlg::OnNMReleasedcaptureSliderRecord)
	//ON_NOTIFY(TRBN_THUMBPOSCHANGING, IDC_SLIDER_RECORD, &CMFC_DIALOG_2Dlg::OnTRBNThumbPosChangingSliderRecord)
	ON_MESSAGE(WM_PLAY_DATA_MSG, &CMFC_DIALOG_2Dlg::OnPlayDataMsg) //OnCountMsg是自定义的消息处理函数，可以在这个函数里面进行自定义的消息处理代码
	ON_MESSAGE(WM_MULTI_PLAY_MSG, &CMFC_DIALOG_2Dlg::OnMultiPlayMsg)
	ON_BN_CLICKED(IDC_BUTTON_MEDIA_INFO, &CMFC_DIALOG_2Dlg::OnBnClickedButtonMediaInfo)
	ON_BN_CLICKED(IDC_BUTTON_AUDIO_TALK, &CMFC_DIALOG_2Dlg::OnBnClickedButtonAudioTalk)
	ON_BN_CLICKED(IDC_BUTTON_REFREASH, &CMFC_DIALOG_2Dlg::OnBnClickedButtonRefreash)
	ON_BN_CLICKED(IDC_BUTTON_MULTI_PLAY, &CMFC_DIALOG_2Dlg::OnBnClickedButtonMultiPlay)
	ON_BN_CLICKED(IDC_BUTTON_CYCLE, &CMFC_DIALOG_2Dlg::OnBnClickedButtonCycle)
END_MESSAGE_MAP()

// CMFC_DIALOG_2Dlg 消息处理程序

BOOL CMFC_DIALOG_2Dlg::OnInitDialog()
{
	// 加入崩溃dump文件功能
	SetUnhandledExceptionFilter(ExceptionFilter);

	CDialogEx::OnInitDialog();
	// 将“关于...”菜单项添加到系统菜单中。
	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	CComboBox* pSplitWndComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_SPLIT_TYPE);
	if (NULL != pSplitWndComboBox)
	{
		pSplitWndComboBox->InsertString(0, L"单屏");
		pSplitWndComboBox->InsertString(1, L"4分屏");
		pSplitWndComboBox->InsertString(2, L"9分屏");
		pSplitWndComboBox->InsertString(3, L"16分屏");
		pSplitWndComboBox->InsertString(4, L"25分屏");
		pSplitWndComboBox->InsertString(5, L"36分屏");
		pSplitWndComboBox->InsertString(6, L"49分屏");
		pSplitWndComboBox->InsertString(7, L"64分屏");

		pSplitWndComboBox->SetCurSel(0);
	}

	CComboBox* pStreamComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_STREAM_TYPE);
	if (NULL != pStreamComboBox)
	{
		pStreamComboBox->InsertString(0, L"主码流");
		pStreamComboBox->InsertString(1, L"子码流");
		pStreamComboBox->SetCurSel(0);
	}

	CComboBox* pRecordScaleComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_RECORD_SCALE);
	if (NULL != pRecordScaleComboBox)
	{
		int iIndex = 0;
		for (int i = -2; i <= 2; i++)
		{
			CString strItemTxt;
			double dScale = pow(2, i);
			strItemTxt.Format(_T("%.2f倍速"), dScale);
			pRecordScaleComboBox->InsertString(iIndex++, strItemTxt);
		}
		pRecordScaleComboBox->SetCurSel(2);
	}

	m_ImageList.Create(16, 16, ILC_COLOR32, 10, CLR_NONE);
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON1));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON2));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_ICON3));

	//m_pSplitScreenWindow = new CSplitScreenWindow(this);
	//m_pSplitScreenWindow->Create(IDD_SPLITSCREENWINDOW, this);
	//InitClientView();
	//m_pSplitScreenWindow->ShowWindow(SW_SHOW);

	m_deviceTree.SetImageList(&m_ImageList, TVSIL_NORMAL);
	//m_deviceTree.SetBkColor(m_deviceTree.m_ImageList.GetBkColor());

	//设置录像列表复选框 参考地址： https://blog.csdn.net/xiaofen17458/article/details/106240230/

	CDateTimeCtrl* pBeginDateTM = (CDateTimeCtrl*)GetDlgItem(IDC_DATETIME_BEGIN);
	//pBeginDateTM->ModifyStyle(NULL, TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT);
	if (NULL != pBeginDateTM)
		pBeginDateTM->SetFormat(L"yyyy-MM-dd HH:mm:ss");

	CDateTimeCtrl* pEndDateTM = (CDateTimeCtrl*)GetDlgItem(IDC_DATETIME_END);
	//pEndDateTM->ModifyStyle(NULL, TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT);
	if (NULL != pEndDateTM)
		pEndDateTM->SetFormat(L"yyyy-MM-dd HH:mm:ss");

	//IDC_COMBO_RECORD_TYPE
	CComboBox* pRecordType = (CComboBox*)GetDlgItem(IDC_COMBO_RECORD_TYPE);
	if (NULL != pRecordType)
	{
		pRecordType->InsertString(0, L"前端");
		pRecordType->InsertString(1, L"平台");
		pRecordType->SetCurSel(1);
	}

	//IDC_BUTTON_SEARCH_RECORD
	//CButton* pRecordSearchBtn = (CButton*)GetDlgItem(IDC_BUTTON_SEARCH_RECORD);

	// TODO:  在此添加额外的初始化代码
	InitClientView();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFC_DIALOG_2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFC_DIALOG_2Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFC_DIALOG_2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFC_DIALOG_2Dlg::OnSplitWndSelectChange()
{
	int iCurtType = m_splitWndSelectComboBox.GetCurSel();
	if (iCurtType < 0 || iCurtType >= 8)
	{
		return;
	}
	int splitCntArr[8] = { 1,4,9,16,25,36,49,64 };
	m_pSplitScreenWindow->SetSplitType(splitCntArr[iCurtType]);
}

void CMFC_DIALOG_2Dlg::OnLoginBtnClicked()
{
	Login();
}

void CMFC_DIALOG_2Dlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	//if (nullptr == m_pSplitScreenWindow)
	//{
	//	m_pSplitScreenWindow = new CSplitScreenWindow(this);
	//	m_pSplitScreenWindow->Create(IDD_SPLITSCREENWINDOW, this);
	//	InitClientView();
	//	m_pSplitScreenWindow->ShowWindow(SW_SHOW);
	//}
	//else
	//{
	//	InitClientView();
	//}

	if (nullptr == m_pSplitScreenWindow)
	{
		m_pSplitScreenWindow = new CSplitScreenWindow(this);
		m_pSplitScreenWindow->Create(IDD_SPLITSCREENWINDOW, this);
		m_pSplitScreenWindow->ShowWindow(SW_SHOW);
	}

	InitClientView();
}

void CMFC_DIALOG_2Dlg::InitClientView()
{
	CRect rect;
	GetClientRect(&rect);
	m_iClientWidth = rect.Width();
	m_iClientHeight = rect.Height();

	//设备树
	if (NULL != m_deviceTree)
	{
		if (g_bDialogTypeChecked)
		{
			m_deviceTree.MoveWindow(rect.left, rect.top, LEFT_ACTION_BAR_WIDTH, m_iClientHeight - RECORD_SEARCH_RECT_HEIGHT);
		}
		else
		{
			m_deviceTree.MoveWindow(rect.left, rect.top, LEFT_ACTION_BAR_WIDTH, m_iClientHeight);
		}
	}

	//录像查询区域
	if (NULL != m_recordBeginTime && NULL != m_recordEndTime && NULL != m_recordTypeComboBox && NULL != m_recordSearchBtn && NULL != m_recordList)
	{
		if (g_bDialogTypeChecked)
		{
			//if (IsWindow(m_recordBeginTime)) //判断窗口是否存在  !=NULL判断也可以
			//{
			m_recordBeginTime.MoveWindow(rect.left, rect.bottom - RECORD_SEARCH_RECT_HEIGHT, LEFT_ACTION_BAR_WIDTH, 25);
			m_recordEndTime.MoveWindow(rect.left, rect.bottom - RECORD_SEARCH_RECT_HEIGHT + 25, LEFT_ACTION_BAR_WIDTH, 25);
			m_recordTypeComboBox.MoveWindow(rect.left, rect.bottom - RECORD_SEARCH_RECT_HEIGHT + 50, LEFT_ACTION_BAR_WIDTH, 25);
			m_recordSearchBtn.MoveWindow(rect.left, rect.bottom - RECORD_SEARCH_RECT_HEIGHT + 75, LEFT_ACTION_BAR_WIDTH, 25);
			m_recordList.MoveWindow(rect.left, rect.bottom - RECORD_SEARCH_RECT_HEIGHT + 100, LEFT_ACTION_BAR_WIDTH, RECORD_SEARCH_RECT_HEIGHT - 125);
			m_selectRecordSubsection.MoveWindow(rect.left, rect.bottom - 25, LEFT_ACTION_BAR_WIDTH, 25);

			m_recordBeginTime.ShowWindow(SW_SHOW);
			m_recordEndTime.ShowWindow(SW_SHOW);
			m_recordTypeComboBox.ShowWindow(SW_SHOW);
			m_recordSearchBtn.ShowWindow(SW_SHOW);
			m_recordList.ShowWindow(SW_SHOW);
			m_selectRecordSubsection.ShowWindow(SW_SHOW);
			//}
		}
		else
		{
			//if (IsWindow(m_recordBeginTime))
			//{
			m_recordBeginTime.ShowWindow(SW_HIDE);
			m_recordEndTime.ShowWindow(SW_HIDE);
			m_recordTypeComboBox.ShowWindow(SW_HIDE);
			m_recordSearchBtn.ShowWindow(SW_HIDE);
			m_recordList.ShowWindow(SW_HIDE);
			m_selectRecordSubsection.ShowWindow(SW_HIDE);
			//}
		}
	}

	//顶部操作栏
	if (NULL != m_ipLab)
		m_ipLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH, rect.top + 5, 10, 25);

	if (NULL != m_ipEdit)
		m_ipEdit.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 15, rect.top + 2, 110, 25);

	if (NULL != m_portLab)
		m_portLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 130, rect.top + 5, 40, 25);

	if (NULL != m_portEdit)
		m_portEdit.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 155, rect.top + 2, 55, 25);

	if (NULL != m_accountLab)
		m_accountLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 215, rect.top + 5, 40, 25);
	if (NULL != m_accountEdit)
		m_accountEdit.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 245, rect.top + 2, 90, 25);
	if (NULL != m_passwordLab)
		m_passwordLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 340, rect.top + 5, 40, 25);
	if (NULL != m_passwordEdit)
		m_passwordEdit.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 370, rect.top + 2, 80, 25);
	if (NULL != m_loginBtn)
		m_loginBtn.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 455, rect.top + 2, 50, 25);
	if (NULL != m_appStatusLab)
		m_appStatusLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 510, rect.top + 5, 100, 25);

	//分屏窗口
	if (NULL != m_pSplitScreenWindow)
	{
		m_pSplitScreenWindow->MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH, rect.top + TOP_ACTION_BAR_HEIGHT, m_iClientWidth, m_iClientHeight - TOP_ACTION_BAR_HEIGHT - BOTTOM_ACTION_BAR_HEIGHT);
		m_pSplitScreenWindow->InitSplitScreenView();

		//CRect splitWindowRect;
		//m_pSplitScreenWindow->GetWindowRect(&splitWindowRect);
	}

	//底部操作栏
	//CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_SPLIT_TYPE);
	if (NULL != m_splitWndSelectComboBox)
	{
		if (g_bDialogTypeChecked)
		{
			m_splitWndSelectComboBox.ShowWindow(SW_HIDE);
		}
		else
		{
			m_splitWndSelectComboBox.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 5, rect.bottom - 35, 95, 25);
			m_splitWndSelectComboBox.ShowWindow(SW_SHOW);
		}
	}

	//CStatic* pStreamTypeLab = (CStatic*)GetDlgItem(IDC_STATIC_STREAM_TYPE);
	if (NULL != m_streamTypeLab)
	{
		if (g_bDialogTypeChecked)
		{
			m_streamTypeLab.ShowWindow(SW_HIDE);
		}
		else
		{
			m_streamTypeLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 105, rect.bottom - 35, 40, 25);
			m_streamTypeLab.ShowWindow(SW_SHOW);
		}
	}

	//CComboBox* pStreamTypeComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_STREAM_TYPE);
	if (NULL != m_streamTypeComboBox)
	{
		if (g_bDialogTypeChecked)
		{
			m_streamTypeComboBox.ShowWindow(SW_HIDE);
		}
		else
		{
			m_streamTypeComboBox.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 150, rect.bottom - 35, 75, 25);
			m_streamTypeComboBox.ShowWindow(SW_SHOW);
		}
	}

	if (NULL != m_mediaInfoBtn) {

		if (g_bDialogTypeChecked)
		{
			m_mediaInfoBtn.ShowWindow(SW_HIDE);
		}
		else
		{
			m_mediaInfoBtn.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 245, rect.bottom - 35, 75, 25);
			m_mediaInfoBtn.ShowWindow(SW_SHOW);
		}
	}

	if (NULL != m_audioTalkBtn) {

		if (g_bDialogTypeChecked)
		{
			m_audioTalkBtn.ShowWindow(SW_HIDE);
		}
		else
		{
			m_audioTalkBtn.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 320, rect.bottom - 35, 120, 25);
			m_audioTalkBtn.ShowWindow(SW_SHOW);
		}
	}


	if (NULL != m_recordDevName)
	{
		if (g_bDialogTypeChecked)
		{
			m_recordDevName.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH, rect.bottom - 40, 100, 35);
			m_recordDevName.ShowWindow(SW_SHOW);
		}
		else
		{
			m_recordDevName.ShowWindow(SW_HIDE);
		}
	}

	if (NULL != m_recordScaleComboBox)
	{
		if (g_bDialogTypeChecked)
		{
			m_recordScaleComboBox.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 100, rect.bottom - 30, 80, 35);
			m_recordScaleComboBox.ShowWindow(SW_SHOW);
		}
		else
		{
			m_recordScaleComboBox.ShowWindow(SW_HIDE);
		}
	}

	if (NULL != m_recordBeginTimeLab)
	{
		if (g_bDialogTypeChecked)
		{
			m_recordBeginTimeLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 180, rect.bottom - 40, 100, 35);
			m_recordBeginTimeLab.ShowWindow(SW_SHOW);
		}
		else
		{
			m_recordBeginTimeLab.ShowWindow(SW_HIDE);
		}
	}

	if (NULL != m_recordPlaySlider)
	{
		if (g_bDialogTypeChecked)
		{
			m_recordPlayTimeLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 280 + (rect.Width() - 400 - LEFT_ACTION_BAR_WIDTH - 150) / 2, rect.bottom - 45, 150, 20);
			m_recordPlaySlider.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 280, rect.bottom - 30, rect.Width() - 500 - LEFT_ACTION_BAR_WIDTH, 20);

			//m_recordPlayTimeLab.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 200 + (rect.Width() - 400 - LEFT_ACTION_BAR_WIDTH - 150) / 2, rect.bottom - 40, 150, 35);
			//m_recordPlaySlider.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 200, rect.bottom + 30, rect.Width() - 400 - LEFT_ACTION_BAR_WIDTH, 20);

			m_recordPlayTimeLab.ShowWindow(SW_SHOW);
			m_recordPlaySlider.ShowWindow(SW_SHOW);
		}
		else
		{
			m_recordPlayTimeLab.ShowWindow(SW_HIDE);
			m_recordPlaySlider.ShowWindow(SW_HIDE);
		}
	}

	if (NULL != m_recordEndTimeLab)
	{
		if (g_bDialogTypeChecked)
		{
			m_recordEndTimeLab.MoveWindow(rect.right - 200, rect.bottom - 40, 100, 35);
			m_recordEndTimeLab.ShowWindow(SW_SHOW);
		}
		else
		{
			m_recordEndTimeLab.ShowWindow(SW_HIDE);
		}
	}

	if (NULL != m_multiPlayBtn) { //拼接流开/关按钮
		if (g_bDialogTypeChecked) {
			m_multiPlayBtn.ShowWindow(SW_HIDE);
		}
		else
		{
			m_multiPlayBtn.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 455, rect.bottom - 35, 70, 25);
			m_multiPlayBtn.ShowWindow(SW_SHOW);
		}
	}

	if (NULL != m_multiPlayUrlEdit) { //拼接流开/关按钮
		if (g_bDialogTypeChecked) {
			m_multiPlayUrlEdit.ShowWindow(SW_HIDE);
		}
		else
		{
			m_multiPlayUrlEdit.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 530, rect.bottom - 35, 120, 25);
			m_multiPlayUrlEdit.ShowWindow(SW_SHOW);
		}
	}

	if (NULL != m_cycleBtn) { //拼接流开/关按钮
		if (g_bDialogTypeChecked) {
			m_cycleBtn.ShowWindow(SW_HIDE);
		}
		else
		{
			m_cycleBtn.MoveWindow(rect.left + LEFT_ACTION_BAR_WIDTH + 655, rect.bottom - 35, 70, 25);
			m_cycleBtn.ShowWindow(SW_SHOW);
		}
	}


	//窗口类型
	//CButton* pTypeCheckBox = (CButton*)GetDlgItem(IDC_CHECK_RECORD);
	if (NULL != m_dialogTypeCheckBtn)
	{
		m_dialogTypeCheckBtn.MoveWindow(rect.right - 95, rect.bottom - 35, 90, 25);
	}
}

//https://blog.csdn.net/a956602523/article/details/50956530
void CMFC_DIALOG_2Dlg::Login()
{
	//获取登录基本信息
	CString strIP;
	m_ipEdit.GetWindowTextW(strIP);
	CString strPort;
	m_portEdit.GetWindowTextW(strPort);
	CString strAccount;
	m_accountEdit.GetWindowTextW(strAccount);
	CString strPassWord;
	m_passwordEdit.GetWindowTextW(strPassWord);
	CString strVerification;
	m_verification.GetWindowTextW(strVerification);

	if (strIP.IsEmpty() || strPort.IsEmpty() || strAccount.IsEmpty() || strPassWord.IsEmpty())
	{
		AfxMessageBox(L"登录信息不完整");
		return;
	}

	CInternetSession session;
	//设置超时和连接重试次数
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 1000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 1);
	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 1000);
	int iPort = _ttoi(strPort);
	CHttpConnection* pConnection = session.GetHttpConnection(strIP, (INTERNET_PORT)iPort, strAccount, strPassWord);
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_LOGIN, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc";
	USES_CONVERSION;
	CString cStrFormData = L"{\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"captcha\":\"" + strVerification + "\"}";
	CHAR* strFormData = T2A(cStrFormData);

	//CHAR* strFormData = "{\"captcha\":\"qwer\",\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\"}";
	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);
	if (dwRet == HTTP_STATUS_OK)
	{
		//读取http response数据
		int len = pFile->GetLength();
		char buf[2000];
		int numread;
		CString strJsonContent = L"";

		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
		{
			buf[numread] = '\0';
			strJsonContent += UTF8ToUnicode(buf);
		}
		//AfxMessageBox(strJsonContent);

		const int iLength = strJsonContent.GetLength();

		USES_CONVERSION;
		LPSTR lpszTest = T2A(strJsonContent);
		cJSON* root = cJSON_Parse(lpszTest);
		if (!root) {
			return;
		}

		cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
		if (!jResCode || jResCode->valueint != 0)
		{
			m_appStatusLab.SetWindowTextW(L"登录失败:");
			if (root) {
				cJSON_Delete(root);
			}
			return;
		}

		cJSON* jAccessToken = cJSON_GetObjectItem(root, "accessToken");
		if (!jAccessToken) {
			if (root) {
				cJSON_Delete(root);
			}
			return;
		}
		if (!jAccessToken->valuestring) {
			if (root) {
				cJSON_Delete(root);
			}
			return;
		}

		//m_strLoginedToken = CString(jAccessToken->valuestring);
		//m_strLoginedIP = strIP;
		//m_strLoginedPort = strPort;
		//m_strLoginedAccount = strAccount;
		//m_strLoginedPassword = strPassWord;

		g_strLoginedAccount = strAccount;
		g_strLoginedPassword = strPassWord;
		g_strLoginedIP = strIP;
		g_strLoginedPort = strPort;
		g_strLoginedToken = CString(jAccessToken->valuestring);

		m_appStatusLab.SetWindowTextW(L"已登录");
		if (root) {
			cJSON_Delete(root);
		}
		//获取设备树
		GetDeviceTree();

		//网关测试
		//GetDeviceTreeOnGateWay();

		//加载设备树
		LoadDeviceTree();
	}
	else
	{
		m_appStatusLab.SetWindowTextW(L"登录失败");
	}

	pConnection->Close();

	session.Close();
	pFile->Close();
	pFile->Abort();
	delete pFile;
}

void CMFC_DIALOG_2Dlg::GetDeviceTreeOnGateWay()
{
	CInternetSession session;
	//设置超时和连接重试次数
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 10000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 3);
	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 5000);

	CString strLoginedIp = L"114.116.223.94";
	int iLoginedPort = 10010;
	CString strGetDeviceList = L"/api/dev/list";

	int iPort = _ttoi(g_strLoginedPort);
	CHttpConnection* pConnection = session.GetHttpConnection(strLoginedIp, (INTERNET_PORT)iLoginedPort, g_strLoginedAccount, g_strLoginedPassword);
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, strGetDeviceList, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc";
	CHAR* strFormData = "{\"type\":2,\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\"}";
	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);
	if (dwRet == HTTP_STATUS_OK)
	{
		//读取http response数据
		int len2 = pFile->GetLength();
		char buf[2000];
		int numread;
		CString strJsonContent = L"";
		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
		{
			buf[numread] = '\0';
			strJsonContent += UTF8ToUnicode(buf);
		}

		const int iLength = strJsonContent.GetLength();

		int len = WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), NULL, 0, NULL, NULL);
		char* pFileName = new char[len + 1];   //以字节为单位
		memset(pFileName, 0, len);
		WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), pFileName, len, NULL, NULL);


		cJSON* root = cJSON_Parse(pFileName);
		if (!root) {
			m_appStatusLab.SetWindowTextW(L"获取设备树失败");
			return;
		}

		cJSON* jResToken = cJSON_GetObjectItem(root, "resultCode");
		if (!jResToken || jResToken->valueint != 0)
		{
			m_appStatusLab.SetWindowTextW(L"获取设备树失败");
			if (root) {
				cJSON_Delete(root);
			}
			return;
		}

		//解析设备树
		cJSON* nodeList = cJSON_GetObjectItem(root, "devList");
		if (!nodeList || cJSON_GetObjectItem(nodeList, ""))
		{
			m_appStatusLab.SetWindowTextW(L"获取设备树失败");
			if (root) {
				cJSON_Delete(root);
			}
			return;
		}

		int iNodeListSize = cJSON_GetArraySize(nodeList);

		for (int iIndex = 0; iIndex < iNodeListSize; ++iIndex)
		{
			DeviceTreeNodeData nodeData;
			nodeData.setLevel(0);
			nodeData.setParentNodeId(L"");
			cJSON* pNodeJson = cJSON_GetArrayItem(nodeList, iIndex);
			if (pNodeJson)
			{
				//解析节点数据
				if (cJSON_GetObjectItem(pNodeJson, "cameraId") && cJSON_GetObjectItem(pNodeJson, "cameraId")->valuestring)
				{
					nodeData.setNodeId(CString(cJSON_GetObjectItem(pNodeJson, "cameraId")->valuestring));
				}

				if (cJSON_GetObjectItem(pNodeJson, "cameraName") && cJSON_GetObjectItem(pNodeJson, "cameraName")->valuestring)
				{
					nodeData.setNodeName(CString(cJSON_GetObjectItem(pNodeJson, "cameraName")->valuestring));
				}

				nodeData.setNodeType(L"2");

				if (cJSON_GetObjectItem(pNodeJson, "devStatus"))
				{
					CString strOnlineStatus;
					strOnlineStatus.Format(_T("%d"), cJSON_GetObjectItem(pNodeJson, "devStatus")->valueint == 0 ? 2 : 1);
					nodeData.setDevOnlineStatus(strOnlineStatus);
				}
				m_treeNodeDataList.push_back(nodeData);
			}
		}
		if (root) {
			cJSON_Delete(root);
		}
	}
	else
	{
		m_appStatusLab.SetWindowTextW(L"获取设备树失败");
	}

	session.Close();
	pFile->Close();
	delete pFile;
}

void CMFC_DIALOG_2Dlg::GetDeviceTree()
{
	CInternetSession session;
	//设置超时和连接重试次数
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 10000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 3);
	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 5000);
	int iPort = _ttoi(g_strLoginedPort);
	CHttpConnection* pConnection = session.GetHttpConnection(g_strLoginedIP, (INTERNET_PORT)iPort, g_strLoginedAccount, g_strLoginedPassword);
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_GET_DEVICE_TREE, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc";
	CHAR* strFormData = "{\"type\":2,\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\"}";
	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);
	if (dwRet == HTTP_STATUS_OK)
	{
		//读取http response数据
		int len2 = pFile->GetLength();
		char buf[2000];
		int numread;
		CString strJsonContent = L"";
		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
		{
			buf[numread] = '\0';
			strJsonContent += UTF8ToUnicode(buf);
		}
		//AfxMessageBox(strJsonContent);

		const int iLength = strJsonContent.GetLength();

		int len = WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), NULL, 0, NULL, NULL);
		char* pFileName = new char[len + 1];   //以字节为单位
		memset(pFileName, 0, len);
		WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), pFileName, len, NULL, NULL);


		cJSON* root = cJSON_Parse(pFileName);
		if (!root) {
			m_appStatusLab.SetWindowTextW(L"获取设备树失败");
			return;
		}

		cJSON* jResToken = cJSON_GetObjectItem(root, "resultCode");
		if (!jResToken || jResToken->valueint != 0)
		{
			m_appStatusLab.SetWindowTextW(L"获取设备树失败");
			if (root) {
				cJSON_Delete(root);
			}
			return;
		}

		//解析设备树
		cJSON* nodeList = cJSON_GetObjectItem(root, "nodeList");
		if (!nodeList || cJSON_GetObjectItem(nodeList, ""))
		{
			m_appStatusLab.SetWindowTextW(L"获取设备树失败");
			if (root) {
				cJSON_Delete(root);
			}
			return;
		}

		int iNodeListSize = cJSON_GetArraySize(nodeList);

		for (int iIndex = 0; iIndex < iNodeListSize; ++iIndex)
		{
			DeviceTreeNodeData nodeData;
			nodeData.setLevel(0);
			nodeData.setParentNodeId(L"");
			cJSON* pNodeJson = cJSON_GetArrayItem(nodeList, iIndex);
			if (pNodeJson)
			{
				//解析节点数据
				if (cJSON_GetObjectItem(pNodeJson, "id") && cJSON_GetObjectItem(pNodeJson, "id")->valuestring)
				{
					nodeData.setNodeId(CString(cJSON_GetObjectItem(pNodeJson, "id")->valuestring));
				}

				if (cJSON_GetObjectItem(pNodeJson, "label") && cJSON_GetObjectItem(pNodeJson, "label")->valuestring)
				{
					nodeData.setNodeName(CString(cJSON_GetObjectItem(pNodeJson, "label")->valuestring));
				}

				if (cJSON_GetObjectItem(pNodeJson, "type") && cJSON_GetObjectItem(pNodeJson, "type")->valuestring)
				{
					nodeData.setNodeType(CString(cJSON_GetObjectItem(pNodeJson, "type")->valuestring));
				}

				if (cJSON_GetObjectItem(pNodeJson, "payload"))
				{
					cJSON* payloadJson = cJSON_GetObjectItem(pNodeJson, "payload");
					if (payloadJson)
					{
						//if (cJSON_GetObjectItem(payloadJson, "organizationId"))
						//{
						//	CString strParentId;
						//	strParentId.Format(_T("%d"), cJSON_GetObjectItem(payloadJson, "organizationId")->valueint);
						//	nodeData.setParentNodeId(strParentId);
						//}

						//if (cJSON_GetObjectItem(pNodeJson, "status"))
						//{
						//	CString strOnlineStatus;
						//	strOnlineStatus.Format(_T("%d"), cJSON_GetObjectItem(pNodeJson, "status")->valueint);
						//	nodeData.setDevOnlineStatus(strOnlineStatus);
						//}

						if (cJSON_GetObjectItem(payloadJson, "totalCount"))
						{
							nodeData.setTotalCount(cJSON_GetObjectItem(payloadJson, "totalCount")->valueint);
						}

						if (cJSON_GetObjectItem(payloadJson, "onlineCount"))
						{
							nodeData.setOnlineCount(cJSON_GetObjectItem(payloadJson, "onlineCount")->valueint);
						}
					}
				}

				if (cJSON_GetObjectItem(pNodeJson, "children"))
				{
					nodeData.parseJsonData(cJSON_GetObjectItem(pNodeJson, "children"));
				}

				m_treeNodeDataList.push_back(nodeData);
			}
		}
		if (root) {
			cJSON_Delete(root);
		}
	}
	else
	{
		m_appStatusLab.SetWindowTextW(L"获取设备树失败");
	}

	session.Close();
	pFile->Close();
	delete pFile;
}

void CMFC_DIALOG_2Dlg::LoadDeviceTree()
{
	if (m_treeNodeDataList.size() > 0)
	{
		for each (DeviceTreeNodeData node in m_treeNodeDataList)
		{
			int iImgType = 0;
			CString strNodeName = node.getNodeName();

			//分组
			if ("1" == node.getNodeType())
			{
				iImgType = 0;
				CString strCount;
				strCount.Format(_T("(%d/%d)"), node.getOnlineCount(), node.getTotalCount());
				strNodeName += strCount;
			}
			else
			{
				//在线
				if ("1" == node.getDevOnlineStatus())
				{
					iImgType = 1;
				}
				else
				{
					iImgType = 2;
				}
			}

			HTREEITEM item = m_deviceTree.InsertItem(strNodeName, iImgType, iImgType);

			DeviceTreeNodeData* pData = new DeviceTreeNodeData;
			pData->setDevOnlineStatus(node.getDevOnlineStatus());
			pData->setLevel(node.getLevel());
			pData->setNodeId(node.getNodeId());
			pData->setNodeName(node.getNodeName());
			pData->setNodeType(node.getNodeType());
			pData->setOnlineCount(node.getOnlineCount());
			pData->setParentNodeId(node.getParentNodeId());
			pData->setTotalCount(node.getTotalCount());

			m_deviceTree.SetItemData(item, (DWORD_PTR)pData);
			if ("1" == node.getNodeType())
			{
				list<DeviceTreeNodeData> childNodeList = node.getChildNodeList();
				for each (DeviceTreeNodeData childNodeData in childNodeList)
				{
					LoadNode(item, childNodeData);
				}
			}
		}
	}
}

void CMFC_DIALOG_2Dlg::LoadNode(HTREEITEM parentNode, DeviceTreeNodeData data)
{
	int iImgType = 0;
	CString strNodeName = data.getNodeName();

	//分组
	if ("1" == data.getNodeType())
	{
		iImgType = 0;
		CString strCount;
		strCount.Format(_T("(%d/%d)"), data.getOnlineCount(), data.getTotalCount());
		strNodeName += strCount;
	}
	else
	{
		//在线
		if ("1" == data.getDevOnlineStatus())
		{
			iImgType = 1;
		}
		else
		{
			iImgType = 2;
		}
	}

	HTREEITEM item = m_deviceTree.InsertItem(strNodeName, iImgType, iImgType, parentNode);

	DeviceTreeNodeData* pData = new DeviceTreeNodeData;
	pData->setDevOnlineStatus(data.getDevOnlineStatus());
	pData->setLevel(data.getLevel());
	pData->setNodeId(data.getNodeId());
	pData->setNodeName(data.getNodeName());
	pData->setNodeType(data.getNodeType());
	pData->setOnlineCount(data.getOnlineCount());
	pData->setParentNodeId(data.getParentNodeId());
	pData->setTotalCount(data.getTotalCount());

	m_deviceTree.SetItemData(item, (DWORD_PTR)pData);
	if ("1" == data.getNodeType())
	{
		list<DeviceTreeNodeData> childNodeList = data.getChildNodeList();
		for each (DeviceTreeNodeData childNode in childNodeList)
		{
			LoadNode(item, childNode);
		}
	}
}

CString CMFC_DIALOG_2Dlg::GetRealTimeLive(CString strCameraID)
{
	CString strLive = L"";

	return strLive;
}

void CMFC_DIALOG_2Dlg::OnNMDblclkDeviceTree(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO:  在此添加控件通知处理程序代码
	//*pResult = 0;

	if (g_bDialogTypeChecked)
	{
		//录像回放不做处理
		return;
	}

	CPoint pt = GetCurrentMessage()->pt;
	m_deviceTree.ScreenToClient(&pt);
	UINT uFlags = 0;
	HTREEITEM hItem = m_deviceTree.HitTest(pt, &uFlags);
	if ((NULL != hItem) && (TVHT_ONITEM & uFlags))
	{
		m_deviceTree.SelectItem(hItem);
		DWORD_PTR dataPtr = m_deviceTree.GetItemData(hItem);
		DeviceTreeNodeData* pData = (DeviceTreeNodeData*)dataPtr;
		if (pData)
		{
			//设备在线
			if (("2" == pData->getNodeType()) && ("1" == pData->getDevOnlineStatus()))
			{
				StartPlay(pData->getNodeId(), pData->getNodeName());
			}
		}
		else
		{
			int i = 1;
			i = 0;
		}
	}

}

void CMFC_DIALOG_2Dlg::StartPlay(CString strCameraID, CString strCameraName)
{
	//码流类型1-主码流 2-子码流
	int iCurtStreamType = m_streamTypeComboBox.GetCurSel() + 1;

	//* pDialogTypeComboBox = (CComboBox*)GetDlgItem();
	bool bRecordChecked = false;
	CButton* pCheckRecordCheckBtn = (CButton*)GetDlgItem(IDC_CHECK_RECORD);
	if (pCheckRecordCheckBtn)
	{
		bRecordChecked = (1 == pCheckRecordCheckBtn->GetCheck()) ? true : false;
	}

	//判断当前窗口类型
	if (bRecordChecked)
	{

	}
	else
	{
		m_pSplitScreenWindow->StartPlayLive(strCameraID, iCurtStreamType, strCameraName);
	}
}

void CMFC_DIALOG_2Dlg::OnDialogTypeChecked()
{
	if (1 == m_dialogTypeCheckBtn.GetCheck())
	{
		g_bDialogTypeChecked = true;
		//切换到录像回放模式

		//1-关闭所有窗口
		m_pSplitScreenWindow->StopAllVideo(false);
		//2-切换到单屏
		m_splitWndSelectComboBox.SetCurSel(0);
		//显示单屏幕
		m_pSplitScreenWindow->SetSplitType(1);
	}
	else
	{
		g_bDialogTypeChecked = false;
		//停止论巡定时器
		if (m_bIsCycling) {
			OnBnClickedButtonCycle();
		}
		else {
			m_pSplitScreenWindow->StopAllVideo(true);
		}
	}

	InitClientView();
}

void CMFC_DIALOG_2Dlg::OnRecordSearchBtnClicked()
{
	HTREEITEM pCurtItem = m_deviceTree.GetSelectedItem();
	if (NULL == pCurtItem)
	{
		AfxMessageBox(L"未选择设备项-1");
		return;
	}

	DWORD_PTR dataPtr = m_deviceTree.GetItemData(pCurtItem);
	DeviceTreeNodeData* pData = (DeviceTreeNodeData*)dataPtr;
	if (NULL == pData || pData->getNodeType() != "2")
	{
		AfxMessageBox(L"未选择设备项-2");
		return;
	}

	//设备ID
	CString strDevID = pData->getNodeId();

	//开始时间
	COleDateTime beginDateTime;

	//结束时间
	COleDateTime endDateTime;
	m_recordBeginTime.GetTime(beginDateTime);
	m_recordEndTime.GetTime(endDateTime);

	if (beginDateTime >= endDateTime)
	{
		AfxMessageBox(L"时间选择异常");
		return;
	}

	CString strBeginTime = beginDateTime.Format(L"%Y-%m-%d %H:%M:%S");
	CString strEndTime = endDateTime.Format(L"%Y-%m-%d %H:%M:%S");

	//录像平台类型
	int iPlatType = m_recordTypeComboBox.GetCurSel();
	CString strPlatType;
	if (0 == iPlatType)
	{
		strPlatType = "DEVICE";
	}
	else
	{
		strPlatType = "PLATFORM";
	}

	//调用查询接口
	CInternetSession session;
	//设置超时和连接重试次数
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 10000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 3);
	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 5000);
	int iPort = _ttoi(g_strLoginedPort);
	CHttpConnection* pConnection = session.GetHttpConnection(g_strLoginedIP, (INTERNET_PORT)iPort, g_strLoginedAccount, g_strLoginedPassword);
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_GET_RECORD_LIST, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	//CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc";
	//CHAR* strFormData = "{\"type\":2,\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\"}";
	//pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));
	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc";
	CString strRequestJson;
	strRequestJson.Format(_T("{\"cameraList\":[{\"cameraId\":\"%s\"}],\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"pageInfo\":{\"pageNum\":1,\"pageSize\":5000},\"searchInfo\":{\"beginTime\":\"%s\",\"bookMarkName\":\"\",\"bookMarkSearchTag\":0,\"endTime\":\"%s\",\"eventList\":[{\"event\":\"ALL\"}],\"from\":\"%s\"}}"), strDevID, strBeginTime, strEndTime, strPlatType);

	USES_CONVERSION;
	CHAR* strFormData = T2A(strRequestJson);
	//CString strAuth;
	//strAuth.Format(_T("Authorization: \r\n"));
	//pFile->AddRequestHeaders(strAuth);
	//pFile->AddRequestHeaders(_T("Accept-Language: zh-CN,en,*\r\n"));
	//pFile->AddRequestHeaders(_T("Accept-Encoding: gzip, deflate\r\n"));
	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));


	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);
	if (dwRet == HTTP_STATUS_OK)
	{
		//读取http response数据
		int len2 = pFile->GetLength();
		char buf[2000];
		memset(buf, 0, 2000);
		int numread = 0;
		CString strJsonContent = L"";
		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
		{
			buf[numread] = '\0';
			strJsonContent += UTF8ToUnicode(buf);
		}
		//AfxMessageBox(strJsonContent);

		const int iLength = strJsonContent.GetLength();

		int len = WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), NULL, 0, NULL, NULL);
		char* pFileName = new char[len + 1];   //以字节为单位
		memset(pFileName, 0, len);
		WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), pFileName, len, NULL, NULL);


		cJSON* root = cJSON_Parse(pFileName);
		if (!root) {
			AfxMessageBox(L"查询录像列表失败-1");
			session.Close();
			pFile->Close();
			delete pFile;
			return;
		}

		cJSON* jResToken = cJSON_GetObjectItem(root, "resultCode");
		if (!jResToken || jResToken->valueint != 0)
		{
			AfxMessageBox(L"查询录像列表失败-2");
			if (root) {
				cJSON_Delete(root);
			}
			session.Close();
			pFile->Close();
			delete pFile;
			return;
		}

		//解析录像列表
		cJSON* pPageInfo = cJSON_GetObjectItem(root, "pageInfo");
		if (!pPageInfo || cJSON_GetObjectItem(pPageInfo, ""))
		{
			AfxMessageBox(L"获取录像列表失败-3");
			if (root) {
				cJSON_Delete(root);
			}
			session.Close();
			pFile->Close();
			delete pFile;
			return;
		}

		cJSON* pRecordList = cJSON_GetObjectItem(root, "recordList");
		if (!pRecordList || cJSON_GetObjectItem(pRecordList, ""))
		{
			AfxMessageBox(L"获取录像列表失败-4");
			if (root) {
				cJSON_Delete(root);
			}
			session.Close();
			pFile->Close();
			delete pFile;
			return;
		}

		int iRecordListSize = cJSON_GetArraySize(pRecordList);
		m_recordSubsectionList.clear();
		for (int iIndex = 0; iIndex < iRecordListSize; ++iIndex)
		{
			cJSON* pRecordJson = cJSON_GetArrayItem(pRecordList, iIndex);
			if (pRecordJson)
			{
				RecordNodeData recordData;
				//解析节点数据
				if (cJSON_GetObjectItem(pRecordJson, "cameraId") && cJSON_GetObjectItem(pRecordJson, "cameraId")->valuestring)
				{
					recordData.setCameraId(CString(cJSON_GetObjectItem(pRecordJson, "cameraId")->valuestring));
				}

				if (cJSON_GetObjectItem(pRecordJson, "beginTime") && cJSON_GetObjectItem(pRecordJson, "beginTime")->valuestring)
				{
					recordData.setBeginTime(CString(cJSON_GetObjectItem(pRecordJson, "beginTime")->valuestring));
				}

				if (cJSON_GetObjectItem(pRecordJson, "endTime") && cJSON_GetObjectItem(pRecordJson, "endTime")->valuestring)
				{
					recordData.setEndTime(CString(cJSON_GetObjectItem(pRecordJson, "endTime")->valuestring));
				}

				if (cJSON_GetObjectItem(pRecordJson, "cameraName") && cJSON_GetObjectItem(pRecordJson, "cameraName")->valuestring)
				{
					recordData.setCameraName(CString(cJSON_GetObjectItem(pRecordJson, "cameraName")->valuestring));
				}

				if (cJSON_GetObjectItem(pRecordJson, "contentId") && cJSON_GetObjectItem(pRecordJson, "contentId")->valuestring)
				{
					recordData.setContentId(CString(cJSON_GetObjectItem(pRecordJson, "contentId")->valuestring));
				}

				if (cJSON_GetObjectItem(pRecordJson, "event") && cJSON_GetObjectItem(pRecordJson, "event")->valuestring)
				{
					recordData.setEvent(CString(cJSON_GetObjectItem(pRecordJson, "event")->valuestring));
				}

				if (cJSON_GetObjectItem(pRecordJson, "location"))
				{
					recordData.setLocation(cJSON_GetObjectItem(pRecordJson, "location")->valueint);
				}

				if (cJSON_GetObjectItem(pRecordJson, "nvrCode") && cJSON_GetObjectItem(pRecordJson, "nvrCode")->valuestring)
				{
					recordData.setNvrCode(CString(cJSON_GetObjectItem(pRecordJson, "nvrCode")->valuestring));
				}

				m_recordSubsectionList.push_back(recordData);
			}
		}
		if (root) {
			cJSON_Delete(root);
		}

		LoadRecordInfoList();
	}
	else if (HTTP_STATUS_DENIED == dwRet)
	{
		//AfxMessageBox(L"查询录像列表失败");
		//重新查询
		pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));
		DWORD dwRet2;
		pFile->QueryInfoStatusCode(dwRet2);
		if (dwRet2 == HTTP_STATUS_OK)
		{
			//读取http response数据
			int len2 = pFile->GetLength();
			char buf[2000];
			int numread;
			CString strJsonContent = L"";

			while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
			{
				buf[numread] = '\0';
				strJsonContent += UTF8ToUnicode(buf);
			}

			int len = WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), NULL, 0, NULL, NULL);
			char* pFileName = new char[len + 1];   //以字节为单位
			memset(pFileName, 0, len);
			WideCharToMultiByte(CP_ACP, 0, strJsonContent, strJsonContent.GetLength(), pFileName, len, NULL, NULL);


			const int iLength = strJsonContent.GetLength();
			LPSTR lpszTest = T2A(strJsonContent);
			cJSON* root = cJSON_Parse(lpszTest);
			if (!root) {
				AfxMessageBox(L"查询录像列表失败-5");
				session.Close();
				pFile->Close();
				delete pFile;
				return;
			}

			cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
			if (!jResCode || jResCode->valueint != 0)
			{
				AfxMessageBox(L"查询录像列表失败-6");
				if (root) {
					cJSON_Delete(root);
				}
				session.Close();
				pFile->Close();
				delete pFile;
				return;
			}

			//解析录像列表
			cJSON* pPageInfo = cJSON_GetObjectItem(root, "pageInfo");
			if (!pPageInfo || cJSON_GetObjectItem(pPageInfo, ""))
			{
				AfxMessageBox(L"获取录像列表失败-7");
				if (root) {
					cJSON_Delete(root);
				}
				session.Close();
				pFile->Close();
				delete pFile;
				return;
			}

			cJSON* pRecordList = cJSON_GetObjectItem(root, "recordList");
			if (!pRecordList || cJSON_GetObjectItem(pRecordList, ""))
			{
				AfxMessageBox(L"获取录像列表失败-8");
				if (root) {
					cJSON_Delete(root);
				}
				session.Close();
				pFile->Close();
				delete pFile;
				return;
			}

			int iRecordListSize = cJSON_GetArraySize(pRecordList);
			m_recordSubsectionList.clear();

			for (int iIndex = 0; iIndex < iRecordListSize; ++iIndex)
			{
				cJSON* pRecordJson = cJSON_GetArrayItem(pRecordList, iIndex);
				if (pRecordJson)
				{
					RecordNodeData recordData;
					//解析节点数据
					if (cJSON_GetObjectItem(pRecordJson, "cameraId") && cJSON_GetObjectItem(pRecordJson, "cameraId")->valuestring)
					{
						recordData.setCameraId(CString(cJSON_GetObjectItem(pRecordJson, "cameraId")->valuestring));
					}

					if (cJSON_GetObjectItem(pRecordJson, "beginTime") && cJSON_GetObjectItem(pRecordJson, "beginTime")->valuestring)
					{
						recordData.setBeginTime(CString(cJSON_GetObjectItem(pRecordJson, "beginTime")->valuestring));
					}

					if (cJSON_GetObjectItem(pRecordJson, "endTime") && cJSON_GetObjectItem(pRecordJson, "endTime")->valuestring)
					{
						recordData.setEndTime(CString(cJSON_GetObjectItem(pRecordJson, "endTime")->valuestring));
					}

					if (cJSON_GetObjectItem(pRecordJson, "cameraName") && cJSON_GetObjectItem(pRecordJson, "cameraName")->valuestring)
					{
						recordData.setCameraName(CString(cJSON_GetObjectItem(pRecordJson, "cameraName")->valuestring));
					}

					if (cJSON_GetObjectItem(pRecordJson, "contentId") && cJSON_GetObjectItem(pRecordJson, "contentId")->valuestring)
					{
						recordData.setContentId(CString(cJSON_GetObjectItem(pRecordJson, "contentId")->valuestring));
					}

					if (cJSON_GetObjectItem(pRecordJson, "event") && cJSON_GetObjectItem(pRecordJson, "event")->valuestring)
					{
						recordData.setEvent(CString(cJSON_GetObjectItem(pRecordJson, "event")->valuestring));
					}

					if (cJSON_GetObjectItem(pRecordJson, "location"))
					{
						recordData.setLocation(cJSON_GetObjectItem(pRecordJson, "location")->valueint);
					}

					if (cJSON_GetObjectItem(pRecordJson, "nvrCode") && cJSON_GetObjectItem(pRecordJson, "nvrCode")->valuestring)
					{
						recordData.setNvrCode(CString(cJSON_GetObjectItem(pRecordJson, "nvrCode")->valuestring));
					}
					m_recordSubsectionList.push_back(recordData);
				}
			}
			if (root) {
				cJSON_Delete(root);
			}
			LoadRecordInfoList();
		}
		else
		{
			AfxMessageBox(L"查询录像列表失败-9");
		}
	}

	session.Close();
	pFile->Close();
	delete pFile;
}

//加载录像列表
void CMFC_DIALOG_2Dlg::LoadRecordInfoList()
{
	//清理录像列表
	m_recordList.DeleteAllItems();

	if (m_recordSubsectionList.size() <= 0)
	{
		AfxMessageBox(L"录像列表为空");
		return;
	}

	int i = 0;
	for each (RecordNodeData data in m_recordSubsectionList)
	{
		CString strBeginTime = data.getBeginTime();
		strBeginTime.Replace('-', '/');
		//strBeginTime.Remove('-');
		//strBeginTime.Remove(':');
		//strBeginTime.Remove(' ');

		CString strEndTime = data.getEndTime();
		//strEndTime.Remove('-');
		//strEndTime.Remove(':');
		//strEndTime.Remove(' ');
		strEndTime.Replace('-', '/');

		CString strOrder;
		strOrder.Format(_T("%d"), i + 1);

		m_recordList.InsertItem(i, strOrder);
		m_recordList.SetItemText(i, 1, strBeginTime.Mid(5));
		m_recordList.SetItemText(i++, 2, strEndTime.Mid(5));
	}
}

void CMFC_DIALOG_2Dlg::OnBnClickedButtonSelectRecordSubsection()
{
	// TODO:  在此添加控件通知处理程序代码
	int iPos = m_recordList.GetNextItem(-1, LVIS_SELECTED);
	if (iPos < 0)
	{
		AfxMessageBox(L"未选择录像分段");
		return;
	}

	if (iPos >= m_recordSubsectionList.size())
	{
		AfxMessageBox(L"数据异常");
		return;
	}

	RecordNodeData recordData = m_recordSubsectionList[iPos];
	g_curtRecordSubsection = recordData;

	SetDlgItemTextW(IDC_STATIC_RECORD_DEVICE_NAME, recordData.getCameraName());
	SetDlgItemTextW(IDC_STATIC_RECORD_BEGIN_TIME, recordData.getBeginTime());
	SetDlgItemTextW(IDC_STATIC_RECORD_END_TIME, recordData.getEndTime());

	COleVariant vBegintime(recordData.getBeginTime());
	vBegintime.ChangeType(VT_DATE);
	//COleDateTime beginDateTime = vBegintime;
	m_curtSubsectionBeginTime = vBegintime;

	COleVariant vEndtime(recordData.getEndTime());
	vEndtime.ChangeType(VT_DATE);
	//COleDateTime endDateTime = vEndtime;
	m_curtSubsectionEndTime = vEndtime;

	SetDlgItemTextW(IDC_STATIC_PLAY_TIME, recordData.getBeginTime());

	//取时间差
	//COleDateTime leadTime = endDateTime - beginDateTime;

	COleDateTimeSpan leadTimeSpan = m_curtSubsectionEndTime - m_curtSubsectionBeginTime;

	double totalSeconds = leadTimeSpan.GetTotalSeconds();

	//滑块范围调整
	m_recordPlaySlider.SetRange(0, totalSeconds);
	m_recordPlaySlider.SetTicFreq(totalSeconds / 100);
	m_recordPlaySlider.SetLineSize(1);
	m_recordPlaySlider.SetPageSize(1);
}

void CMFC_DIALOG_2Dlg::OnRecordScaleSelectChange()
{
	int iCurtType = m_recordScaleComboBox.GetCurSel();
	if (iCurtType < 0 || iCurtType > 4)
	{
		return;
	}
	//刷新当前倍速
	int iType = iCurtType - 2;
	g_dRecordScale = pow(2, iType);

	/*if (-1 != m_strCurtRecordCameraID)
	{
		return;
	}*/

	//获取当前时间点
	//m_pSplitScreenWindow->RecordVCRControl();
	int curtPlayedSeconds = -1.0;//m_recordPlaySlider.GetPos();
	SYSTEMTIME sysTime;
	m_curtSubsectionBeginTime.GetAsSystemTime(sysTime);
	CTime curtTime(sysTime);
	curtTime += curtPlayedSeconds;
	//SetDlgItemTextW(IDC_STATIC_PLAY_TIME, curtTime.Format(L"%Y-%m-%d %H:%M:%S"));

	//调用录像vcrcontrol方法
	//取pos 取scale

	m_pSplitScreenWindow->RecordVCRControl(curtPlayedSeconds);
}

void CMFC_DIALOG_2Dlg::OnNMReleasedcaptureSliderRecord(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;

	int curtPlayedSeconds = m_recordPlaySlider.GetPos();
	SYSTEMTIME sysTime;
	m_curtSubsectionBeginTime.GetAsSystemTime(sysTime);
	CTime curtTime(sysTime);
	curtTime += curtPlayedSeconds;
	//SetDlgItemTextW(IDC_STATIC_PLAY_TIME, curtTime.Format(L"%Y-%m-%d %H:%M:%S"));

	//调用录像vcrcontrol方法
	//取pos 取scale

	m_pSplitScreenWindow->RecordVCRControl(curtPlayedSeconds);
}

void CMFC_DIALOG_2Dlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
	int curtPlayedSeconds = m_recordPlaySlider.GetPos();
	SYSTEMTIME sysTime;
	m_curtSubsectionBeginTime.GetAsSystemTime(sysTime);
	CTime curtTime(sysTime);
	curtTime += curtPlayedSeconds;
	SetDlgItemTextW(IDC_STATIC_PLAY_TIME, curtTime.Format(L"%Y-%m-%d %H:%M:%S"));
}

//void CMFC_DIALOG_2Dlg::OnTRBNThumbPosChangingSliderRecord(NMHDR *pNMHDR, LRESULT *pResult)
//{
//	// 此功能要求 Windows Vista 或更高版本。
//	// _WIN32_WINNT 符号必须 >= 0x0600。
//	NMTRBTHUMBPOSCHANGING *pNMTPC = reinterpret_cast<NMTRBTHUMBPOSCHANGING *>(pNMHDR);
//	// TODO:  在此添加控件通知处理程序代码
//	*pResult = 0;
//
//	if (NULL != pNMTPC)
//	{
//		CString str;
//		str.Format(_T("%d"), pNMTPC->dwPos);
//		SetDlgItemTextW(IDC_STATIC_PLAY_TIME, str);
//	}
//}

LRESULT CMFC_DIALOG_2Dlg::OnPlayDataMsg(WPARAM wParam, LPARAM lParam)
{
	//计算时间并刷新界面
	//if (BUSINESS_TYPE_NETRECORD_START == (ACS_BUSSINES_TYPE)lBusinessType)
	//{
	//	m_recordPlaySlider.SetPos(lCurtTime);
	//}

	//std::string strParams = (std::string)wParam;
	BSTR b = (BSTR)wParam;
	CString s(b);
	SysFreeString(b);
	//AfxMessageBox(s);

	//CString类型分离子字符串
	CString strSrc = s;
	CStringArray strResult;

	CString strGap = _T("/");
	int nPos = strSrc.Find(strGap);

	CString strLeft = _T("");
	while (0 <= nPos)
	{
		strLeft = strSrc.Left(nPos);
		if (!strLeft.IsEmpty())
			strResult.Add(strLeft);

		strSrc = strSrc.Right(strSrc.GetLength() - nPos - 1);
		nPos = strSrc.Find(strGap);
	}

	if (!strSrc.IsEmpty()) {
		strResult.Add(strSrc);
	}

	int nSize = strResult.GetSize();
	//for (int i = 0; i < nSize; i++)
	//{
	//	AfxMessageBox(strResult[i]);
	//}

	if (nSize < 4)
	{
		return -1;
	}

	long lBusinessId = _ttol(strResult[0]);
	long lBusinessType = _ttol(strResult[1]);
	long lCurtTime = _ttol(strResult[2]);

	if (BUSINESS_TYPE_NETRECORD_START == (ACS_BUSSINES_TYPE)lBusinessType)
	{
		if (g_bDialogTypeChecked && NULL != m_recordPlaySlider)
		{
			//printf("record slider rangeMax: %d, curtPos: %d\n", m_recordPlaySlider.GetRangeMax(), m_recordPlaySlider.GetPos());

			TRACE("record slider rangeMax: %d, curtPos: %d, newPos: %d\n", m_recordPlaySlider.GetRangeMax(), m_recordPlaySlider.GetPos(), lCurtTime);
			m_recordPlaySlider.SetPos(lCurtTime);
		}
	}

	return 0;
}

LRESULT CMFC_DIALOG_2Dlg::OnMultiPlayMsg(WPARAM wParam, LPARAM lParam)
{
	BSTR b = (BSTR)wParam;
	CString s(b);
	SysFreeString(b);
	m_multiPlayUrlEdit.SetWindowTextW(s);
	OnBnClickedButtonMultiPlay();
	return 0;
}

void CMFC_DIALOG_2Dlg::OnBnClickedButtonMediaInfo()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_pSplitScreenWindow) {
		m_pSplitScreenWindow->GetMediaInfo();
	}
}

int CMFC_DIALOG_2Dlg::getAudioTalkBackUrl(CString& strUrl)
{
	//获取实时浏览地址
	CInternetSession session;
	//设置超时和连接重试次数
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 10000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 3);
	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 5000);
	int iPort = _ttoi(g_strLoginedPort);
	CHttpConnection* pConnection = session.GetHttpConnection(g_strLoginedIP, (INTERNET_PORT)iPort, g_strLoginedAccount, g_strLoginedPassword);
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_GET_AUDIOTALK_URL, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc\nConnection: Keep-Alive";
	CString strRequestJson;
	strRequestJson.Format(_T("{\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"agentType\":1,\"urlType\":1,\"cameraId\":\"%s\"}"), m_strCurtAudioTalkCameraID);

	USES_CONVERSION;
	CHAR* strFormData = T2A(strRequestJson);
	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

	int iRes = -1;

	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);
	if (dwRet == HTTP_STATUS_OK)
	{
		//读取http response数据
		int len = pFile->GetLength();
		char buf[2000];
		int numread;
		CString strJsonContent = L"";

		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
		{
			buf[numread] = '\0';
			strJsonContent += UTF8ToUnicode(buf);
		}

		const int iLength = strJsonContent.GetLength();
		LPSTR lpszTest = T2A(strJsonContent);
		cJSON* root = cJSON_Parse(lpszTest);
		if (!root) {
			return -1;
		}

		cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
		if (!jResCode || jResCode->valueint != 0)
		{
			//AfxMessageBox(L"请求语音对讲推流地址失败-1");
			cJSON_Delete(root);
			return -1;
		}

		if (cJSON_GetObjectItem(root, "url"))
		{
			strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
			iRes = 0;
		}
		else {
			return -1;
		}
	}
	else if (HTTP_STATUS_DENIED == dwRet)
	{
		//401 重新请求
		pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

		DWORD dwRet2;
		pFile->QueryInfoStatusCode(dwRet2);
		if (dwRet2 == HTTP_STATUS_OK)
		{
			//读取http response数据
			int len = pFile->GetLength();
			char buf[2000];
			int numread;
			CString strJsonContent = L"";

			while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
			{
				buf[numread] = '\0';
				strJsonContent += UTF8ToUnicode(buf);
			}

			const int iLength = strJsonContent.GetLength();
			LPSTR lpszTest = T2A(strJsonContent);
			cJSON* root = cJSON_Parse(lpszTest);
			if (!root) {
				return -1;
			}

			cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
			if (!jResCode || jResCode->valueint != 0)
			{
				//AfxMessageBox(L"请求语音对讲推流地址失败-2");
				cJSON_Delete(root);
				return -1;
			}

			if (cJSON_GetObjectItem(root, "url"))
			{
				strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
				iRes = 0;
			}
		}
	}

	session.Close();
	pFile->Close();
	delete pFile;

	return iRes;
}

void CMFC_DIALOG_2Dlg::OnBnClickedButtonAudioTalk()
{
	CString strVoiceTalkUrl;
	m_multiPlayUrlEdit.GetWindowTextW(strVoiceTalkUrl);
	if (strVoiceTalkUrl.IsEmpty()) {
		AfxMessageBox(L"请填写语音对讲URl");
		return;
	}
	string url = CT2A(strVoiceTalkUrl.GetBuffer());
	AudioTalkTest(url);
	return;
}

void CMFC_DIALOG_2Dlg::OnBnClickedButtonRefreash()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateVerification();
}

//更新验证码
void CMFC_DIALOG_2Dlg::UpdateVerification()
{
	//获取登录基本信息
	CString strIP;
	m_ipEdit.GetWindowTextW(strIP);

	CInternetSession session;
	//设置超时和连接重试次数
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 1000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 1);
	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 1000);
	int iPort = 10000;
	CHttpConnection* pConnection = session.GetHttpConnection(strIP, (INTERNET_PORT)iPort);
	//QString url = QString("http://%1:10000/uas/v1/api/captcha?api=/api/user/login&cn=%2").arg(ui.ComboBox_IP->currentText()).arg(CommonFun::getCommonData()->getClientNonce());

	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET, L"/uas/v1/api/captcha?api=/api/user/login&cn=wpc01202104300945531486529", NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:Mozilla/5.0;charset=UTF-8\nUser-Agent:allcam-wpc";
	pFile->SendRequest(szHeaders, szHeaders.GetLength());

	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);

	CString strUrl;
	if (dwRet == HTTP_STATUS_OK)
	{
		//读取http response数据
		int len = pFile->GetLength();
		//char buf[2000];
		int numread;
		CString strJsonContent = L"";
		//schar *ch = buf;
		BYTE* buf = new BYTE[2000];           //创建数组，用来保存图像的数据
		CFile tempfile(L"temp.jpg", CFile::modeCreate | CFile::modeWrite);//创建文件temp.bmp
		CArchive ar(&tempfile, CArchive::store); //创建缓冲区
		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
		{
			//向缓冲区内写数据，buf为保存图像数据的数组，dwDataLen
			buf[numread] = '\0';
			ar.Write(buf, numread);
		}
		ar.Close();                 //关闭缓冲区
		tempfile.Close();        //关闭文件

		//CWnd *p1 = (CWnd *)GetDlgItem(IDC_STATIC_VERIFICATION_SHOW);
		//p1->ModifyStyle(0xf, SS_BITMAP | SS_CENTERIMAGE);
		//CRect r1;
		//p1->GetWindowRect(&r1);
		//HBITMAP hBitmap = (HBITMAP)::LoadImage(NULL, TEXT("E:\\QtProject\\work\\Allplayer_new\\allplayer\\Prj-Win\\PC_DEMO\\temp.bmp"), IMAGE_BITMAP,240, 80, LR_LOADFROMFILE);
		//p1->SetBitmap(hBitmap);

		CImage  image;
		image.Load(L"temp.jpg");
		CRect r1;
		CWnd* pWnd = (CWnd*)GetDlgItem(IDC_STATIC_VERIFICATION_SHOW);
		CDC* pDC = pWnd->GetDC();//获取picture的DC
		pWnd->GetClientRect(&r1);//获取句柄指向控件区域的大小
		image.Draw(pDC->m_hDC, r1);//将图片绘制到picture表示的区域内
		ReleaseDC(pDC);
	}
	else
	{
	}
	session.Close();
	pFile->Close();
	pFile->Abort();
	delete pFile;
}

void CMFC_DIALOG_2Dlg::AudioTalkTest(std::string& url)
{
	static bool start = false;
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr, * crtsp = nullptr;
	int32_t businessType = BUSINESS_TYPE_AUDIO_TALK_START;
	if (!start) {
		businessType = BUSINESS_TYPE_AUDIO_TALK_START;
	}
	else {
		businessType = BUSINESS_TYPE_AUDIO_TALK_STOP;
	}
	start = !start;
	//生成的结果
	char* result;
	//调用allplayer接口，下载
	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", businessType);
	cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	_snprintf_s(cbuf, 512, "%s", url.c_str());
	crtsp = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "RtspUrl", crtsp);

	result = cJSON_Print(root);
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void CMFC_DIALOG_2Dlg::FileBroadCastTest(std::string& url)
{
	static bool start1 = false;
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr, * crtsp = nullptr;
	int32_t businessType = TYPE_FILE_BROADCASR_START;
	if (!start1) {
		businessType = TYPE_FILE_BROADCASR_START;
	}
	else {
		businessType = TYPE_FILE_BROADCASR_STOP;
	}
	start1 = !start1;
	//生成的结果
	char* result;
	//调用allplayer接口，下载
	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", businessType);
	cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	_snprintf_s(cbuf, 512, "%s", url.c_str());
	crtsp = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "RtspUrl", crtsp);

	_snprintf_s(cbuf, 256, "%s", "test.wav");
	cJSON* path = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "WavPath", path);

	_snprintf_s(cbuf, 256, "%d", 1);
	cJSON* loop = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "WavLoop", loop);

	result = cJSON_Print(root);
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void CMFC_DIALOG_2Dlg::PostUrlTest()
{
	CString strPlayUrl;
	m_multiPlayUrlEdit.GetWindowTextW(strPlayUrl);
	if (strPlayUrl.IsEmpty()) {
		AfxMessageBox(L"请输入播放URl");
		return;
	}

	static bool start_url = true;
	//调用allplayer接口，播放
	VideoPlayer *player = m_pSplitScreenWindow->getFirstPlayWindow();	
	player->UrlPlayTest(strPlayUrl, start_url);
	start_url = !start_url;
}

void CMFC_DIALOG_2Dlg::OnBnClickedButtonMultiPlay()
{
	/*PostUrlTest();
	return;
	
	CString strVoiceTalkUrl;
	m_multiPlayUrlEdit.GetWindowTextW(strVoiceTalkUrl);
	if (strVoiceTalkUrl.IsEmpty()) {
		AfxMessageBox(L"请填写语音对讲URl");
		return;
	}
	string url = CT2A(strVoiceTalkUrl.GetBuffer());
	FileBroadCastTest(url);
	return;*/

	if (-1 != m_iCurtMultiBusinessID) {		//停止拼接流播放
		cJSON* root = nullptr;
		cJSON* cbusinessType = nullptr;
		int32_t businessType = TYPE_MULTI_REALVIDEO_STOP;
		//生成的结果
		char* result;

		root = cJSON_CreateObject();
		if (!root) {
			return;
		}
		char cbuf[1024] = { 0 };
		_snprintf_s(cbuf, 96, "%d", businessType);
		cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

		result = cJSON_Print(root);
		int iRes = ap_lib_excute(m_iCurtMultiBusinessID, (char*)result);
		cJSON_Delete(root);
		free(result);
		if (0 != iRes) {
			AfxMessageBox(L"停止拼接流播放失败");
		}
		else {
			m_multiPlayBtn.SetWindowTextW(L"拼接流/ON");
			m_iCurtMultiBusinessID = -1;
		}
	}
	else  //开始播放拼接流
	{
		CString strMultiPlayUrl;
		m_multiPlayUrlEdit.GetWindowTextW(strMultiPlayUrl);
		if (strMultiPlayUrl.IsEmpty()) {
			AfxMessageBox(L"请输入拼接流播放URl");
			return;
		}

		std::vector<VideoPlayer*> playerVec = m_pSplitScreenWindow->getMultiPlayWindows();
		if (playerVec.size() <= 0) {
			AfxMessageBox(L"请选择拼接流子窗口");
			return;
		}

		cJSON* root = nullptr;
		cJSON* cbusinessType = nullptr;
		cJSON* cmultiFragsCount = nullptr;
		cJSON* cwindowsHandles = nullptr;
		cJSON* crtspurl = nullptr;

		int32_t businessType = TYPE_MULTI_REALVIDEO_START;
		int32_t multiFragsCount = playerVec.size();
		//生成的结果
		char* result;

		root = cJSON_CreateObject();
		if (!root) {
			return;
		}
		char cbuf[1024] = { 0 };
		_snprintf_s(cbuf, 96, "%d", businessType);
		cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

		_snprintf_s(cbuf, 96, "%d", multiFragsCount);
		cmultiFragsCount = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "MultiFragsCount", cmultiFragsCount);

		//调用allplayer接口，拼接流播放
		cwindowsHandles = cJSON_CreateArray();
		for (VideoPlayer* pPlayer : playerVec) {
			HWND playerHwnd = pPlayer->GetPlayerHwnd();
			_snprintf_s(cbuf, 128, "%d", playerHwnd);
			cJSON* cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
			cJSON_AddItemToArray(cwindowsHandles, cwindowsHandle);
		}
		cJSON_AddItemToObject(root, "MultiHWNDs", cwindowsHandles);

		string url = CT2A(strMultiPlayUrl.GetBuffer());
		_snprintf_s(cbuf, 1000, "%s", url.c_str());
		crtspurl = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

		result = cJSON_Print(root);
		AfxMessageBox(CString(result));
		m_iCurtMultiBusinessID = g_iBusinessID++;
		int iRes = ap_lib_excute(m_iCurtMultiBusinessID, (char*)result);
		cJSON_Delete(root);
		free(result);
		if (0 != iRes) {
			AfxMessageBox(L"拼接流播放失败");
		}
		else {
			m_multiPlayBtn.SetWindowTextW(L"拼接流/OFF");
		}
	}
}

void CMFC_DIALOG_2Dlg::OnBnClickedButtonCycle()
{
	m_bIsCycling = !m_bIsCycling;
	if (m_bIsCycling) {
		m_cycleBtn.SetWindowTextW(L"轮巡/OFF");
		//启动论巡
		m_cycleTimer.start(10000, std::bind(&CSplitScreenWindow::switchUrlInCycling, m_pSplitScreenWindow));
	}
	else 
	{
		m_cycleBtn.SetWindowTextW(L"轮巡");
		m_cycleTimer.stop();
	}
}
