// MyTestDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC_DIALOG_2.h"
#include "MyTestDialog.h"
#include "afxdialogex.h"


// MyTestDialog 对话框

IMPLEMENT_DYNAMIC(MyTestDialog, CDialogEx)

MyTestDialog::MyTestDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(MyTestDialog::IDD, pParent)
{

}

MyTestDialog::~MyTestDialog()
{
}

void MyTestDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(MyTestDialog, CDialogEx)
END_MESSAGE_MAP()


// MyTestDialog 消息处理程序
