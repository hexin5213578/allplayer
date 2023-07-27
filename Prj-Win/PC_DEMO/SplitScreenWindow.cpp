// SplitScreenWindow.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC_DIALOG_2.h"
#include "SplitScreenWindow.h"
#include "afxdialogex.h"

#include "CommonDefine.h"

extern int LEFT_ACTION_BAR_WIDTH;

// CSplitScreenWindow 对话框

IMPLEMENT_DYNAMIC(CSplitScreenWindow, CDialogEx)

CSplitScreenWindow::CSplitScreenWindow(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSplitScreenWindow::IDD, pParent)
{
	m_iCurtSplitType = SPLIT_WND_TYPE_ONE;
	//m_iCurtSplitType = SPLIT_WND_TYPE_FOUR;
	m_dlgBrush = CreateSolidBrush(RGB(216, 191, 216));
}

CSplitScreenWindow::~CSplitScreenWindow()
{

}

void CSplitScreenWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSplitScreenWindow, CDialogEx)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CSplitScreenWindow::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CreateVideoPlayers();

	return TRUE;
}

void CSplitScreenWindow::InitSplitScreenView()
{
	ArrangeVideoPlayers();
}

void CSplitScreenWindow::CreateVideoPlayers()
{
	for (int iIndex = 0; iIndex < SPLIT_WND_MAX_NUM; ++iIndex)
	{
		VideoPlayer* pVideoPlayer = new VideoPlayer(iIndex, this);
		pVideoPlayer->Create(IDD_VIDEOPLAYER, this);
		m_pVideoPlayerArr[iIndex] = pVideoPlayer;
		if (0 == iIndex)
		{
			pVideoPlayer->SetPlayerSelectStatus(true);
		}
	}
}

void CSplitScreenWindow::SetSplitType(int iType)
{
	m_iCurtSplitType = iType;
	ArrangeVideoPlayers();

	//重新确认当前选中窗口
	if (m_iCurtPlayerIndex >= sqrt((double)m_iCurtSplitType))
	{
		m_iCurtPlayerIndex = 0;
		for (int i = 0; i < SPLIT_WND_MAX_NUM; ++i)
		{
			if (i == m_iCurtPlayerIndex)
			{
				m_pVideoPlayerArr[i]->SetPlayerSelectStatus(true);
			}
			else
			{
				m_pVideoPlayerArr[i]->SetPlayerSelectStatus(false);
			}
		}
	}

}

void CSplitScreenWindow::ArrangeVideoPlayers()
{
	if (m_iCurtSplitType > SPLIT_WND_MAX_NUM)
	{
		return;
	}

	for (int iIndex = 0; iIndex < SPLIT_WND_MAX_NUM; ++iIndex)
	{
		if (NULL != m_pVideoPlayerArr[iIndex])
		{
			m_pVideoPlayerArr[iIndex]->ShowWindow(SW_HIDE);
		}
	}

	int iColRowNum = sqrt((double)m_iCurtSplitType);

	CRect rect1;
	GetClientRect(&rect1);

	CRect rect;
	rect.SetRect(rect1.left + LEFT_ACTION_BAR_WIDTH, rect1.top, rect1.right, rect1.bottom);

	int iSplitWindowWidth = rect.Width() - 2 * SPLIT_WINDOW_BORDER_WIDTH - (iColRowNum - 1) * SPLIT_GRID_INTERVAL;
	int iSplitWindowHeight = rect.Height() - 2 * SPLIT_WINDOW_BORDER_WIDTH - (iColRowNum - 1) * SPLIT_GRID_INTERVAL;


	//计算单窗口宽高
	int iPlayerWidth = (double)iSplitWindowWidth / iColRowNum;
	int iPlayerHeight = (double)iSplitWindowHeight / iColRowNum;


	for (int iIndex = 0; iIndex < m_iCurtSplitType; ++iIndex)
	{
		int iLeftTopX = iPlayerWidth * (iIndex % iColRowNum) + (iIndex % iColRowNum) * SPLIT_GRID_INTERVAL + SPLIT_WINDOW_BORDER_WIDTH;
		int iLeftTopY = iPlayerHeight * (iIndex / iColRowNum) + (iIndex / iColRowNum) * SPLIT_GRID_INTERVAL + SPLIT_WINDOW_BORDER_WIDTH;
		m_pVideoPlayerArr[iIndex]->MoveWindow(iLeftTopX, iLeftTopY, iPlayerWidth, iPlayerHeight);
		m_pVideoPlayerArr[iIndex]->InitPlayerView();
		m_pVideoPlayerArr[iIndex]->ShowWindow(SW_SHOW);
	}
}

