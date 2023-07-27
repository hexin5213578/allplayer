//
//  AllcamApi.h
//  AllcamSDK
//
//  Created by 秦骏 on 2021/11/19.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 接口
@interface AllcamApi : NSObject

/** 查询用户设备树
 @param type 查询类型：默认为1 1:懒加载查询 2:查询全部
 @param parentId 父ID，懒加载查询下级节点时必填
 @param subUserId 子用户id
 @param deviceType 查询设备类型，1为摄像头，其他根据业务字典定义 不填或填0为查询全部
 @param success 查询成功回调
 nodeList =(树节点列表
         {
          id = 节点ID;
          label = 节点名称;
          leaf = 是否为叶子节点，仅在指定了 lazy 属性的情况下生效;
          type = 节点类型 1-组；2-设备;
          payload = 节点对应组织信息或者设备信息 当节点类型为：1-组时，payload为组织信息；当节点类型为：2-设备时，payload为设备信息；
                   {
                    areaCode = 000000000000;
                    belongTenantId = 所属租户的id;
                     createTime = "2020-09-09 14:36:00";
                     mapType = 映射类型 1-非映射组；2-全映射组;
                     onlineCount = 下级镜头设备在线总数;
                     organizationId = 组织id;
                     organizationName = 组织名称;
                     parentId = 组织id;
                     parents = 父组id;
                     totalCount = 651;
                     type = 组织类型 1-租户（客户站点）；2-普通组;
                     updateTime = "2020-09-09 14:36:00";
                    };
         },
         {
           id = 节点ID;
           label = 节点名称;
           type =  节点类型 1-组；2-设备;
           leaf = 是否为叶子节点，仅在指定了 lazy 属性的情况下生效;
           payload =  节点对应组织信息或者设备信息 当节点类型为：1-组时，payload为组织信息；当节点类型为：2-设备时，payload为设备信息；
                   {
                      belongTenantId = 所属租户id;
                      deviceId = 设备id;
                      deviceName = 设备名称;
                      deviceStatus = 设备业务状态 发生故障、产生警告等;
                      deviceType = 设备类型，按照字段表定义（固定枪机，云台枪机，球机等等);
                      mainDevId = 主设备id;
                      organizationId = 所在组织id;
                      platId = 接入能力平台id;
                      status = 状态 1-在线；2-离线;
                      thirdId = 接入能力平台对应设备id;
                      type = 1;
                    };
})
 @param failure 查询失败回调
 */
+(void)getDeviceListWithType:(NSString*)type ParentId:(NSString*)parentId SubUserId:(NSString*)subUserId DeviceType:(NSString*)deviceType Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 获取设备列表(非懒加载)
 @param type 请求类型：默认为0 0：全部分组和设备 1：仅仅返回设备信息
 @param clientId 客户ID，按照指定客户ID查询，用于管理员查询设备列表
 @param countOnOffLine 是否统计分组下在线镜头数与总镜头数 0：否1：是
 @param success 获取成功回调
 @param failure 获取失败回调
 */
+(void)getDeviceListNonlazyWithType:(NSString*)type clientId:(NSString*)clientId countOnOffLine:(NSString*)countOnOffLine Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 获取实时播放地址
 @param cameraId 摄像头ID
 @param streamType 码流类型(多码流时使用) 1表示主码流 2表示子码流 3表示第三码流
 @param urlType 所请求的URL类型，默认值为1  1表示rtsp格式的URL 2表示http格式的URL  3表示rtmp格式的URL 4表示HTTP+FLV格式的URL  5表示DASH格式的URL 一般只支持1，不支持2,3需与平台研发确认
 agentType 是否走代理，默认值1   0表示不走代理（VPN用户 1表示走代理（公网用户）
 @param success 获取成功回调
 @param failure 获取失败回调
 */
+(void)getMediaLiveUrlWithCameraId:(NSString*)cameraId streamType:(NSString*)streamType urlType:(NSString*)urlType agentType:(NSString*)agentType Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;


/** 云镜控制
 @param cameraId 镜头ID
 @param opCode 控制代码名称
                    云台向上:PTZ_UP
                    云台向下:PTZ_DOWN
                    云台向左:PTZ_LEFT
                    云台左上:PTZ_UP_LEFT
                    云台左下:PTZ_DOWN_LEFT
                    云台向右:PTZ_RIGHT
                    云台右上:PTZ_UP_RIGHT
                    云台右下:PTZ_DOWN_RIGHT
                    云台停止:PTZ_STOP
 @param param1 参数1 : 1点动模式 2连续模式
 @param param2 参数2 步长(1-10)
 @param success 获取成功回调
 @param failure 获取失败回调
 */
+(void)ptzControlWithcameraId:(NSString*)cameraId opCode:(NSString*)opCode param1:(NSString*)param1 param2:(NSString*)param2 Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;
    
/** 获取录像列表
 @param cameraList 镜头id (Array)
                cameraId 镜头id     例如 "cameraId":"00000021102009010301000000019738"
 @param searchInfo 查询条件
                from 查询类型  DEVICE:从设备（IPC、DVR、NVR）上查询  PLATFORM:从平台查询
                bookMarkSearchTag 标签条件查询开关 0：不使用（默认）1：使用书签查询
                bookMarkName 标签名称（用于标签模糊查询-标签条件查询开关为1时为必填）开关为1且传空值时代表查询所有标签
                beginTime yyyy-mm-dd HH:MM:SS
                endTime yyyy-mm-dd HH:MM:SS
                eventList (Array)录像类型
                         ALARM：告警录像 MANUAL：手动录像 TIMING：定时录像（按时间查询所有录像）ALL：所有录像   例如 "event":"ALL"


 @param pageNum 分页索引从1开始  默认分页50条
 @param success 获取成功回调
 @param failure 获取失败回调
 */
