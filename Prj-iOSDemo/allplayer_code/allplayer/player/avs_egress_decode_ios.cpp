//
//  avs_egress_decode_ios.cpp
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/26.
//

#include "avs_egress_decode_ios.hpp"

AVPixelFormat videoFmt_RGB24 = AV_PIX_FMT_YUV420P;

avs_egress_decode_ios::avs_egress_decode_ios(){
    m_avpkt = NULL;
    m_VideoDecoder = NULL;
    m_parser = NULL;
    m_pCodecContext = NULL;
    m_pFrameYuv = NULL;
    m_img_convert_ctx = NULL;
    m_picWidth = 0;
    m_picHeight = 0;
    m_videolength = 0;
    m_audiolength = 0;
    m_rgbWidth = 0;
    m_rgbHeight = 0;
    AlgorithmFlag = SWS_BICUBIC;
    m_videobuf = (char*)malloc(H264_FRAME_SIZE_MAX);
    if (m_videobuf == NULL) {
        AS_LOG(AS_LOG_ERROR,"h264");
    }
    memset(m_videobuf, 0, H264_FRAME_SIZE_MAX);
    
    m_audiobuf = (char*)malloc(4096);
    if (m_audiobuf == NULL) {
        AS_LOG(AS_LOG_ERROR,"H264");
    }
    memset(m_audiobuf, 0, 4096);
    
    m_audio_pos = NULL;
    m_audio_chunk = NULL;
    
    
    
}
avs_egress_decode_ios:: ~avs_egress_decode_ios(){
  
    if (NULL != m_avpkt) {
        av_packet_free(&m_avpkt);
    }
    if (NULL != m_pCodecContext) {
        avcodec_free_context(&m_pCodecContext);
        m_pCodecContext = NULL;
    }
    if (m_pFrameYuv != NULL) {
        av_frame_free(&m_pFrameYuv);
        m_pFrameYuv = NULL;
    }
    if (m_videobuf != NULL) {
        free(m_videobuf);
    }
    if (m_img_convert_ctx != NULL) {
        sws_freeContext(m_img_convert_ctx);
        m_img_convert_ctx = NULL;
    }
    av_parser_close(m_parser);
}

int avs_egress_decode_ios::init(avs_render& render,AVCodecID codeId,int width,int height){
    printf("init codeId");
    m_avsRenderStruct = render;
    m_avpkt = av_packet_alloc();
    m_VideoDecoder = avcodec_find_decoder(codeId);
  
    if (m_VideoDecoder == NULL) {
        return  AS_ERROR_CODE_FAIL;
    }
    m_parser  = av_parser_init(m_VideoDecoder->id);
    if (!m_parser) {
        return  AS_ERROR_CODE_FAIL;
    }
    m_pCodecContext = avcodec_alloc_context3(m_VideoDecoder);
    if (!m_pCodecContext) {
        return  AS_ERROR_CODE_FAIL;
    }
    
    m_pCodecContext->time_base.num = 1;
    m_pCodecContext->frame_number = 1;
    m_pCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    m_pCodecContext->bit_rate = 0;
    m_pCodecContext->time_base.den = 25;
    m_pCodecContext->color_range = AVCOL_RANGE_MPEG;
    m_pCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    
    int32_t iRte = avcodec_open2(m_pCodecContext, m_VideoDecoder, NULL);
    if (iRte >= 0) {
        m_pFrameYuv = av_frame_alloc();
    }else{
        return AS_ERROR_CODE_FAIL;
    }
    m_picWidth = width;
    m_picHeight = height;
    
    return AS_ERROR_CODE_OK;
}
int32_t avs_egress_decode_ios:: init(){
    printf("testavs_egress_decode_ios");
    return AS_ERROR_CODE_OK;
}

