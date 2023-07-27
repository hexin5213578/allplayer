
// MFC_DIALOG_2Dlg.h : 头文件
//

#pragma once

//#include "MyTestDialog.h"
#include "SplitScreenWindow.h"
#include "AScJSON.h"
#include "DeviceTreeInfo.h"
#include "RecordNodeInfo.h"
#include "timer.h"

#define WM_PLAY_STATUS_MSG (WM_USER+100)  //播放状态通知 
#define WM_PLAY_DATA_MSG (WM_USER+101)    //播放进度通知

#define TOP_ACTION_BAR_HEIGHT 30

#define BOTTOM_ACTION_BAR_HEIGHT 45

//接口定义
#define URL_LOGIN  L"/uas/v1/api/user/login"
#define URL_GET_DEVICE_TREE  L"/uas/v1/api/udc/device/tree"
#define URL_GET_RECORD_LIST  L"/uas/v1/api/record/list"
#define URL_GET_AUDIOTALK_URL  L"/uas/v1/api/audio/talk"

// CMFC_DIALOG_2Dlg 对话框
class CMFC_DIALOG_2Dlg : public CDialogEx
{
	// 构造
public:
	CMFC_DIALOG_2Dlg(CWnd* pParent = NULL);	// 标准构造函数

	virtual ~CMFC_DIALOG_2Dlg();

	// 对话框数据
	enum { IDD = IDD_MFC_DIALOG_2_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	//MyTestDialog* m_pMyTestDialog;

private:
	void Login();
	void GetDeviceTree();
	void GetDeviceTreeOnGateWay();
	void LoadDeviceTree();
	void LoadNode(HTREEITEM parentNode, DeviceTreeNodeData data);
	CString GetRealTimeLive(CString strCameraID);

	void StartPlay(CString strCameraID, CString strCameraName = L"");

	//加载录像列表
	void LoadRecordInfoList();

	//获取设备推流地址
	int getAudioTalkBackUrl(CString& strUrl);

	//更新验证码
	void UpdateVerification();

	//语音对讲测试
	void AudioTalkTest(std::string& url);

	//文件广播测试
	void FileBroadCastTest(std::string& url);

	void PostUrlTest();

private:
	CSplitScreenWindow* m_pSplitScreenWindow;
	CTreeCtrl m_deviceTree;
	CImageList m_ImageList;

	CStatic m_ipLab;
	CEdit m_ipEdit;//ip输入框
	CStatic m_portLab;
	CEdit m_portEdit;//port输入框

	CButton m_loginBtn;
	CStatic m_accountLab;
	CEdit m_accountEdit; //账号输入框
	CStatic m_passwordLab;
	CEdit m_passwordEdit; //密码输入框
	CStatic m_appStatusLab;

	CEdit m_verification;  //验证码输入框

	CComboBox m_splitWndSelectComboBox;
	CStatic m_streamTypeLab;
	CComboBox m_streamTypeComboBox; //码流类型
	CButton m_mediaInfoBtn;  //媒体信息
	CButton m_audioTalkBtn;  //语音对讲

	CButton m_dialogTypeCheckBtn;  //窗口类型选择按钮 
	//BOOL m_bDialogTypeChecked;  //窗口类型是否选中

	CDateTimeCtrl m_recordBeginTime;  //查询录像开始时间
	CDateTimeCtrl m_recordEndTime;    //查询录像结束时间
	CComboBox m_recordTypeComboBox;
	CButton m_recordSearchBtn;  //录像查询按钮
	CListCtrl m_recordList;  //录像列表

	CStatic m_recordDevName;
	CComboBox m_recordScaleComboBox;  //录像倍速
	CStatic m_recordBeginTimeLab;
	CStatic m_recordPlayTimeLab;
	CSliderCtrl m_recordPlaySlider;
	CStatic m_recordEndTimeLab;
	CButton m_selectRecordSubsection;

	CString m_strCurtRecordCameraID;  //当前录像设备ID

	CButton m_multiPlayBtn;  //拼接流启动/关闭 按钮
	CEdit m_multiPlayUrlEdit;  //拼接流播放地址输入框

	CButton m_cycleBtn;		//轮巡按钮
	bool m_bIsCycling;		
	Timer m_cycleTimer;

	int m_iCurtRecordPlatType;  //当前录像平台类型
	int m_iCurtRecordIndex;   //当前录像序号

	int m_iClientWidth;
	int m_iClientHeight;

	COleDateTime m_curtSubsectionBeginTime;
	COleDateTime m_curtSubsectionEndTime;


	//CString m_strLoginedAccount;
	//CString m_strLoginedPassword;
	//CString m_strLoginedIP;
	//CString m_strLoginedPort;
	//CString m_strLoginedToken;

	list<DeviceTreeNodeData> m_treeNodeDataList;
	vector<RecordNodeData> m_recordSubsectionList;  //录像分段列表
	CString m_strCurtAudioTalkCameraID = L"";
	long m_iCurtBusinessID;
	long m_iCurtMultiBusinessID;

public:
	void InitClientView();

	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSplitWndSelectChange();
	afx_msg void OnLoginBtnClicked();
	afx_msg void OnDialogTypeChecked();
	afx_msg void OnRecordSearchBtnClicked();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnRecordScaleSelectChange();

	DECLARE_MESSAGE_MAP()

private:

	//CTreeCtrl m_deviceTree;
	//CSplitScreenWindow* m_pSplitScreenWindow;
	//CComboBox m_splitWndSelectComboBox;

	//CStatic m_ipLab;
	//CEdit m_ipEdit;//ip输入框
	//CStatic m_portLab;
	//CEdit m_portEdit;//port输入框

	//CButton m_loginBtn;
	//CStatic m_accountLab;
	//CEdit m_accountEdit; //账号输入框
	//CStatic m_passwordLab;
	//CEdit m_passwordEdit; //密码输入框
	//CStatic m_appStatusLab;

public:
	//afx_msg void OnEnChangeEditUrl();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMDblclkDeviceTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonSelectRecordSubsection();
	afx_msg void OnNMReleasedcaptureSliderRecord(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnTRBNThumbPosChangingSliderRecord(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnPlayDataMsg(WPARAM, LPARAM);  //进度回调
	afx_msg LRESULT OnMultiPlayMsg(WPARAM, LPARAM);  //拼接流回调
	afx_msg void OnBnClickedButtonMediaInfo();
	afx_msg void OnBnClickedButtonAudioTalk();
	afx_msg void OnStnClickedStaticVerificationShow();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonRefreash();
	afx_msg void OnCbnSelchangeComboStreamType();
	afx_msg void OnBnClickedButtonMultiPlay();
	afx_msg void OnBnClickedButtonCycle();
};
