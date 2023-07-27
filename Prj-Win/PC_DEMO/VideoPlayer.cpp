// VideoPlayer.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC_DIALOG_2.h"
#include "VideoPlayer.h"
#include "afxdialogex.h"
#include "allplayer.h"
#include "AScJSON.h"
#include "RecordNodeInfo.h"
#include <afxinet.h>
#include <afx.h>

#include <iostream>
#include <string>
using namespace std;


#define MULTI_PLAY
#define URL_GET_REALTIME_LIVE  L"/uas/v1/api/media/live"
#define URL_GET_RECORD_LIVE  L"/uas/v1/api/record/url"

//全局变量 记录已登录账号信息
extern CString g_strLoginedAccount;
extern CString g_strLoginedPassword;
extern CString g_strLoginedIP;
extern CString g_strLoginedPort;
extern CString g_strLoginedToken;
extern BOOL g_bDialogTypeChecked;
extern RecordNodeData g_curtRecordSubsection;
extern long g_iBusinessID;
extern double g_dRecordScale;

//#pragma comment(lib,"libAllplayer.lib")

// VideoPlayer 对话框

IMPLEMENT_DYNAMIC(VideoPlayer, CDialogEx)

VideoPlayer::VideoPlayer(int iIndex, CWnd* pParent /*=NULL*/)
	: CDialogEx(VideoPlayer::IDD, pParent)
{
	m_bIsSelected = false;
	m_screenBKBrush = CreateSolidBrush(RGB(240, 255, 255));
	m_borderBKBrush = CreateSolidBrush(RGB(255, 0, 0));
	m_pParent = (CSplitScreenWindow*)pParent;
	m_iIndex = iIndex;
	m_iCurtBusinessID = -1;
	m_strCameraID = L"";
	m_iStreamType = -1;
	m_strCameraName = L"";
	m_bLivePlaying = false;
	m_bSetUrl = false;
}

VideoPlayer::~VideoPlayer()
{
}

void VideoPlayer::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_SCREEN, m_playScreen);
	DDX_Control(pDX, IDC_BUTTON_PLAY, m_playBtn);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_stopBtn);
	DDX_Control(pDX, IDC_BUTTON_CAPTURE, m_captureBtn);
	DDX_Control(pDX, IDC_BUTTON_RECORD, m_localRecordBtn);
	DDX_Control(pDX, IDC_SLIDER_VOLUME, m_volumeSlider);

	DDX_Control(pDX, IDC_STATIC_TOP_BORDER, m_topBorder);
	DDX_Control(pDX, IDC_STATIC_BOTTOM_BORDER, m_bottomBorder);
	DDX_Control(pDX, IDC_STATIC_LEFT_BORDER, m_leftBorder);
	DDX_Control(pDX, IDC_STATIC_RIGHT_BORDER, m_rightBorder);

	DDX_Control(pDX, IDC_STATIC_ZOOM, m_zoomLab);
	DDX_Control(pDX, IDC_EDIT_ZOOM_TOP, m_zoomTopEdit);
	DDX_Control(pDX, IDC_EDIT_ZOOM_RIGHT, m_zoomRightEdit);
	DDX_Control(pDX, IDC_EDIT_ZOOM_BOTTOM, m_zoomBottomEdit);
	DDX_Control(pDX, IDC_EDIT_ZOOM_LEFT, m_zoomLeftEdit);

	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_pauseBtn);
	DDX_Control(pDX, IDC_BUTTON_CONTINUE, m_continueBtn);
	DDX_Control(pDX, IDC_BUTTON_ZOOM_START, m_zoomStartBtn);
	DDX_Control(pDX, IDC_BUTTON_ZOOM_UPDATE, m_zoomUpdateBtn);
	DDX_Control(pDX, IDC_BUTTON_ZOOM_END, m_zoomEndBtn);
	DDX_Control(pDX, IDC_BUTTON_FORWARD, m_forwardBtn);
	DDX_Control(pDX, IDC_BUTTON_BACKWARD, m_backwardBtn);
	DDX_Control(pDX, IDC_BUTTON_EXIT, m_stepExitBtn);

	DDX_Control(pDX, IDC_STATIC_ZOOM_PLAYER, m_zoomPlayerLab);
	DDX_Control(pDX, IDC_CHECK_MULTI_PLAY, m_multiPlayCheckBtn);
	m_zoomTopEdit.SetWindowTextW(L"10");
	m_zoomBottomEdit.SetWindowTextW(L"60");
	m_zoomRightEdit.SetWindowTextW(L"90");
	m_zoomLeftEdit.SetWindowTextW(L"10");
}

BEGIN_MESSAGE_MAP(VideoPlayer, CDialogEx)
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &VideoPlayer::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &VideoPlayer::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_CAPTURE, &VideoPlayer::OnBnClickedButtonCapture)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &VideoPlayer::OnBnClickedButtonRecord)
	ON_STN_CLICKED(IDC_STATIC_SCREEN, &VideoPlayer::OnStnClickedStaticScreen)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &VideoPlayer::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_CONTINUE, &VideoPlayer::OnBnClickedButtonContinue)
	ON_BN_CLICKED(IDC_BUTTON_ZOOM_START, &VideoPlayer::OnBnClickedButtonZoomStart)
	ON_BN_CLICKED(IDC_BUTTON_ZOOM_UPDATE, &VideoPlayer::OnBnClickedButtonZoomUpdate)
	ON_BN_CLICKED(IDC_BUTTON_ZOOM_END, &VideoPlayer::OnBnClickedButtonZoomEnd)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_VOLUME, &VideoPlayer::OnNMCustomdrawSliderVolume)
	ON_BN_CLICKED(IDC_BUTTON_FORWARD, &VideoPlayer::OnBnClickedButtonForward)
	ON_BN_CLICKED(IDC_BUTTON_BACKWARD, &VideoPlayer::OnBnClickedButtonBackward)
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &VideoPlayer::OnBnClickedButtonExit)
	//ON_BN_CLICKED(IDC_CHECK_MULTI_PLAY, &VideoPlayer::OnBnClickedCheckMultiPlay)
END_MESSAGE_MAP()

