// MyTestDialog.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "MFC_DIALOG_2.h"
#include "MyTestDialog.h"
#include "afxdialogex.h"


// MyTestDialog �Ի���

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


// MyTestDialog ��Ϣ�������
