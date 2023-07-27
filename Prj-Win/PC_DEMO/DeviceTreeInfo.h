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

	//解析json中该节点下的数据
	void parseJsonData(cJSON* childrenJson);

	//设置节点id
	void setNodeId(CString strNodeId) { m_strNodeId = strNodeId; }
	CString getNodeId() const { return m_strNodeId; }

	//设置节点名称
	void setNodeName(CString strNodeName) { m_strNodeName = strNodeName; }
	CString getNodeName() const { return m_strNodeName; }

	//设置节点类型
	void setNodeType(CString strNodeType) { m_strNodeType = strNodeType; }
	CString getNodeType() const { return m_strNodeType; }

	//设置父节点Id
	void setParentNodeId(CString strParentId) { m_strParentNodeId = strParentId; }
	CString getParentNodeId() const { return m_strParentNodeId; }

	//设置层级
	void setLevel(int iLevel) { m_iLevel = iLevel; }
	int getLevel() const { return m_iLevel; }

	//设置总数
	void setTotalCount(int iTotal) { m_iTotalCount = iTotal; }
	int getTotalCount() const { return m_iTotalCount; }

	//设置在线数
	void setOnlineCount(int iCount) { m_iOnlineCount = iCount; }
	int getOnlineCount() const { return m_iOnlineCount; }

	//设置设备在线状态
	void setDevOnlineStatus(CString iStatus) { m_strDevOnlineStatus = iStatus; }
	CString getDevOnlineStatus() const { return m_strDevOnlineStatus; }

	//获取子节点列表
	list<DeviceTreeNodeData> getChildNodeList() const { 
		//排序 分组 在线 离线
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

	//添加子节点
	void addChildNodeData(DeviceTreeNodeData nodeData) 
	{ 
		m_childNodeList.push_back(nodeData);
	}

private:
	CString m_strNodeId; //节点ID
	CString m_strNodeName;  //节点名称
	CString m_strNodeType;  //节点类型 1-分组, 2-设备
	CString m_strParentNodeId;  //父节点Id
	int m_iLevel;  //节点所在层级
	int m_iTotalCount;  //总数
	int m_iOnlineCount;  //在线数
	CString m_strDevOnlineStatus;  //设备在线状态 1 - 在线 2 - 离线

	list<DeviceTreeNodeData> m_childNodeList;  //分组节点的子节点列表
};