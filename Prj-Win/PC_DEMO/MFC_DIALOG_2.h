
// MFC_DIALOG_2.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CMFC_DIALOG_2App: 
// �йش����ʵ�֣������ MFC_DIALOG_2.cpp
//

class CMFC_DIALOG_2App : public CWinApp
{
public:
	CMFC_DIALOG_2App();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CMFC_DIALOG_2App theApp;