void VideoPlayer::InitPlayerView()
{
	CRect rect;
	GetClientRect(&rect);
	int iPlayerWidth = rect.Width() - PLAYER_BORDER_WIDTH * 2;
	int iPlayerHeight = rect.Height() - PLAYER_BORDER_WIDTH * 2;

	//m_volumeSlider.SetRange(0, 100);  //滑动条默认范围0-100
	m_volumeSlider.SetPos(50);
	if (m_DigitalShow) {
		m_playScreen.MoveWindow(PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH, iPlayerWidth / 2, (iPlayerHeight - PLAYER_ACTION_BAR_HEIGHT) / 2);
	}
	else {
		m_playScreen.MoveWindow(PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH, iPlayerWidth, iPlayerHeight - PLAYER_ACTION_BAR_HEIGHT);
	}


	m_playBtn.MoveWindow(PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 30, 25);
	m_stopBtn.MoveWindow(30 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 30, 25);
	m_captureBtn.MoveWindow(60 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 45, 25);
	m_localRecordBtn.MoveWindow(105 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 45, 25);
	m_pauseBtn.MoveWindow(150 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 40, 25);
	m_continueBtn.MoveWindow(190 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 55, 25);
	m_volumeSlider.MoveWindow(245 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 55, 25);
	m_zoomLab.MoveWindow(300 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 35, 50, 25);
	m_zoomTopEdit.MoveWindow(350 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 25, 25);
	m_zoomRightEdit.MoveWindow(375 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 25, 25);
	m_zoomBottomEdit.MoveWindow(400 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 25, 25);
	m_zoomLeftEdit.MoveWindow(425 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 25, 25);
	m_zoomStartBtn.MoveWindow(450 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 30, 25);
	m_zoomUpdateBtn.MoveWindow(480 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 35, 25);
	m_zoomEndBtn.MoveWindow(515 + PLAYER_BORDER_WIDTH, PLAYER_BORDER_WIDTH + iPlayerHeight - 30, 30, 25);
	if (m_DigitalShow) {
		m_zoomPlayerLab.MoveWindow(PLAYER_BORDER_WIDTH + iPlayerWidth / 2, PLAYER_BORDER_WIDTH + (iPlayerHeight - PLAYER_ACTION_BAR_HEIGHT) / 2, iPlayerWidth / 2, (iPlayerHeight - PLAYER_ACTION_BAR_HEIGHT) / 2);
	}
	else {
		m_zoomPlayerLab.ShowWindow(SW_HIDE);
	}

	if (m_bIsSelected) {
		m_leftBorder.MoveWindow(0, 0, PLAYER_BORDER_WIDTH, rect.Height());
		m_rightBorder.MoveWindow(rect.Width() - PLAYER_BORDER_WIDTH, 0, PLAYER_BORDER_WIDTH, rect.Height());
		m_topBorder.MoveWindow(0, 0, rect.Width(), PLAYER_BORDER_WIDTH);
		m_bottomBorder.MoveWindow(0, rect.Height() - PLAYER_BORDER_WIDTH, rect.Width(), PLAYER_BORDER_WIDTH);

		m_leftBorder.ShowWindow(SW_SHOW);
		m_rightBorder.ShowWindow(SW_SHOW);
		m_topBorder.ShowWindow(SW_SHOW);
		m_bottomBorder.ShowWindow(SW_SHOW);
	}
	else {
		m_leftBorder.ShowWindow(SW_HIDE);
		m_rightBorder.ShowWindow(SW_HIDE);
		m_topBorder.ShowWindow(SW_HIDE);
		m_bottomBorder.ShowWindow(SW_HIDE);
	}
	m_multiPlayCheckBtn.MoveWindow(0, PLAYER_BORDER_WIDTH + iPlayerHeight - 52, 65, 25);
}

HBRUSH VideoPlayer::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH resBrush = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_SCREEN) {
		//pDC->SetTextColor(RGB(0, 0, 0));//改变文本 
		//pDC->SetBkColor(RGB(255, 0, 0));//改变背景 
		pDC->SetBkMode(TRANSPARENT);
		return m_screenBKBrush;
	}
	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_TOP_BORDER || pWnd->GetDlgCtrlID() == IDC_STATIC_BOTTOM_BORDER || pWnd->GetDlgCtrlID() == IDC_STATIC_LEFT_BORDER || pWnd->GetDlgCtrlID() == IDC_STATIC_RIGHT_BORDER)
	{
		if (m_bIsSelected) {
			pDC->SetBkMode(TRANSPARENT);
			return m_borderBKBrush;
		}
	}

	return resBrush;
}

void VideoPlayer::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
}

// VideoPlayer 消息处理程序
void VideoPlayer::OnBnClickedButtonPlay()
{
	static bool sock_init = false;
	if (!sock_init) {
		//testSocks5();
		sock_init = true;
	}

	if (g_bDialogTypeChecked)  {	//录像回放
		StartPlayRecord(12);
	}
	else  {							//实时浏览
		cJSON* root = nullptr;
		cJSON* cbusinessType = nullptr;
		cJSON* cwindowsHandle = nullptr;
		cJSON* curl = nullptr, *volume = nullptr;

		string url = "rtsp://admin:Allcam@2020@172.16.20.250/Streaming/Channels/101";
		//string url = "D:/limit.avs";

		/*	
		CString strTop;
		m_zoomTopEdit.GetWindowTextW(strTop);
		string url = CT2A(strTop.GetBuffer());
		*/
		//string url = "rtsp://139.224.212.51/live/test";
		string set_url = "rtsp://admin:Allcam@2020@172.16.20.250/Streaming/Channels/101";
		int32_t businessType = BUSINESS_TYPE_REALVIDEO_START; // BUSINESS_TYPE_URL_START;
		//生成的结果
		char* result;

		root = cJSON_CreateObject();
		if (!root) {
			return;
		}
		char cbuf[1024] = { 0 };

		if (!m_bSetUrl) {
			cbusinessType = cJSON_CreateString(std::to_string(businessType).data());
			cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

			//调用allplayer接口，播放
			CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
			if (pPlayerFrame) {
				HWND playerHwnd = pPlayerFrame->m_hWnd;
				_snprintf_s(cbuf, 128, "%d", playerHwnd);
				cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

				curl = cJSON_CreateString(url.data());
				if (BUSINESS_TYPE_URL_START <= businessType) {
					cJSON_AddItemToObject(root, "Url", curl);
					if (url.find(".avs") != string::npos) {
						cJSON* jFormat = cJSON_CreateNumber(1);
						cJSON_AddItemToObject(root, "UrlFormat", jFormat);
					}
				}
				else {
					cJSON_AddItemToObject(root, "RtspUrl", curl);
				}
				
				volume = cJSON_CreateString("10");
				cJSON_AddItemToObject(root, "VolumeValue", volume);

				cJSON* cdecodeType = cJSON_CreateString("0");
				cJSON_AddItemToObject(root, "DecodeType", cdecodeType);

				result = cJSON_Print(root);
				m_iCurtBusinessID = g_iBusinessID++;

				char* out = nullptr;
				int len = 0;
				if (BUSINESS_TYPE_URL_START == businessType) {
					len = 1024;
					out = new char[len];
				}
				ap_lib_excute(m_iCurtBusinessID, (char*)result, out, len);
				free(result);
				if (out) {
					delete[] out;
				}
			}
			m_bSetUrl = true;
		}
		else {
			businessType = BUSINESS_TYPE_SET_URL;
			cbusinessType = cJSON_CreateString(std::to_string(businessType).data());
			cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

			curl = cJSON_CreateString(set_url.data());
			if (BUSINESS_TYPE_URL_START >= businessType) {
				cJSON_AddItemToObject(root, "Url", curl);
			}
			else {
				cJSON_AddItemToObject(root, "RtspUrl", curl);
			}

			result = cJSON_Print(root);
			ap_lib_excute(m_iCurtBusinessID, (char*)result);
			free(result);
		}
		
		cJSON_Delete(root);
	}
	m_bLivePlaying = true;
}


void VideoPlayer::OnBnClickedButtonStop()
{
	// TODO:  在此添加控件通知处理程序代码
	if (-1 == m_iCurtBusinessID) {
		return;
	}

	//NetRecordDownloadStopTest();

	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;

	int32_t businessType = BUSINESS_TYPE_URL_STOP;
	if (g_bDialogTypeChecked) {
		businessType = BUSINESS_TYPE_NETRECORD_STOP;
	}
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
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	m_iCurtBusinessID = -1;

	//m_playScreen.ShowWindow(SW_SHOW);
	cJSON_Delete(root);
	free(result);
	m_bLivePlaying = false;
	m_bSetUrl = false;

	//cancelSocks5();
}


