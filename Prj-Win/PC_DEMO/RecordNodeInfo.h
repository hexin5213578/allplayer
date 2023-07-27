#pragma once
#include "AScJSON.h"
#include <list>
#include <vector>
#include <afxstr.h>

#include <iostream>
using namespace std;

class RecordNodeData
{
public:
	RecordNodeData(){}

	~RecordNodeData(){}

	bool operator==(const RecordNodeData& other)
	{
		if ((other.getCameraId() == m_strCameraId) && (other.getBeginTime() == m_strBeginTime) && (other.getEndTime() == m_strEndTime))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	RecordNodeData& operator=(const RecordNodeData& other)
	{
		m_strCameraId = other.getCameraId();
		m_strCameraName = other.getCameraName();
		m_strBeginTime = other.getBeginTime();
		m_strEndTime = other.getEndTime();
		m_strContentId = other.getContentId();
		m_strEvent = other.getEvent();
		m_strNvrCode = other.getNvrCode();
		m_iLocation = other.getLocation();
		return *this;
	}

	//设置设备名称
	void setCameraId(CString strCameraId) { m_strCameraId = strCameraId; }
	CString getCameraId() const { return m_strCameraId; }

	void setCameraName(CString strCameraName) { m_strCameraName = strCameraName; }
	CString getCameraName() const { return m_strCameraName; }

	void setBeginTime(CString strBeginTime) { m_strBeginTime = strBeginTime; }
	CString getBeginTime() const { return m_strBeginTime; }

	void setEndTime(CString strEndTime) { m_strEndTime = strEndTime; }
	CString getEndTime() const { return m_strEndTime; }

	void setContentId(CString strContentId) { m_strContentId = strContentId; }
	CString getContentId() const { return m_strContentId; }

	void setEvent(CString strEvent) { m_strEvent = strEvent; }
	CString getEvent() const { return m_strEvent; }

	void setNvrCode(CString strNvrCode) { m_strNvrCode = strNvrCode; }
	CString getNvrCode() const { return m_strNvrCode; }

	void setLocation(int strLocation) { m_iLocation = strLocation; }
	int getLocation() const { return m_iLocation; }

private:
	CString m_strCameraId = L""; //ID
	CString m_strCameraName = L"";  //名称
	CString m_strBeginTime = L"";
	CString m_strEndTime = L""; 
	CString m_strContentId = L"";
	CString m_strEvent = L"";
	CString m_strNvrCode = L"";
	int m_iLocation = 0;
};