+(void)getRecordListWithCameraList:(NSMutableArray*)cameraList searchInfo:(NSMutableDictionary*)searchInfo pageNum:(NSString*)pageNum Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;
/**  录像回放url
 @param vodInfo 录像信息
 @param streamType 码流类型(多码流时使用)    1表示主码流    2表示子码流   3表示第三码流
 @param urlType 所请求的URL类型，默认值为1。(目前只支持rtsp) 1表示rtsp格式的URL 2表示http格式的URL3表示rtmp格式的URL
 @param agentType 是否走代理，默认值1 0表示不走代理（VPN用户） 1表示走代理（公网用户）
 @param success 获取成功回调
 @param failure 获取成功回调*/
+(void)getRecordUrlWithVodInfo:(NSDictionary*)vodInfo streamType:(NSString*)streamType urlType:(NSString*)urlType agentType:(NSString*)agentType Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;
/** 查询告警组
 @param success 获取成功回调
 @param failure 获取失败回调
 */
+(void)queryAlarmGroupSuccess:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/**  查询告警类型
 @param canQuery 是否支持查询:0-不支持 1-支持
 @param canLinkage 是否支持联动:0-不支持 1-支持
 @param alarmGroup 告警组code
 @param success 获取成功回调
 @param failure 获取失败回调*/

+(void)queryAlarmTypeWithalarmGroup:(NSString*)alarmGroup canQuery:(NSString*)canQuery canLinkage:(NSString*)canLinkage  Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 查询告警信息列表
 @param searchInfo 查询条件
               alarmGroupType 告警类型组编码
               typeList 字符串数组，查询多个告警类型列表，最多可传10个值
               beginTime 开始时间 格式为：yyyy-mm-dd HH:MM:SS(必填) 告警查询时间一次只能查询一天内的时间，如需跨天，需要再上层应用按天逐一调用接口进行查询
               endTime 结束时间 格式为：yyyy-mm-dd HH:MM:SS(必填)
               confirmState 确认状态过滤：0：查询没有确认的告警1：查询已经确认的告警
               checkState 当confirmState为1时必填 正检误检状态：0：未设置 1：误检 2：正检
               organizationId 组织id，如果选择，则按照组织ID查找不能与devList同时查询
               alarmLevel 告警等级，定义变量定义遵循udc字典查询范围
               devList 镜头ID列表，如果不填表示查询所有拥有权限的镜头设备；限制最多10个设备，超出100个设备返回失败 不能与groupId同时查询
 @param pageNum 分页索引从1开始 默认分页50条（必填）
 @param pageSize 分页条数
 @param success 获取成功回调
 @param failure 获取失败回调
 */
+(void)queryAlarmListWithSearchInfo:(NSDictionary*)searchInfo pageNum:(NSString*)pageNum pageSize:(NSString* )pageSize Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 前端抓拍
 @param cameraId 抓拍的视频通道ID，即镜头ID （必填）
 @param snapNum 连拍张数，默认值1
 @param space 抓拍时间间隔，单位：毫秒，不传则表示不连续抓拍，只单张抓拍，默认值1000ms – 一秒钟
 @param success 获取成功回调
 @param failure 获取成功回调
 */
+(void)platSnapcameraId:(NSString*)cameraId snapNum:(NSString*)snapNum space:(NSString*)space Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 向统一监控平台发送添加预置位信息
 @param cameraId 设备ID
 @param presetIndex 预置位索引
 @param presetName 预置位名称
 @param success 获取成功回调
 @param failure 获取成功回调
 */
+(void)presetAddcameraId:(NSString*)cameraId presetIndex:(NSString*)presetIndex presetName:(NSString*)presetName Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 向统一监控平台发送修改预置位信息
 @param cameraId 设备ID
 @param presetIndex 预置位Id
 @param presetName 预置位名称
 @param success 获取成功回调
 @param failure 获取成功回调
 */
+(void)presetModcameraId:(NSString*)cameraId presetIndex:(NSString*)presetIndex presetName:(NSString*)presetName Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 向统一监控平台发送删除预置位信息
 @param cameraId 设备ID
 @param presetIndex 预置位Id
 @param success 获取成功回调
 @param failure 获取成功回调
 */
+(void)presetDelcameraId:(NSString*)cameraId presetIndex:(NSString*)presetIndex  Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/** 向统一监控平台发送查询预置位信息
 @param cameraId 设备ID
 @param success 获取成功回调
 @param failure 获取成功回调
*/
+(void)presetGetcameraId:(NSString*)cameraId  Success:(void (^)( NSDictionary* result)) success failure:(void (^)( NSDictionary *error))failure;

/**  获取语音对讲地址
 @param cameraId 摄像头ID （必填）
 @param agentType 是否走代理，默认值1   0表示不走代理（VPN用户）  1表示走代理（公网用户）
 @param success 获取成功回调
 @param failure 获取成功回调
 */
+(void)audioRtspCameraId:(NSString *)cameraId agentType:(NSString*)agentType Success:(void (^)(NSDictionary * result))success failure:(void (^)(NSDictionary * error))failure;
@end

NS_ASSUME_NONNULL_END