void VideoPlayer::OnBnClickedButtonCapture()
{
	// TODO:  在此添加控件通知处理程序代码
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr, * snapPathJson = nullptr, * snapFormatJson = nullptr, * captureCount = nullptr;

	//int32_t businessType = BUSINESS_TYPE_CAPTURE_FRAMES;
	////生成的结果
	//char* result;

	//root = cJSON_CreateObject();
	//char cbuf[1024] = { 0 };
	//_snprintf_s(cbuf, 96, "%d", businessType);
	//cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
	//cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	//snapPathJson = cJSON_CreateString("D:\\test\\capture");
	//cJSON_AddItemToObject(root, "CapturePath", snapPathJson);

	//_snprintf_s(cbuf, 96, "%d", 0);
	//snapFormatJson = cJSON_CreateString((char*)&cbuf[0]);
	//cJSON_AddItemToObject(root, "CaptureFormat", snapFormatJson);

	//_snprintf_s(cbuf, 96, "%d", 3);
	//captureCount = cJSON_CreateString((char*)&cbuf[0]);
	//cJSON_AddItemToObject(root, "CaptureCount", captureCount);

	//result = cJSON_Print(root);

	//int32_t businessType = BUSINESS_TYPE_URL_SEEK;
	////生成的结果
	//root = cJSON_CreateObject();
	//if (!root) {
	//	return;
	//}
	//char cbuf[1024] = { 0 };
	//_snprintf_s(cbuf, 96, "%d", businessType);
	//cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
	//cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	//static double pos = 0.6;
	//snapFormatJson = cJSON_CreateNumber(pos);
	//cJSON_AddItemToObject(root, "Pos", snapFormatJson);
	//pos = pos == 0.6 ? 0.2 : 0.6;

	root = cJSON_CreateObject();
	
	cbusinessType = cJSON_CreateString(std::to_string(BUSINESS_TYPE_SKIP_NOKEY_FRAME).data());
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	static bool skip = true;
	snapFormatJson = cJSON_CreateString(std::to_string(skip ? 1 : 0).data());
	skip = !skip;
	cJSON_AddItemToObject(root, "SkipNoKey", snapFormatJson);

	auto result = cJSON_Print(root);
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::OnBnClickedButtonRecord()
{
	auto root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	auto cbusinessType = cJSON_CreateString(std::to_string(TYPE_URL_PROBE).data());
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);
	auto curl = cJSON_CreateString("D:/limit.avs");
	cJSON_AddItemToObject(root, "Url", curl);
	auto result = cJSON_Print(root);
	char* probe_result = new char[1024];
	ap_lib_excute(m_iCurtBusinessID, (char*)result, probe_result);
	return;
	/*auto cbusinessType = cJSON_CreateString(std::to_string(TYPE_URL_SPEED).data());
	static int sp = 2;
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);
	switch (sp)
	{
	case 2:	sp = 4; break;
	case 4: sp = 1; break;
	case 1: sp = 2; break;
	default:
		break;
	}
	auto speed = cJSON_CreateNumber(sp /2.0);
	cJSON_AddItemToObject(root, "Speed", speed);
	auto result = cJSON_Print(root);
	ap_lib_excute(m_iCurtBusinessID, (char*)result, nullptr);
	free(result);
	return;*/

	if (g_bDialogTypeChecked)  {		//录像回放
		StartPlayRecord(120);
	}
	else {
		if (m_bIsLocalRecord) {
			//结束本地录像
			m_bIsLocalRecord = false;
			cJSON* root = nullptr;
			cJSON* cbusinessType = nullptr;
			int32_t businessType = BUSINESS_TYPE_LOCAL_RECORD_STOP;
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
			int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
			cJSON_Delete(root);
			free(result);
			return;
		}
		else if (-1 != m_iCurtBusinessID) {
			// TODO:  在此添加控件通知处理程序代码
			cJSON* root = nullptr;
			cJSON* cbusinessType = nullptr, * cRecordDownloadPath = nullptr, * cDownloadCutFormat = nullptr, * cDownloadCutDuration = nullptr, * cDownloadCutSize = nullptr;

			int32_t businessType = BUSINESS_TYPE_LOCAL_RECORD_START;
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

			cRecordDownloadPath = cJSON_CreateString("D://LocalRecord//");
			cJSON_AddItemToObject(root, "RecordDownloadPath", cRecordDownloadPath);

			_snprintf_s(cbuf, 96, "%d", 1);
			cDownloadCutFormat = cJSON_CreateString((char*)&cbuf[0]);
			cJSON_AddItemToObject(root, "DownloadCutFormat", cDownloadCutFormat);

			_snprintf_s(cbuf, 96, "%d", 5);
			cDownloadCutDuration = cJSON_CreateString((char*)&cbuf[0]);
			cJSON_AddItemToObject(root, "DownloadCutDuration", cDownloadCutDuration);

			_snprintf_s(cbuf, 96, "%d", 50);
			cDownloadCutSize = cJSON_CreateString((char*)&cbuf[0]);
			cJSON_AddItemToObject(root, "DownloadCutSize", cDownloadCutSize);

			result = cJSON_Print(root);
			int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
			m_bIsLocalRecord = true;
			cJSON_Delete(root);
			free(result);
		}
	}
}

//窗口屏幕选中，需要在UI界面设置该控件的Notify属性为True
void VideoPlayer::OnStnClickedStaticScreen()
{
	// TODO:  在此添加控件通知处理程序代码
	SetPlayerSelectStatus(true);
	m_pParent->FreshSelectStatus(m_iIndex);
}

//设置窗口选中状态
void VideoPlayer::SetPlayerSelectStatus(bool bSelected)
{
	m_bIsSelected = bSelected;
	CRect rect;
	GetClientRect(&rect);
	int iPlayerWidth = rect.Width() - PLAYER_BORDER_WIDTH * 2;
	int iPlayerHeight = rect.Height() - PLAYER_BORDER_WIDTH * 2;
	if (m_bIsSelected) {
		m_leftBorder.MoveWindow(0, 0, PLAYER_BORDER_WIDTH, rect.Height());
		m_rightBorder.MoveWindow(rect.Width() - PLAYER_BORDER_WIDTH, 0, PLAYER_BORDER_WIDTH, rect.Height());
		m_topBorder.MoveWindow(0, 0, rect.Width(), PLAYER_BORDER_WIDTH);
		m_bottomBorder.MoveWindow(0, rect.Height() - PLAYER_BORDER_WIDTH, rect.Width(), PLAYER_BORDER_WIDTH);

		m_leftBorder.ShowWindow(SW_SHOW);
		m_rightBorder.ShowWindow(SW_SHOW);
		m_topBorder.ShowWindow(SW_SHOW);
		m_bottomBorder.ShowWindow(SW_SHOW);
	}
	else {
		m_leftBorder.ShowWindow(SW_HIDE);
		m_rightBorder.ShowWindow(SW_HIDE);
		m_topBorder.ShowWindow(SW_HIDE);
		m_bottomBorder.ShowWindow(SW_HIDE);
	}
}

bool VideoPlayer::GetIsSelectStatus()
{
	return m_bIsSelected;
}

