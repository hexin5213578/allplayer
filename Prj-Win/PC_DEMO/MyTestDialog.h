#pragma once


// MyTestDialog 对话框

class MyTestDialog : public CDialogEx
{
	DECLARE_DYNAMIC(MyTestDialog)

public:
	MyTestDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~MyTestDialog();

// 对话框数据
	enum { IDD = IDD_MYTESTDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
