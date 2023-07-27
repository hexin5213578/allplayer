#pragma once

#include "VideoPlayer.h"
#include "CommonDefine.h"
#include <vector>

// CSplitScreenWindow 对话框

class VideoPlayer;
class CSplitScreenWindow : public CDialogEx
{
	DECLARE_DYNAMIC(CSplitScreenWindow)

public:
	CSplitScreenWindow(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSplitScreenWindow();
	void FreshSelectStatus(int iIndex);

	void StartPlayLive(CString strCameraID, int iStreamType, CString strCameraName);

	void RecordVCRControl(double dCurtPos);

	void StopAllVideo(bool bRecord);

	void GetMediaInfo();

	// 对话框数据
	enum { IDD = IDD_SPLITSCREENWINDOW };

public:
	VideoPlayer* m_pVideoPlayerArr[SPLIT_WND_MAX_NUM];  //窗口缓存
	int m_iCurtPlayerIndex;  //当前窗口
	int m_iCurtSplitType;  //当前分屏类型
	bool m_playing = true;

	//初始化分屏视图界面
	void InitSplitScreenView();

	//创建播放窗口
	void CreateVideoPlayers();

	//设置分屏类型
	void SetSplitType(int iType);

	//排列分屏窗口
	void ArrangeVideoPlayers();

	std::vector<VideoPlayer*> getMultiPlayWindows();

	VideoPlayer* getFirstPlayWindow();

	void switchUrlInCycling();

public:
	BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	DECLARE_MESSAGE_MAP()

public:


private:
	HBRUSH m_dlgBrush;
};