void VideoPlayer::StartPlayLiveOnGateWay(CString strCameraID, int iStreamType, CString strCameraName)
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

	CString strLoginedIp = L"114.116.223.94";
	int iLoginedPort = 10010;

	CHttpConnection* pConnection = session.GetHttpConnection(strLoginedIp, (INTERNET_PORT)iLoginedPort, g_strLoginedAccount, g_strLoginedPassword);
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_GET_REALTIME_LIVE, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc\nConnection: Keep-Alive";
	CString strRequestJson;
	strRequestJson.Format(_T("{\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"agentType\":1,\"urlType\":1,\"cameraId\":\"%s\",\"streamType\":%d}"), strCameraID, iStreamType);

	USES_CONVERSION;
	CHAR* strFormData = T2A(strRequestJson);
	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);
	if (dwRet == HTTP_STATUS_OK) {
		//读取http response数据
		int len = pFile->GetLength();
		char buf[2000];
		int numread;
		CString strJsonContent = L"";

		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0) {
			buf[numread] = '\0';
			strJsonContent += UTF8ToUnicode(buf);
		}

		const int iLength = strJsonContent.GetLength();
		LPSTR lpszTest = T2A(strJsonContent);
		cJSON* root = cJSON_Parse(lpszTest);
		if (!root) {
			return;
		}

		cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
		if (!jResCode || jResCode->valueint != 0) {
			AfxMessageBox(L"请求播放地址失败");
			return;
		}

		if (cJSON_GetObjectItem(root, "url")) {
			cJSON_Delete(root);
			CString strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
			//启动播放

			cJSON* root = nullptr;
			cJSON* cbusinessType = nullptr;
			cJSON* cwindowsHandle = nullptr;
			cJSON* crtspurl = nullptr;
			int32_t businessType = BUSINESS_TYPE_REALVIDEO_START;
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

			//调用allplayer接口，播放
			CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
			if (pPlayerFrame) {
				HWND playerHwnd = pPlayerFrame->m_hWnd;

				_snprintf_s(cbuf, 128, "%d", playerHwnd);
				cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

				_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
				crtspurl = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

				result = cJSON_Print(root);
				m_iCurtBusinessID = g_iBusinessID++;
				ap_lib_excute(m_iCurtBusinessID, (char*)result);
				cJSON_Delete(root);
				free(result);
			}
		}
	}
	else if (HTTP_STATUS_DENIED == dwRet) {
		//401 重新请求
		pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

		DWORD dwRet2;
		pFile->QueryInfoStatusCode(dwRet2);
		if (dwRet2 == HTTP_STATUS_OK) {
			//读取http response数据
			int len = pFile->GetLength();
			char buf[2000];
			int numread;
			CString strJsonContent = L"";

			while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0) {
				buf[numread] = '\0';
				strJsonContent += UTF8ToUnicode(buf);
			}

			const int iLength = strJsonContent.GetLength();
			LPSTR lpszTest = T2A(strJsonContent);
			cJSON* root = cJSON_Parse(lpszTest);
			if (!root) {
				return;
			}

			cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
			if (!jResCode || jResCode->valueint != 0) {
				AfxMessageBox(L"请求播放地址失败");
				if (root) {
					cJSON_Delete(root);
				}
				return;
			}

			if (cJSON_GetObjectItem(root, "url")) {
				CString strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
				//启动播放

				cJSON* root = nullptr;
				cJSON* cbusinessType = nullptr;
				cJSON* cwindowsHandle = nullptr;
				cJSON* crtspurl = nullptr;
				int32_t businessType = BUSINESS_TYPE_REALVIDEO_START;
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

				//调用allplayer接口，播放
				CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
				if (pPlayerFrame) {
					HWND playerHwnd = pPlayerFrame->m_hWnd;

					_snprintf_s(cbuf, 128, "%d", playerHwnd);
					cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

					_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
					crtspurl = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

					result = cJSON_Print(root);
					m_iCurtBusinessID = g_iBusinessID++;
					ap_lib_excute(m_iCurtBusinessID, (char*)result);
					cJSON_Delete(root);
					free(result);
				}
			}
		}
	}

	session.Close();
	pFile->Close();
	delete pFile;
}

//挑战鉴权
void VideoPlayer::StartPlayLive(CString strCameraID, int iStreamType, CString strCameraName)
{
	m_strCameraID = strCameraID;
	m_iStreamType = iStreamType;
	m_strCameraName = strCameraName;
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
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_GET_REALTIME_LIVE, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc\nConnection: Keep-Alive";
	CString strRequestJson;
	strRequestJson.Format(_T("{\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"agentType\":1,\"urlType\":1,\"cameraId\":\"%s\",\"streamType\":%d}"), strCameraID, iStreamType);

	USES_CONVERSION;
	CHAR* strFormData = T2A(strRequestJson);
	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));

	DWORD dwRet;
	pFile->QueryInfoStatusCode(dwRet);
	if (dwRet == HTTP_STATUS_OK) {
		//读取http response数据
		int len = pFile->GetLength(), numread;
		char buf[2000];
		CString strJsonContent = L"";

		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0) {
			buf[numread] = '\0';
			strJsonContent += UTF8ToUnicode(buf);
		}

		const int iLength = strJsonContent.GetLength();
		LPSTR lpszTest = T2A(strJsonContent);
		cJSON* root = cJSON_Parse(lpszTest);
		if (!root) {
			return;
		}

		cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
		if (!jResCode || jResCode->valueint != 0) {
			AfxMessageBox(L"请求播放地址失败");
			cJSON_Delete(root);
			return;
		}

		if (cJSON_GetObjectItem(root, "url")) {
			CString strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
			//启动播放
			cJSON* root = nullptr;
			cJSON* cbusinessType = nullptr;
			cJSON* cwindowsHandle = nullptr;
			cJSON* crtspurl = nullptr;
			int32_t businessType = BUSINESS_TYPE_REALVIDEO_START;
			//生成的结果
			char* result;

	#ifndef MULTI_PLAY
			if (m_bSetUrl) {
				businessType = BUSINESS_TYPE_SET_URL;
			}
	#endif

			root = cJSON_CreateObject();
			if (!root) {
				return;
			}
			char cbuf[1024] = { 0 };
			_snprintf_s(cbuf, 96, "%d", businessType);
			cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
			cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

			//调用allplayer接口，播放
			CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
			if (pPlayerFrame) {
				HWND playerHwnd = pPlayerFrame->m_hWnd;

				_snprintf_s(cbuf, 128, "%d", playerHwnd);
				cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

				_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
				crtspurl = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

				_snprintf_s(cbuf, 128, "%s", "1");
				cJSON* cdecodeType = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "DecodeType", cdecodeType);

				result = cJSON_Print(root);
				if (!m_bSetUrl) {
					m_iCurtBusinessID = g_iBusinessID++;
				}
#ifndef MULTI_PLAY
				ap_lib_excute(m_iCurtBusinessID, (char*)result);	
				m_bSetUrl = true;
#else
				HWND hWnd = (HWND)((CFrameWnd*)AfxGetApp()->m_pMainWnd->m_hWnd);
				if (NULL != hWnd) {
					::PostMessage(hWnd, WM_MULTI_PLAY_MSG, (WPARAM)(strUrl.AllocSysString()), LPARAM(4));
				}
#endif // MULTI_PLAY
				m_bLivePlaying = true;
				cJSON_Delete(root);
				free(result);
			}

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
				return;
			}

			cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
			if (!jResCode || jResCode->valueint != 0)
			{
				AfxMessageBox(L"请求播放地址失败");
				cJSON_Delete(root);
				return;
			}

			if (cJSON_GetObjectItem(root, "url"))
			{
				CString strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
				//启动播放
				cJSON_Delete(root);

				cJSON* root = nullptr;
				cJSON* cbusinessType = nullptr;
				cJSON* cwindowsHandle = nullptr;
				cJSON* crtspurl = nullptr;
				int32_t businessType = BUSINESS_TYPE_REALVIDEO_START;
				//生成的结果
				char* result;

		#ifndef MULTI_PLAY
				if (m_bSetUrl) {
					businessType = BUSINESS_TYPE_SET_URL;
				}
		#endif

				root = cJSON_CreateObject();
				if (!root) {
					return;
				}
				char cbuf[1024] = { 0 };
				_snprintf_s(cbuf, 96, "%d", businessType);
				cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

				//调用allplayer接口，播放
				CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
				if (pPlayerFrame) {
					HWND playerHwnd = pPlayerFrame->m_hWnd;

					_snprintf_s(cbuf, 128, "%d", playerHwnd);
					cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

					_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
					crtspurl = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

					_snprintf_s(cbuf, 128, "%s", "1");
					cJSON* cdecodeType = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "DecodeType", cdecodeType);

					result = cJSON_Print(root);
					if (!m_bSetUrl) {
						m_iCurtBusinessID = g_iBusinessID++;
					}
#ifndef MULTI_PLAY
					ap_lib_excute(m_iCurtBusinessID, (char*)result);
					m_bSetUrl = true;
#else
					HWND hWnd = (HWND)((CFrameWnd*)AfxGetApp()->m_pMainWnd->m_hWnd);
					if (NULL != hWnd)
					{
						::PostMessage(hWnd, WM_MULTI_PLAY_MSG, (WPARAM)(strUrl.AllocSysString()), LPARAM(4));
					}
#endif // MULTI_PLAY
					m_bLivePlaying = true;
					cJSON_Delete(root);
					free(result);
				}
			}
		}
	}

	session.Close();
	pFile->Close();
	delete pFile;
}

