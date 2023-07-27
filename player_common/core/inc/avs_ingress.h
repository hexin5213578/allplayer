#ifndef __AVS_INGRESS_H__
#define __AVS_INGRESS_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "avs_player_common.h"

class avs_ingress_data
{
public:
    avs_ingress_data(){};
    virtual ~avs_ingress_data(){};

    virtual int32_t handle_ingress_status(MEDIA_STATUS_INFO status) = 0;
    virtual char* alloc_ingress_data_buf(uint32_t len,uint32_t& ulBufLen) = 0;
	virtual int32_t handle_ingress_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len) = 0;
};

class avs_ingress
{
public:
    avs_ingress() = default;
    virtual ~avs_ingress() = default;
    
    virtual int32_t init(std::string strUrl,avs_ingress_data* ingressData);
    virtual void    release() { return; }
    
    virtual int32_t start_ingress(int16_t fragments = 1,bool bAudio = false) = 0;
    virtual void    stop_ingress() = 0;	
    virtual void    pause_ingress() = 0;
	
    virtual int32_t vcr_control(double dropPos,double scale) = 0;
    
    virtual int32_t get_stream_stat(RTP_PACKET_STAT_INFO& stat) = 0;
   
    virtual MK_Format_Contex* get_format_context() {  
        return nullptr;
    }

protected:
    avs_ingress_data*   m_ingreeData = nullptr;
	std::string         m_url;
};

class avs_ingress_prober {
public:
    static std::string probe_url(const std::string& url);
};

#endif /* __AVS_INGRESS_H__ */