// CSplitScreenWindow 消息处理程序


HBRUSH CSplitScreenWindow::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	pDC->SetBkMode(TRANSPARENT);
	return m_dlgBrush;
}

void CSplitScreenWindow::FreshSelectStatus(int iIndex)
{
	m_iCurtPlayerIndex = iIndex;
	for (int i = 0; i < SPLIT_WND_MAX_NUM; ++i)
	{
		if (i != iIndex && m_pVideoPlayerArr[i])
		{
			m_pVideoPlayerArr[i]->SetPlayerSelectStatus(false);
		}
	}
}

void CSplitScreenWindow::StopAllVideo(bool bRecord)
{
	for (int i = 0; i < SPLIT_WND_MAX_NUM; ++i)
	{
		if (bRecord)
		{
			m_pVideoPlayerArr[i]->StopPlayRecord();
		}
		else
		{
			m_pVideoPlayerArr[i]->StopPlayLive();
		}

	}
}

void CSplitScreenWindow::StartPlayLive(CString strCameraID, int iStreamType, CString strCameraName)
{
	//获取当前选中窗口 如果选中窗口在播放，则查找空闲窗口 （目前只按照选中窗口进行处理）
	int iCurtWindowCnt = m_iCurtSplitType;
	for (int i = 0; i < SPLIT_WND_MAX_NUM && i < iCurtWindowCnt; ++i)
	{
		if (m_pVideoPlayerArr[i]->GetIsSelectStatus())
		{
			m_pVideoPlayerArr[i]->StartPlayLive(strCameraID, iStreamType, strCameraName);

			//网关测试
			//m_pVideoPlayerArr[i]->StartPlayLiveOnGateWay(strCameraID, iStreamType, strCameraName);
			break;
		}
	}

}

void CSplitScreenWindow::RecordVCRControl(double dCurtPos)
{
	int iCurtWindowCnt = m_iCurtSplitType;
	for (int i = 0; i < SPLIT_WND_MAX_NUM && i < iCurtWindowCnt; ++i)
	{
		if (m_pVideoPlayerArr[i]->GetIsSelectStatus())
		{
			m_pVideoPlayerArr[i]->RecordVCRControl(dCurtPos);
			break;
		}
	}
}

void CSplitScreenWindow::GetMediaInfo()
{
	int iCurtWindowCnt = m_iCurtSplitType;
	for (int i = 0; i < SPLIT_WND_MAX_NUM && i < iCurtWindowCnt; ++i)
	{
		if (m_pVideoPlayerArr[i]->GetIsSelectStatus())
		{
			m_pVideoPlayerArr[i]->GetMediaInfo();
			break;
		}
	}
}

std::vector<VideoPlayer*> CSplitScreenWindow::getMultiPlayWindows()
{
	std::vector<VideoPlayer*> playerVec;
	for (int iIndex = 0; (iIndex < SPLIT_WND_MAX_NUM) && (iIndex < m_iCurtSplitType); ++iIndex) {

		VideoPlayer* curtPlayer = m_pVideoPlayerArr[iIndex];
		if (curtPlayer->GetIsMultiPlayer()) {
			playerVec.push_back(curtPlayer);
		}
	}
	return playerVec;
}

VideoPlayer* CSplitScreenWindow::getFirstPlayWindow()
{
	return m_pVideoPlayerArr[0];
}

void CSplitScreenWindow::switchUrlInCycling()
{
	for (int iIndex = 0; (iIndex < SPLIT_WND_MAX_NUM) && (iIndex < m_iCurtSplitType); ++iIndex) {
		VideoPlayer* curtPlayer = m_pVideoPlayerArr[iIndex];
		if (curtPlayer && curtPlayer->isLivePlaying()) {
			curtPlayer->switchUrlInCycling();
		}

		////方便排查泄漏
		//if(curtPlayer) {
		//	if (m_playing)
		//		curtPlayer->OnBnClickedButtonStop();
		//	else
		//		curtPlayer->OnBnClickedButtonPlay();
		//}
	}
}