void VideoPlayer::StopPlayLive()
{
	if (-1 != m_iCurtBusinessID) {
		if (m_bIsLocalRecord) {
			OnBnClickedButtonRecord();
		}
		cJSON* root = nullptr;
		cJSON* cbusinessType = nullptr;

		int32_t businessType = BUSINESS_TYPE_REALVIDEO_STOP;
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
		ap_lib_excute(m_iCurtBusinessID, (char*)result);
		m_iCurtBusinessID = -1;
		cJSON_Delete(root);
		free(result);
		m_playScreen.ShowWindow(SW_SHOW);
		m_bLivePlaying = false;
	}
}

//void VideoPlayer::StartPlayLive(CString strCameraID, int iStreamType, CString strCameraName)
//{
//	//获取实时浏览地址
//	CInternetSession session;
//	//设置超时和连接重试次数
//	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
//	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
//	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 10000);
//	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 3);
//	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 5000);
//	int iPort = _ttoi(g_strLoginedPort);
//	//CHttpConnection* pConnection = session.GetHttpConnection(g_strLoginedIP, (INTERNET_PORT)iPort, g_strLoginedAccount, g_strLoginedPassword);
//	CHttpConnection* pConnection = session.GetHttpConnection(g_strLoginedIP, (INTERNET_PORT)iPort);
//	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_GET_REALTIME_LIVE, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);
//
//	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc\nConnection: Keep-Alive";
//	CString strRequestJson;
//	strRequestJson.Format(_T("{\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"agentType\":1,\"urlType\":1,\"cameraId\":\"%s\",\"streamType\":%d}"), strCameraID, iStreamType);
//	
//	USES_CONVERSION;
//	CHAR* strFormData = T2A(strRequestJson);
//	CString strAuth;
//	strAuth.Format(_T("Authorization: Bearer %s\r\n"), g_strLoginedToken);
//	pFile->AddRequestHeaders(strAuth);
//	pFile->AddRequestHeaders(_T("Accept-Language: zh-CN,en,*\r\n"));
//	pFile->AddRequestHeaders(_T("Accept-Encoding: gzip, deflate\r\n"));
//	pFile->SendRequest(szHeaders, szHeaders.GetLength(), (LPVOID)strFormData, strlen(strFormData));
//
//	DWORD dwRet;
//	pFile->QueryInfoStatusCode(dwRet);
//	if (dwRet == HTTP_STATUS_OK)
//	{
//		//读取http response数据
//		int len = pFile->GetLength();
//		char buf[2000];
//		int numread;
//		CString strJsonContent = L"";
//
//		while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
//		{
//			buf[numread] = '\0';
//			strJsonContent += UTF8ToUnicode(buf);
//		}
//		//AfxMessageBox(strJsonContent);
//
//		const int iLength = strJsonContent.GetLength();
//		LPSTR lpszTest = T2A(strJsonContent);
//		cJSON* root = cJSON_Parse(lpszTest);
//		if (!root) {
//			return;
//		}
//
//		cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
//		if (!jResCode || jResCode->valueint != 0)
//		{
//			AfxMessageBox(L"请求播放地址失败");
//			return;
//		}
//
//		if (cJSON_GetObjectItem(root, "url"))
//		{
//			CString strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
//			//启动播放
//
//			cJSON* root = nullptr;
//			cJSON* cbusinessType = nullptr;
//			cJSON* cwindowsHandle = nullptr;
//			cJSON* crtspurl = nullptr;
//
//			//string url = "rtsp://admin:Allcam123@172.16.20.127:554/Streaming/Channels/101?transportmode=unicast";
//			//string url = "rtsp://172.16.20.252/LiveMedia/ch1/Media1";
//			//string url = "rtsp://127.0.0.1:8554/test";
//			int32_t businessType = 0;
//			//生成的结果
//			char* result;
//
//			root = cJSON_CreateObject();
//			char cbuf[1024] = { 0 };
//			_snprintf_s(cbuf, 96, "%d", businessType);
//			cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
//			cJSON_AddItemToObject(root, "BusinessType", cbusinessType);
//
//			//调用allplayer接口，播放
//			CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
//			if (pPlayerFrame) {
//				HWND playerHwnd = pPlayerFrame->m_hWnd;
//
//				_snprintf_s(cbuf, 128, "%d", playerHwnd);
//				cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
//				cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);
//
//				_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
//				crtspurl = cJSON_CreateString((char*)&cbuf[0]);
//				cJSON_AddItemToObject(root, "RtspUrl", crtspurl);
//
//				result = cJSON_Print(root);
//				m_iCurtBusinessID = g_iBusinessID++;
//				ap_lib_excute(m_iCurtBusinessID, (char*)result);
//			}
//
//		}
//	}
//
//	session.Close();
//	pFile->Close();
//	delete pFile;

