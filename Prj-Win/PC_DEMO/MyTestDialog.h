#pragma once


// MyTestDialog �Ի���

class MyTestDialog : public CDialogEx
{
	DECLARE_DYNAMIC(MyTestDialog)

public:
	MyTestDialog(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~MyTestDialog();

// �Ի�������
	enum { IDD = IDD_MYTESTDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
