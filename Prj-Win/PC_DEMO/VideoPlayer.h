#pragma once

#include "SplitScreenWindow.h"

#define PLAYER_ACTION_BAR_HEIGHT  40
#define PLAYER_BORDER_WIDTH 1

//播放器状态
typedef struct structStatus
{
	bool bIsPlaying;
	bool bIsPausing;
	bool bIsLocalRecrding;
}Player_Status;

// VideoPlayer 对话框

class CSplitScreenWindow;

class VideoPlayer : public CDialogEx
{
	DECLARE_DYNAMIC(VideoPlayer)

public:
	VideoPlayer(int iIndex, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~VideoPlayer();
	HWND GetPlayerHwnd();
	bool GetIsMultiPlayer();  //获取是否为拼接流播放窗口
	bool GetIsSelectStatus();
	void StartPlayLive(CString strCameraID, int iStreamType, CString strCameraName);
	void StartPlayLiveOnGateWay(CString strCameraID, int iStreamType, CString strCameraName);
	void StopPlayLive();

	void StartPlayRecord(int cacheSize);
	void StopPlayRecord();

	//录像VCR控制
	void RecordVCRControl(double dPos);

	void GetMediaInfo();

	void GetExperienceInfo();

	void SetWaterMark();

	void UrlPlayTest(CString strUrl, bool start);

	//录像回放下载测试
	void NetRecordDownloadTest(CString strUrl);

	//录像回放下载结束测试
	void NetRecordDownloadStopTest();

	//录像回放下载暂停测试
	void NetRecordDownloadPauseTest();

	//录像回放下载继续测试
	void NetRecordDownloadResumeTest();

	//语音对讲测试
	void AudioTalkTest();

	//单帧播放测试
	void NetRecordStepFrameTest(int bizType);

	void FilterAdjustTest();

	void getFilterParams();

	void TestHWDecode();

	void testSocks5();

	void cancelSocks5();

	void switchUrlInCycling();

	bool isLivePlaying();

	// 对话框数据
	enum { IDD = IDD_VIDEOPLAYER };

public:
	void InitPlayerView();
	//设置窗口选中状态
	void SetPlayerSelectStatus(bool bSelected);

	CStatic m_leftBorder;
	CStatic m_topBorder;
	CStatic m_rightBorder;
	CStatic m_bottomBorder;

	CStatic m_playScreen;
	CButton m_playBtn;
	CButton m_stopBtn;
	CButton m_captureBtn;
	CButton m_localRecordBtn;
	CSliderCtrl m_volumeSlider;
	Player_Status m_playerStatus;

	CButton m_pauseBtn;
	CButton m_continueBtn;
	CStatic m_zoomLab;
	CEdit m_zoomTopEdit;
	CEdit m_zoomRightEdit;
	CEdit m_zoomBottomEdit;
	CEdit m_zoomLeftEdit;
	CButton m_zoomStartBtn;
	CButton m_zoomUpdateBtn;
	CButton m_zoomEndBtn;
	CStatic m_zoomPlayerLab;

	CButton m_forwardBtn;
	CButton m_backwardBtn;
	CButton m_stepExitBtn;

	CButton m_multiPlayCheckBtn;

	bool	m_DigitalShow = false;
	long 	m_iCurtBusinessID;
	//CEdit m_urlEdit;
	CString m_strCameraID;
	int		m_iStreamType;
	CString m_strCameraName;
	bool	m_bLivePlaying;
	bool	m_bSetUrl;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSize(UINT nType, int cx, int cy);

private:
	HBRUSH m_screenBKBrush;
	HBRUSH m_borderBKBrush;
	bool m_bIsSelected;
	CSplitScreenWindow* m_pParent;
	int m_iIndex;
	bool m_bIsLocalRecord = false;

public:
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonCapture();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnStnClickedStaticScreen();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonContinue();
	afx_msg void OnBnClickedButtonZoomStart();
	afx_msg void OnBnClickedButtonZoomUpdate();
	afx_msg void OnBnClickedButtonZoomEnd();
	afx_msg void OnNMCustomdrawSliderVolume(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonForward();
	afx_msg void OnBnClickedButtonBackward();
	afx_msg void OnBnClickedButtonExit();
	afx_msg void OnBnClickedCheckMultiPlay();
};