void VideoPlayer::StartPlayRecord(int cacheSize)
{
	if (!g_bDialogTypeChecked)
	{
		return;
	}

	//获取录像回放播放地址
	CInternetSession session;
	//设置超时和连接重试次数
	session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);
	session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
	session.SetOption(INTERNET_OPTION_SEND_TIMEOUT, 10000);
	session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 3);
	session.SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 5000);
	int iPort = _ttoi(g_strLoginedPort);
	CHttpConnection* pConnection = session.GetHttpConnection(g_strLoginedIP, (INTERNET_PORT)iPort, g_strLoginedAccount, g_strLoginedPassword);
	CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, URL_GET_RECORD_LIVE, NULL, 1, NULL, L"HTTP/1.1", INTERNET_FLAG_RELOAD);

	CString szHeaders = L"Content-Type:application/json;charset=UTF-8\nUser-Agent:allcam-wpc";
	CString strRequestJson;
	//strRequestJson.Format(_T("{\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"agentType\":1,\"urlType\":1,\"cameraId\":\"%s\",\"streamType\":%d}"), strCameraID, iStreamType);
	strRequestJson.Format(_T("{\"agentType\":1,\"cameraId\":\"%s\",\"clientNonce\":\"wpc01202104300945531486529\",\"cuType\":\"1\",\"streamType\":1,\"urlType\":1,\"vodInfo\":{\"beginTime\":\"%s\",\"cameraId\":\"%s\",\"contentId\":\"%s\",\"endTime\":\"%s\",\"location\":%d,\"nvrCode\":\"%s\"},\"vodType\":\"vod\"}"), g_curtRecordSubsection.getCameraId(), g_curtRecordSubsection.getBeginTime(), g_curtRecordSubsection.getCameraId(), g_curtRecordSubsection.getContentId(), g_curtRecordSubsection.getEndTime(), g_curtRecordSubsection.getLocation(), g_curtRecordSubsection.getNvrCode());

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
			AfxMessageBox(L"请求录像播放地址失败-1");
			cJSON_Delete(root);
			return;
		}

		cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
		if (!jResCode || jResCode->valueint != 0)
		{
			AfxMessageBox(L"请求录像播放地址失败-2");
			cJSON_Delete(root);
			return;
		}

		if (cJSON_GetObjectItem(root, "url"))
		{
			cJSON_Delete(root);
			CString strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
			//启动播放

			NetRecordDownloadTest(strUrl);
			return;

			cJSON* root = nullptr;
			cJSON* cbusinessType = nullptr;
			cJSON* cwindowsHandle = nullptr;
			cJSON* crtspurl = nullptr;
			int32_t businessType = BUSINESS_TYPE_NETRECORD_START;
			//生成的结果
			char* result;

			root = cJSON_CreateObject();
			if (root) {
				return;
			}
			char cbuf[1024] = { 0 };
			_snprintf_s(cbuf, 96, "%d", businessType);
			cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
			cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

			//调用allplayer接口，播放
			CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
			if (pPlayerFrame) {
				HWND playerHwnd = pPlayerFrame->m_hWnd;

				_snprintf_s(cbuf, 128, "%d", playerHwnd);
				cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

				_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
				crtspurl = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

				result = cJSON_Print(root);
				m_iCurtBusinessID = g_iBusinessID++;
				ap_lib_excute(m_iCurtBusinessID, (char*)result);
				cJSON_Delete(root);
				free(result);
			}

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
				AfxMessageBox(L"请求录像播放地址失败-3");
				cJSON_Delete(root);
				return;
			}

			cJSON* jResCode = cJSON_GetObjectItem(root, "resultCode");
			if (!jResCode || jResCode->valueint != 0)
			{
				AfxMessageBox(L"请求录像播放地址失败-4");
				cJSON_Delete(root);
				return;
			}

			if (cJSON_GetObjectItem(root, "url"))
			{
				CString strUrl = CString(cJSON_GetObjectItem(root, "url")->valuestring);
				cJSON_Delete(root);
				//启动播放
				cJSON* root = nullptr;
				cJSON* cbusinessType = nullptr;
				cJSON* cwindowsHandle = nullptr;
				cJSON* crtspurl = nullptr;
				int32_t businessType = BUSINESS_TYPE_NETRECORD_START;
				//生成的结果
				char* result;

				root = cJSON_CreateObject();
				char cbuf[1024] = { 0 };
				_snprintf_s(cbuf, 96, "%d", businessType);
				cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
				cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

				//调用allplayer接口，播放
				CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
				if (pPlayerFrame) {
					HWND playerHwnd = pPlayerFrame->m_hWnd;

					_snprintf_s(cbuf, 128, "%d", playerHwnd);
					cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

					_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
					crtspurl = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

					_snprintf_s(cbuf, 96, "%d", 1);
					auto cQuality = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "RealOrQuality", cQuality);

					_snprintf_s(cbuf, 96, "%d", cacheSize);
					auto cCacheSize = cJSON_CreateString((char*)&cbuf[0]);
					cJSON_AddItemToObject(root, "CacheSize", cCacheSize);

					result = cJSON_Print(root);
					m_iCurtBusinessID = g_iBusinessID++;
					ap_lib_excute(m_iCurtBusinessID, (char*)result);
					cJSON_Delete(root);
					free(result);
				}
			}
		}
	}

	session.Close();
	pFile->Close();
	delete pFile;
}

void VideoPlayer::StopPlayRecord()
{
	if (!g_bDialogTypeChecked)
	{
		return;
	}

	if (-1 == m_iCurtBusinessID)
	{
		return;
	}

	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;

	int32_t businessType = BUSINESS_TYPE_NETRECORD_STOP;
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
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	m_iCurtBusinessID = -1;
	cJSON_Delete(root);
	free(result);
	m_playScreen.ShowWindow(SW_SHOW);
}

//录像回放下载测试
void VideoPlayer::NetRecordDownloadTest(CString strUrl)
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	//cJSON* cwindowsHandle = nullptr;
	cJSON* crtspurl = nullptr;
	cJSON* cRecordDownloadPath = nullptr;
	cJSON* cCutFormat = nullptr;
	cJSON* cCutSize = nullptr;
	cJSON* cCutDuration = nullptr;
	int32_t businessType = BUSINESS_TYPE_DOWNLOAD_START;
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

	USES_CONVERSION;
	_snprintf_s(cbuf, 1000, "%s", T2A(strUrl));
	crtspurl = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

	CString strRecordDownloadPath = L"D://NetRecordDownload//";
	_snprintf_s(cbuf, 1000, "%s", T2A(strRecordDownloadPath));
	cRecordDownloadPath = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "RecordDownloadPath", cRecordDownloadPath);

	//0-按时间 1-按大小
	_snprintf_s(cbuf, 96, "%d", 1);
	cCutFormat = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "DownloadCutFormat", cCutFormat);

	//2M
	_snprintf_s(cbuf, 96, "%d", 2);
	cCutSize = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "DownloadCutSize", cCutSize);

	//1min
	_snprintf_s(cbuf, 96, "%d", 1);
	cCutDuration = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "DownloadCutDuration", cCutDuration);

	result = cJSON_Print(root);
	m_iCurtBusinessID = g_iBusinessID++;
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

//录像回放下载结束测试
void VideoPlayer::NetRecordDownloadStopTest()
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	int32_t businessType = BUSINESS_TYPE_DOWNLOAD_STOP;
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

	result = cJSON_Print(root);
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

//录像回放下载暂停测试
void VideoPlayer::NetRecordDownloadPauseTest()
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	int32_t businessType = BUSINESS_TYPE_DOWNLOAD_PAUSE;
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

	result = cJSON_Print(root);
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

//录像回放下载继续测试
void VideoPlayer::NetRecordDownloadResumeTest()
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	int32_t businessType = BUSINESS_TYPE_DOWNLOAD_CONTINUE;
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

	result = cJSON_Print(root);
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::FilterAdjustTest()
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	cJSON* brightness = nullptr, * contrast = nullptr, * saturation = nullptr;
	cJSON* smartblur = nullptr, * unsharp = nullptr, * hqdn3d = nullptr;
	int32_t businessType = BUSINESS_TYPE_PICTURE_PARAMS;
	//生成的结果
	char* result;
	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	//调用allplayer接口
	_snprintf_s(cbuf, 96, "%d", businessType);
	cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	brightness = cJSON_CreateString("60");
	cJSON_AddItemToObject(root, "brightness", brightness);

	contrast = cJSON_CreateString("60");
	cJSON_AddItemToObject(root, "contrast", contrast);

	saturation = cJSON_CreateString("70");
	cJSON_AddItemToObject(root, "saturation", saturation);

	unsharp = cJSON_CreateObject();
	cJSON* luma_x = cJSON_CreateString("100");
	cJSON_AddItemToObject(unsharp, "luma_x", luma_x);

	cJSON* luma_y = cJSON_CreateString("100");
	cJSON_AddItemToObject(unsharp, "luma_y", luma_y);

	cJSON* luma_amount = cJSON_CreateString("60");
	cJSON_AddItemToObject(unsharp, "luma_amount", luma_amount);

	cJSON_AddItemToObject(root, "unsharp", unsharp);

	_snprintf_s(cbuf, 10, "%s", "1");
	hqdn3d = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "hqdn3d", hqdn3d);

	result = cJSON_Print(root);
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::getFilterParams()
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;

	int32_t businessType = BUSINESS_TYPE_GET_PIC_PARAMS;
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
	char* pMediaInfo = (char*)malloc(1024);
	if (NULL == pMediaInfo) {
		return;
	}
	memset(pMediaInfo, 0, 1024);
	ap_lib_excute(m_iCurtBusinessID, (char*)result, pMediaInfo);
	free(pMediaInfo);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::AudioTalkTest()
{
}

