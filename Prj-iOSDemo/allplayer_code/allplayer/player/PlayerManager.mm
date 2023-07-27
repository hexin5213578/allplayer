//
//  PlayerManager.m
//  allplayer
//
//  Created by ZC1AE6-4501-B15A on 2021/3/23.
//

#import "PlayerManager.h"

#include "avs_player.h"
#include "as_log.h"

static void lib_mk_log(const char* szFileName,int32_t lLine,int32_t lLevel,const char* format,va_list argp){
    char buf[1024];
    (void)::vsnprintf(buf, 1024, format, argp);
    buf[1023]='\0';
    printf("%s:%d%s\n",szFileName,lLine,buf);
    
}
@implementation PlayerManager
+ (instancetype)sharedPlayerManager
{
    /*! 为单例对象创建的静态实例，置为nil，因为对象的唯一性，必须是static类型 */
    static id sharedPlayerManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedPlayerManager = [[super allocWithZone:NULL] init];
    });
    return sharedPlayerManager;
}
+ (instancetype)allocWithZone:(struct _NSZone *)zone
{
    return [self sharedPlayerManager];
}
+ (id)copyWithZone:(struct _NSZone *)zone
{
    return [self sharedPlayerManager];
}
- (id)copy
{
    NSLog(@"这是一个单例对象，copy将不起任何作用");
    return self;
}
-(OpenGLView20*)intPlayerView:(CGFloat)playerWidth{
    self.playerWidth = playerWidth;
    return  self.playerView1;
}

-(void)preparePlayer:(NSString*)url{
    pavc_player = new avs_player();
   string rtspUrl = "rtsp://172.16.20.252/LiveMedia/ch1/Media1";
    //string rtspUrl = "rtmp://58.200.131.2:1935/livetv/hunantv";
   // string rtspUrl = "http://allcam-app.oss-cn-hangzhou.aliyuncs.com/b.mp4";

  
    AVCodecID avcodeId = AV_CODEC_ID_H264;
    
    
    if (0!= mk_lib_init(2, lib_mk_log, 600, 100)) {
        AS_LOG(AS_LOG_ERROR,"mk");
     //   return AS_ERROR_CODE_FAIL;
    }
    //ffmpge 初始化
    avdevice_register_all();
    
    m_avsrender.m_bussinesType = AVS_REALTIME_PLAY;
    memcpy(m_avsrender.m_url, rtspUrl.c_str(), URL_LEN_MAX);
    m_avsrender.m_codeId = avcodeId;
    
    m_avsrender.func_init = func_init;
    m_avsrender.func_release = func_release;
    m_avsrender.func_snap_picture = func_snap_picture;
    m_avsrender.func_voice_control = func_voice_control;
    m_avsrender.func_start_egress = func_start_egress;
    m_avsrender.func_insert_audio_data = func_insert_audio_data;
    m_avsrender.func_insert_video_data = func_insert_video_data;
    m_avsrender.func_decodedData = func_decodedData;
    
    int32_t iRes = pavc_player->init(m_avsrender);
    if (AS_ERROR_CODE_FAIL == iRes) {
        AS_LOG(AS_LOG_ERROR,"avs_player init faile");
    }
    iRes = pavc_player->play();
    if (AS_ERROR_CODE_OK != iRes ) {
        AS_LOG(AS_LOG_ERROR,"avs_player init faile");
    }
//    while (1) {
//    }
}
-(PJGLKView*)playerView{
    if (!_playerView) {
        _playerView = [[PJGLKView alloc] initWithFrame:CGRectMake(0, 20, self.playerWidth, self.playerWidth/16*9)];
    }
    return _playerView;
}
-(OpenGLView20*)playerView1{
    if (!_playerView1) {
        _playerView1 = [[OpenGLView20 alloc] initWithFrame:CGRectMake(0, 20, self.playerWidth, self.playerWidth/16*9)];
        _playerView1.backgroundColor = [UIColor redColor];
    }
    return _playerView1;
}

 int32_t func_init(avs_render*render){
     PlayerManager * playerM = [PlayerManager sharedPlayerManager];
     return  [playerM playerInit];
}

-(int32_t)playerInit{
    m_avsdecode = new avs_egress_decode_ios();
    return   m_avsdecode->init(m_avsrender,m_avsrender.m_codeId,self.playerWidth,self.playerWidth/16*9);;
}

void func_release(avs_render* render){
    PlayerManager * playerM = [PlayerManager sharedPlayerManager];
    [playerM decoderelease:render];
}
-(void)decoderelease:(avs_render*)render{
    m_avsdecode->release();
}
int32_t func_snap_picture(avs_render*render){
    NSLog(@"func_snap_picture");
    return 0;
}
void func_voice_control(avs_render*render){
    NSLog(@"func_voice_control");
}
int32_t func_start_egress(avs_render*render){
    NSLog(@"func_start_egress");
    PlayerManager * playerM = [PlayerManager sharedPlayerManager];
    [playerM start_egress:render];
    return 0;
}

-(int32_t)start_egress:(avs_render*)render{
    return m_avsdecode->start_egress();
}
int32_t func_insert_video_data(avs_render*render, char* pData, uint32_t len){
    PlayerManager * playerM = [PlayerManager sharedPlayerManager];
    return [playerM insertVideoData:pData length:len];
}
-(int32_t)insertVideoData:(char*)pData length:(uint32_t)len
{
    return  m_avsdecode->insert_video_data(pData, len);
}
int32_t func_insert_audio_data(avs_render*render, char* pData, uint32_t len){
    PlayerManager * playerM = [PlayerManager sharedPlayerManager];
    return [playerM insertAudioData:pData length:len];
}
-(int32_t)insertAudioData:(char*)pData length:(uint32_t)len{
    
    return  m_avsdecode->insert_audio_data(pData, len);
}

int32_t func_decodedData(AVFrame* pFrameYuv){
    NSLog(@"解码完成,开始渲染");
    PlayerManager * playerM = [PlayerManager sharedPlayerManager];
    return [playerM decodeData:pFrameYuv];
}
-(int32_t)decodeData:(AVFrame*)pFrame{
    NSLog(@"解码完成,开始渲染");
    char *buf = (char *)malloc(pFrame->width * pFrame->height * 3 / 2);

    AVPicture *pict;
    int w, h;
    char *y, *u, *v;
    pict = (AVPicture *)pFrame;//这里的frame就是解码出来的AVFrame
    w = pFrame->width;
    h = pFrame->height;
    y = buf;
    u = y + w * h;
    v = u + w * h / 4;
    
    for (int i=0; i<h; i++)
        memcpy(y + w * i, pict->data[0] + pict->linesize[0] * i, w);
    for (int i=0; i<h/2; i++)
        memcpy(u + w / 2 * i, pict->data[1] + pict->linesize[1] * i, w / 2);
    for (int i=0; i<h/2; i++)
        memcpy(v + w / 2 * i, pict->data[2] + pict->linesize[2] * i, w / 2);
    
    [self.playerView1 setVideoSize:pFrame->width height:pFrame->height];
    [self.playerView1 displayYUV420pData:buf width:pFrame->width height:pFrame->height];
    free(buf);
    
    return 0;
}
@end
