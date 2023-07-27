#include "stdafx.h"
#include "DeviceTreeInfo.h"

DeviceTreeNodeData::DeviceTreeNodeData()
{

}

DeviceTreeNodeData::~DeviceTreeNodeData()
{

}

bool DeviceTreeNodeData::operator == (const DeviceTreeNodeData& other)
{
	if (m_strNodeId == other.getNodeId())
	{
		return true;
	}
	else
	{
		return false;
	}
}

//解析json中该节点下的数据
void DeviceTreeNodeData::parseJsonData(cJSON* nodeList)
{
	int iNodeListSize = cJSON_GetArraySize(nodeList);
	for (int iIndex = 0; iIndex < iNodeListSize; ++iIndex)
	{
		DeviceTreeNodeData nodeData;
		nodeData.setLevel(m_iLevel+1);
		nodeData.setParentNodeId(L"");
		cJSON* pNodeJson = cJSON_GetArrayItem(nodeList, iIndex);
		if (pNodeJson)
		{
			//解析节点数据
			if (cJSON_GetObjectItem(pNodeJson, "id") && cJSON_GetObjectItem(pNodeJson, "id")->valuestring)
			{
				nodeData.setNodeId(CString(cJSON_GetObjectItem(pNodeJson, "id")->valuestring));
			}

			if (cJSON_GetObjectItem(pNodeJson, "label") && cJSON_GetObjectItem(pNodeJson, "label")->valuestring)
			{
				nodeData.setNodeName(CString(cJSON_GetObjectItem(pNodeJson, "label")->valuestring));
			}

			if (cJSON_GetObjectItem(pNodeJson, "type") && cJSON_GetObjectItem(pNodeJson, "type")->valuestring)
			{
				nodeData.setNodeType(CString(cJSON_GetObjectItem(pNodeJson, "type")->valuestring));
			}

			if (cJSON_GetObjectItem(pNodeJson, "payload"))
			{
				cJSON* payloadJson = cJSON_GetObjectItem(pNodeJson, "payload");
				//if (payloadJson)
				//{
					if (cJSON_GetObjectItem(payloadJson, "organizationId"))
					{
						CString strParentId;
						strParentId.Format(_T("%d"), cJSON_GetObjectItem(payloadJson, "organizationId")->valueint);
						nodeData.setParentNodeId(strParentId);
					}

					if (cJSON_GetObjectItem(payloadJson, "status"))
					{
						CString strOnlineStatus;
						strOnlineStatus.Format(_T("%d"), cJSON_GetObjectItem(payloadJson, "status")->valueint);
						nodeData.setDevOnlineStatus(strOnlineStatus);
					}

					if (cJSON_GetObjectItem(payloadJson, "totalCount"))
					{
						nodeData.setTotalCount(cJSON_GetObjectItem(payloadJson, "totalCount")->valueint);
					}

					if (cJSON_GetObjectItem(payloadJson, "onlineCount"))
					{
						nodeData.setOnlineCount(cJSON_GetObjectItem(payloadJson, "onlineCount")->valueint);
					}
				//}
			}

			if (cJSON_GetObjectItem(pNodeJson, "children"))
			{
				nodeData.parseJsonData(cJSON_GetObjectItem(pNodeJson, "children"));
			}
			addChildNodeData(nodeData);
		}
	}
}