void VideoPlayer::NetRecordStepFrameTest(int bizType)
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	int32_t businessType = bizType;
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

	result = cJSON_Print(root);
	int iRet = ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::TestHWDecode()
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	cJSON* cwindowsHandle = nullptr;
	cJSON* crtspurl = nullptr, * cdecodeType = nullptr;

	string url = "rtsp://admin:Allcam2019@172.16.20.94/ch1/av_stream";
	//string url = "rtsp://admin:Allcam@2021@36.152.144.102/ch1/av_stream";
	int32_t businessType = BUSINESS_TYPE_REALVIDEO_START;
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

	//调用allplayer接口，播放
	CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
	if (pPlayerFrame) {
		HWND playerHwnd = pPlayerFrame->m_hWnd;

		_snprintf_s(cbuf, 128, "%d", playerHwnd);
		cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

		_snprintf_s(cbuf, 1000, "%s", url.c_str());
		crtspurl = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

		_snprintf_s(cbuf, 128, "%s", "1");
		cdecodeType = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "DecodeType", cdecodeType);

		result = cJSON_Print(root);
		m_iCurtBusinessID = g_iBusinessID++;
		ap_lib_excute(m_iCurtBusinessID, (char*)result);
		free(result);
	}
	cJSON_Delete(root);
}

void VideoPlayer::testSocks5()
{
	cJSON* root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", BUSINESS_TYPE_SOCKS_INIT);
	cJSON* json = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", json);

	_snprintf_s(cbuf, 128, "%s", "127.0.0.1");
	json = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "SocksIP", json);

	_snprintf_s(cbuf, 128, "%s", "1080");
	json = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "SocksPort", json);

	_snprintf_s(cbuf, 128, "%s", "jack");
	json = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "SocksUsername", json);

	_snprintf_s(cbuf, 128, "%s", "1111");
	json = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "SocksPassword", json);

	char* result = cJSON_Print(root);
	ap_lib_excute(0, (char*)result);
	free(result);
	cJSON_Delete(root);
}

void VideoPlayer::cancelSocks5()
{
	cJSON* root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", BUSINESS_TYPE_SOCKS_CANCEL);
	cJSON* json = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", json);

	char* result = cJSON_Print(root);
	ap_lib_excute(0, (char*)result);
	free(result);
	cJSON_Delete(root);
}

void VideoPlayer::RecordVCRControl(double dPos)
{
	if (!g_bDialogTypeChecked)
	{
		return;
	}

	if (-1 == m_iCurtBusinessID)
	{
		return;
	}

	//录像vcr控制
	g_dRecordScale;

	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	cJSON* cScale = nullptr;
	cJSON* cPos = nullptr;
	int32_t businessType = BUSINESS_TYPE_NETRECORD_CONTROL;
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

	_snprintf_s(cbuf, 24, "%f", g_dRecordScale);
	cScale = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "PlayScale", cScale);

	_snprintf_s(cbuf, 24, "%f", dPos);
	cPos = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "NptPos", cPos);

	result = cJSON_Print(root);
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::GetMediaInfo()
{
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;

	int32_t businessType = BUSINESS_TYPE_GET_MEDIA_INFO;
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
	char* pMediaInfo = (char*)malloc(1024);
	if (NULL == pMediaInfo) {
		return;
	}
	memset(pMediaInfo, 0, 1024);
	ap_lib_excute(m_iCurtBusinessID, (char*)result, pMediaInfo);
	free(pMediaInfo);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::GetExperienceInfo()
{
	cJSON* root = nullptr, * cbusinessType = nullptr, * idArr = nullptr;
	int32_t businessType = TYPE_GET_EXPERIENCE_DATA;
	char cbuf[1024] = { 0 };
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	_snprintf_s(cbuf, 96, "%d", businessType);
	cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

	idArr = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "BussinessIdArr", idArr);

	for (int i = 0; i < g_iBusinessID; ++i)
	{
		cJSON* id = cJSON_CreateNumber(i);
		if (id)	cJSON_AddItemToArray(idArr, id);
	}

	result = cJSON_Print(root);
	char* szExperience = (char*)malloc(1024);
	memset(szExperience, 0, 1024);
	ap_lib_excute(-1, (char*)result, szExperience);
	cJSON_Delete(root);
	free(result);
	free(szExperience);
}

void VideoPlayer::UrlPlayTest(CString strUrl, bool start)
{
	string url = CT2A(strUrl.GetBuffer());
	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	cJSON* cwindowsHandle = nullptr;
	cJSON* crtspurl = nullptr, * volume = nullptr;

	int32_t businessType;
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };

	CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
	if (pPlayerFrame && start) {

		businessType = BUSINESS_TYPE_REALVIDEO_START;
		_snprintf_s(cbuf, 96, "%d", businessType);
		cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "BusinessType", cbusinessType);

		HWND playerHwnd = pPlayerFrame->m_hWnd;

		_snprintf_s(cbuf, 128, "%d", playerHwnd);
		cwindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "BusinessHwnd", cwindowsHandle);

		_snprintf_s(cbuf, 1000, "%s", url.c_str());
		crtspurl = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "RtspUrl", crtspurl);

		result = cJSON_Print(root);
		m_iCurtBusinessID = g_iBusinessID++;
		ap_lib_excute(m_iCurtBusinessID, (char*)result);
		free(result);
	}
	else {
		businessType = BUSINESS_TYPE_REALVIDEO_STOP;
		_snprintf_s(cbuf, 96, "%d", businessType);
		cbusinessType = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "BusinessType", cbusinessType);
		result = cJSON_Print(root);
		ap_lib_excute(m_iCurtBusinessID, (char*)result);
		free(result);
	}
}

void VideoPlayer::OnBnClickedButtonPause()
{
	// TODO:  在此添加控件通知处理程序代码
	if (-1 == m_iCurtBusinessID) {
		return;
	}

	//NetRecordDownloadPauseTest();

	cJSON* root = nullptr;
	cJSON* cBusinessType = nullptr;

	int32_t businessType = BUSINESS_TYPE_URL_PAUSE;
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", businessType);
	cBusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cBusinessType);

	result = cJSON_Print(root);
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}


void VideoPlayer::OnBnClickedButtonContinue()
{
	// TODO:  在此添加控件通知处理程序代码
	if (-1 == m_iCurtBusinessID) {
		return;
	}

	//NetRecordDownloadResumeTest();

	cJSON* root = nullptr;
	cJSON* cBusinessType = nullptr;
	int32_t businessType = BUSINESS_TYPE_URL_RESUE;
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", businessType);
	cBusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cBusinessType);

	result = cJSON_Print(root);
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}


