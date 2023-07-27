#pragma once
#include "AScJSON.h"
#include <list>
#include <afxstr.h>

#include <iostream>
using namespace std;


class DeviceTreeNodeData
{
public:
	DeviceTreeNodeData();

	~DeviceTreeNodeData();

	bool operator==(const DeviceTreeNodeData& other);

	//����json�иýڵ��µ�����
	void parseJsonData(cJSON* childrenJson);

	//���ýڵ�id
	void setNodeId(CString strNodeId) { m_strNodeId = strNodeId; }
	CString getNodeId() const { return m_strNodeId; }

	//���ýڵ�����
	void setNodeName(CString strNodeName) { m_strNodeName = strNodeName; }
	CString getNodeName() const { return m_strNodeName; }

	//���ýڵ�����
	void setNodeType(CString strNodeType) { m_strNodeType = strNodeType; }
	CString getNodeType() const { return m_strNodeType; }

	//���ø��ڵ�Id
	void setParentNodeId(CString strParentId) { m_strParentNodeId = strParentId; }
	CString getParentNodeId() const { return m_strParentNodeId; }

	//���ò㼶
	void setLevel(int iLevel) { m_iLevel = iLevel; }
	int getLevel() const { return m_iLevel; }

	//��������
	void setTotalCount(int iTotal) { m_iTotalCount = iTotal; }
	int getTotalCount() const { return m_iTotalCount; }

	//����������
	void setOnlineCount(int iCount) { m_iOnlineCount = iCount; }
	int getOnlineCount() const { return m_iOnlineCount; }

	//�����豸����״̬
	void setDevOnlineStatus(CString iStatus) { m_strDevOnlineStatus = iStatus; }
	CString getDevOnlineStatus() const { return m_strDevOnlineStatus; }

	//��ȡ�ӽڵ��б�
	list<DeviceTreeNodeData> getChildNodeList() const { 
		//���� ���� ���� ����
		list<DeviceTreeNodeData> groupList;
		list<DeviceTreeNodeData> onlineList;
		list<DeviceTreeNodeData> offlineList;

		list<DeviceTreeNodeData> tmpList;

		for each (DeviceTreeNodeData nodeDa in m_childNodeList)
		{
			if ("1" == nodeDa.getNodeType())
			{
				groupList.push_back(nodeDa);
			}
			else
			{
				if ("1" == nodeDa.getDevOnlineStatus())
				{
					onlineList.push_back(nodeDa);
				}
				else
				{
					offlineList.push_back(nodeDa);
				}
			}
		}
		
		for each (DeviceTreeNodeData nodeData in onlineList)
		{
			groupList.push_back(nodeData);
		}

		for each (DeviceTreeNodeData nodeData in offlineList)
		{
			groupList.push_back(nodeData);
		}

		return groupList;
		//return m_childNodeList; 
	}

	//����ӽڵ�
	void addChildNodeData(DeviceTreeNodeData nodeData) 
	{ 
		m_childNodeList.push_back(nodeData);
	}

private:
	CString m_strNodeId; //�ڵ�ID
	CString m_strNodeName;  //�ڵ�����
	CString m_strNodeType;  //�ڵ����� 1-����, 2-�豸
	CString m_strParentNodeId;  //���ڵ�Id
	int m_iLevel;  //�ڵ����ڲ㼶
	int m_iTotalCount;  //����
	int m_iOnlineCount;  //������
	CString m_strDevOnlineStatus;  //�豸����״̬ 1 - ���� 2 - ����

	list<DeviceTreeNodeData> m_childNodeList;  //����ڵ���ӽڵ��б�
};