void avs_egress_decode_ios::release(){

}
int32_t avs_egress_decode_ios::start_egress(){
    
    int32_t iRet = put_video_stream((char*)m_videobuf,m_videolength);
    if (AS_ERROR_CODE_OK != iRet) {
        AS_LOG(AS_LOG_ERROR,"put video stream failed");
        return  AS_ERROR_CODE_FAIL;
    }
    
    iRet = get_next_video_frame(m_picWidth, m_picHeight);
    if (AS_ERROR_CODE_FAIL == iRet) {
        //
        AS_LOG(AS_LOG_ERROR,"get_next_video_frame");
    }
    
    return AS_ERROR_CODE_OK;
}
int32_t avs_egress_decode_ios::insert_video_data(char* pData,uint32_t len){
    m_videolength = len;
    if (NULL != m_videobuf) {
        memcpy(m_videobuf, pData, len);
    }
    return AS_ERROR_CODE_OK;
}
int32_t avs_egress_decode_ios::insert_audio_data(char* pData, uint32_t len){
  
    data_count += len;
//    s_audio_end = (Uint8*)pData + len;
//    s_audio_pos = (Uint8*)pData;
//    while (s_audio_pos < s_audio_end) {
//        SDL_Delay(1);
//    }
    return 0;
}
int32_t avs_egress_decode_ios::put_video_stream(char* buffer,int32_t bufferLen){
    int ret =0;
    int bOK = 0;
    uint8_t* data = (uint8_t*)buffer;
    int32_t ulSize = bufferLen;
    while (ulSize > 0) {
        ret = av_parser_parse2(m_parser, m_pCodecContext, &m_avpkt->data, &m_avpkt->size, data, ulSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE,0);
        if (ret < 0) {
            fprintf(stderr, "error while parsing");
            exit(1);
        }
        
        data += ret;
        ulSize -= ret;
        
        if (m_avpkt->size) {
            ret = avcodec_send_packet(m_pCodecContext, m_avpkt);
            if (ret < 0) {
                string strErr = av_err2str(ret);
                AS_LOG(AS_LOG_DEBUG,"send packet error ");
            }
        }
        
        av_packet_unref(m_avpkt);
        if (ret != 0) {
            continue;
        }
    }
    return bOK;
}
int32_t avs_egress_decode_ios::get_next_video_frame(int32_t width,int32_t height){
    int ret = avcodec_receive_frame(m_pCodecContext, m_pFrameYuv);
    if (ret == AVERROR(EAGAIN)|| ret == AVERROR_EOF) {
        return 0;
    }else if(ret < 0){
        fprintf(stderr, "error during decoding\n");
    }
    bool scale = reset_rgb_scale(width,height);
    if (false == scale) {
        AS_LOG(AS_LOG_ERROR,"reset rgb scale faile");
        return AS_ERROR_CODE_FAIL;
    }
    
    uint8_t* data[3];
    data[0] = m_pFrameYuv->data[0];
    data[1] = m_pFrameYuv->data[1];
    data[2] = m_pFrameYuv->data[2];

    m_avsRenderStruct.func_decodedData(m_pFrameYuv);
    ret = sws_scale(m_img_convert_ctx, (const unsigned char* const*)data, m_pFrameYuv->linesize, 0,  m_pCodecContext->height, m_outputdate, m_outputlinesize);
    return AS_ERROR_CODE_OK;
    
}
bool avs_egress_decode_ios::reset_rgb_scale(int32_t width,int32_t height){
    if (m_rgbWidth == width &&  m_rgbHeight == height ) {
        return true;
    }
    if (m_img_convert_ctx != NULL) {
        sws_freeContext(m_img_convert_ctx);
        m_img_convert_ctx = NULL;
    }
    m_rgbWidth = width;
    m_rgbHeight = height;
    
    m_img_convert_ctx = sws_getContext(m_pCodecContext->width, m_pCodecContext->height, m_pCodecContext->pix_fmt, m_rgbWidth, m_rgbHeight, videoFmt_RGB24, AlgorithmFlag, NULL, NULL, NULL);
    if (NULL == m_img_convert_ctx) {
        return false;
    }
    return true;
}

