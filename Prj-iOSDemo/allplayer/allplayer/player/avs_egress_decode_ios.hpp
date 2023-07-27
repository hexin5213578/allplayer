//
//  avs_egress_decode_ios.hpp
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/26.
//

#ifndef avs_egress_decode_ios_hpp
#define avs_egress_decode_ios_hpp

#include <stdio.h>
#include "avs_egress.h"
#include "avs_allplayer_common_ios.h"
//#include "PJGLKView.h"
class avs_egress_decode_ios:public avs_egress{
public:
    avs_egress_decode_ios();
    virtual ~avs_egress_decode_ios();
    int init(AVCodecID codeId);
    virtual int32_t init();
    virtual void    release();
    virtual int32_t start_egress();
    virtual int32_t insert_video_data(char* pData,uint32_t len);
    virtual int32_t insert_audio_data(char* pData, uint32_t len);
    
public:
    int32_t put_video_stream(char* buffer,int32_t bufferLen);
    int32_t get_next_video_frame(int32_t width,int32_t height);
    bool reset_rgb_scale(int32_t width,int32_t height);
    
private:
    AVPacket* m_avpkt;
    AVCodec* m_VideoDecoder;
    AVCodecParserContext* m_parser;
    AVCodecContext* m_pCodecContext;
    AVFrame* m_pFrameYuv;
    
    int32_t m_videolength;
    char* m_videobuf;
    char* m_audiobuf;
    int32_t m_audiolength;
    int32_t m_picWidth;
    int32_t m_picHeight;
    int32_t m_rgbWidth;
    int32_t m_rgbHeight;
    struct SwsContext* m_img_convert_ctx;
    int32_t AlgorithmFlag;
    
    uint8_t* m_audio_chunk;
    uint8_t* m_audio_pos;
    uint32_t m_audio_len;
    int32_t m_volume;
    
    int32_t m_outputlinesize[4];
    uint8_t* m_outputdate[4];
    
    
public:
   // PJGLKView *m_glkView;

};

#endif /* avs_egress_decode_ios_hpp */
