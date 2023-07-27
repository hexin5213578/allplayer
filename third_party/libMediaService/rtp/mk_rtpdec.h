#pragma once

#include "mk_rtsp_connection.h"

typedef struct PayloadContext PayloadContext;

class RTPDynamicHandler
{
public:
	RTPDynamicHandler(MK_Format_Contex* s, int st_index);
	virtual ~RTPDynamicHandler();
	
	int init(PayloadContext* priv_data)

private:

	enum MK_MediaType	m_codecType;
	enum MKCodecID		m_codecId;
	enum MKStreamParseType m_needParsing;
	int static_payload_id; // 0 means no payload id is set.
	int priv_data_size;
};