void VideoPlayer::OnBnClickedButtonZoomStart()
{
	if (-1 == m_iCurtBusinessID) {
		return;
	}

	// TODO:  在此添加控件通知处理程序代码
	CString strTop;
	m_zoomTopEdit.GetWindowTextW(strTop);
	int iTop = _ttoi(strTop);
	iTop = iTop < 0 ? 0 : (iTop > 100 ? 100 : iTop);

	CString strRight;
	m_zoomRightEdit.GetWindowTextW(strRight);
	int iRight = _ttoi(strRight);
	iRight = iRight < 0 ? 0 : (iRight > 100 ? 100 : iRight);

	CString strBottom;
	m_zoomBottomEdit.GetWindowTextW(strBottom);
	int iBottom = _ttoi(strBottom);
	iBottom = iBottom < 0 ? 0 : (iBottom > 100 ? 100 : iBottom);

	CString strLeft;
	m_zoomLeftEdit.GetWindowTextW(strLeft);
	int iLeft = _ttoi(strLeft);
	iLeft = iLeft < 0 ? 0 : (iLeft > 100 ? 100 : iLeft);

	cJSON* root = nullptr;
	cJSON* cBusinessType = nullptr;
	cJSON* cWindowsHandle = nullptr;
	cJSON* cZoomTop = nullptr;
	cJSON* cZoomRight = nullptr;
	cJSON* cZoomBottom = nullptr;
	cJSON* cZoomLeft = nullptr;

	int32_t businessType = BUSINESS_TYPE_DIGITAL_START;
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", businessType);
	cBusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cBusinessType);

	//调用allplayer接口
	CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_ZOOM_PLAYER);
	if (pPlayerFrame) {
		HWND playerHwnd = pPlayerFrame->m_hWnd;

		_snprintf_s(cbuf, 128, "%d", playerHwnd);
		cWindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialHwnd", cWindowsHandle);

		_snprintf_s(cbuf, 24, "%d", iTop);
		cZoomTop = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialTop", cZoomTop);

		_snprintf_s(cbuf, 24, "%d", iRight);
		cZoomRight = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialRight", cZoomRight);

		_snprintf_s(cbuf, 24, "%d", iBottom);
		cZoomBottom = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialBottom", cZoomBottom);

		_snprintf_s(cbuf, 24, "%d", iLeft);
		cZoomLeft = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialLeft", cZoomLeft);

		result = cJSON_Print(root);
		ap_lib_excute(m_iCurtBusinessID, (char*)result);
		cJSON_Delete(root);
		free(result);
		//m_zoomPlayerLab.ShowWindow(SW_SHOW);
	}
}


void VideoPlayer::OnBnClickedButtonZoomUpdate()
{
	if (-1 == m_iCurtBusinessID) {
		return;
	}

	CString strTop;
	m_zoomTopEdit.GetWindowTextW(strTop);
	int iTop = _ttoi(strTop);
	iTop = iTop < 0 ? 0 : (iTop > 100 ? 100 : iTop);

	CString strRight;
	m_zoomRightEdit.GetWindowTextW(strRight);
	int iRight = _ttoi(strRight);
	iRight = iRight < 0 ? 0 : (iRight > 100 ? 100 : iRight);

	CString strBottom;
	m_zoomBottomEdit.GetWindowTextW(strBottom);
	int iBottom = _ttoi(strBottom);
	iBottom = iBottom < 0 ? 0 : (iBottom > 100 ? 100 : iBottom);

	CString strLeft;
	m_zoomLeftEdit.GetWindowTextW(strLeft);
	int iLeft = _ttoi(strLeft);
	iLeft = iLeft < 0 ? 0 : (iLeft > 100 ? 100 : iLeft);

	cJSON* root = nullptr;
	cJSON* cBusinessType = nullptr;
	cJSON* cWindowsHandle = nullptr;
	cJSON* cZoomTop = nullptr;
	cJSON* cZoomRight = nullptr;
	cJSON* cZoomBottom = nullptr;
	cJSON* cZoomLeft = nullptr;

	int32_t businessType = BUSINESS_TYPE_DIGITAL_UPDATE;
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", businessType);
	cBusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cBusinessType);

	//调用allplayer接口
	CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_ZOOM_PLAYER);
	if (pPlayerFrame) {
		HWND playerHwnd = pPlayerFrame->m_hWnd;

		_snprintf_s(cbuf, 128, "%d", playerHwnd);
		cWindowsHandle = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialHwnd", cWindowsHandle);

		_snprintf_s(cbuf, 24, "%d", iTop);
		cZoomTop = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialTop", cZoomTop);

		_snprintf_s(cbuf, 24, "%d", iRight);
		cZoomRight = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialRight", cZoomRight);

		_snprintf_s(cbuf, 24, "%d", iBottom);
		cZoomBottom = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialBottom", cZoomBottom);

		_snprintf_s(cbuf, 24, "%d", iLeft);
		cZoomLeft = cJSON_CreateString((char*)&cbuf[0]);
		cJSON_AddItemToObject(root, "PartialLeft", cZoomLeft);

		result = cJSON_Print(root);
		ap_lib_excute(m_iCurtBusinessID, (char*)result);
		cJSON_Delete(root);
		free(result);
		//m_zoomPlayerLab.ShowWindow(SW_SHOW);
	}
}


void VideoPlayer::OnBnClickedButtonZoomEnd()
{
	// TODO:  在此添加控件通知处理程序代码
	if (-1 == m_iCurtBusinessID)
	{
		return;
	}

	cJSON* root = nullptr;
	cJSON* cBusinessType = nullptr;

	int32_t businessType = BUSINESS_TYPE_DIGITAL_STOP;
	//生成的结果
	char* result;

	root = cJSON_CreateObject();
	if (!root) {
		return;
	}
	char cbuf[1024] = { 0 };
	_snprintf_s(cbuf, 96, "%d", businessType);
	cBusinessType = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "BusinessType", cBusinessType);

	result = cJSON_Print(root);
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}


void VideoPlayer::OnNMCustomdrawSliderVolume(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	int volume = m_volumeSlider.GetPos();

	cJSON* root = nullptr;
	cJSON* cbusinessType = nullptr;
	cJSON* volumeJson = nullptr;

	int32_t businessType = TYPE_URL_VOLUME; //BUSINESS_TYPE_VOLUME_CONTROL;
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

	static int iVolume = 0;
	iVolume += 9;
	volumeJson = cJSON_CreateNumber(100);
	cJSON_AddItemToObject(root, "Volume", volumeJson);

	result = cJSON_Print(root);
	ap_lib_excute(m_iCurtBusinessID, (char*)result);
	cJSON_Delete(root);
	free(result);
}

void VideoPlayer::OnBnClickedButtonForward()
{
	// TODO: 在此添加控件通知处理程序代码
	NetRecordStepFrameTest(BUSINESS_TYPE_STEP_FORWARD);
}

void VideoPlayer::OnBnClickedButtonBackward()
{
	// TODO: 在此添加控件通知处理程序代码
	NetRecordStepFrameTest(BUSINESS_TYPE_STEP_BACKWARD);
}


void VideoPlayer::OnBnClickedButtonExit()
{
	// TODO: 在此添加控件通知处理程序代码
	NetRecordStepFrameTest(BUSINESS_TYPE_STEP_EXIT);
}


void VideoPlayer::OnBnClickedCheckMultiPlay()
{
	// TODO: 在此添加控件通知处理程序代码
}

bool VideoPlayer::GetIsMultiPlayer()
{
	if (NULL != m_multiPlayCheckBtn && m_multiPlayCheckBtn.GetCheck()) {
		return true;
	}
	return false;
}

HWND VideoPlayer::GetPlayerHwnd()
{
	CStatic* pPlayerFrame = (CStatic*)GetDlgItem(IDC_STATIC_SCREEN);
	HWND playerHwnd = pPlayerFrame->m_hWnd;
	return playerHwnd;
}

void VideoPlayer::switchUrlInCycling()
{
	/*StopPlayLive();
	StartPlayLive(m_strCameraID, m_iStreamType, m_strCameraName);*/
	OnBnClickedButtonStop();
	OnBnClickedButtonPlay();
}

bool VideoPlayer::isLivePlaying()
{
	return m_bLivePlaying;
}
