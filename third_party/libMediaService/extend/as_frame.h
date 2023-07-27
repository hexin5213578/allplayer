#pragma once 
#include <memory>
#include <string>

enum MK_MediaType {
    MEDIA_TYPE_UNKNOWN = -1,  ///< Usually treated as AVMEDIA_TYPE_DATA
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_DATA,          ///< Opaque data information usually continuous
    MEDIA_TYPE_SUBTITLE,
    MEDIA_TYPE_ATTACHMENT,    ///< Opaque data information usually sparse
    MEDIA_TYPE_MAX
};

enum MKCodecID {
    MK_CODEC_ID_NONE = -1,
    MK_CODEC_ID_MJPEG = 7,
    MK_CODEC_ID_H264 = 27,
    MK_CODEC_ID_HEVC = 173,
    MK_CODEC_ID_PCM_MULAW = 65542,
    MK_CODEC_ID_PCM_ALAW = 65543,
    MK_CODEC_ID_AAC = 86018,
};

enum MR_MEDIA_ENCODE_TYPE {
    MR_MEDIA_ENCODE_TYPE_H264 = 0,
    MR_MEDIA_ENCODE_TYPE_H265 = 1,
    MR_MEDIA_ENCODE_TYPE_G711A = 2,
    MR_MEDIA_ENCODE_TYPE_G711U = 3,
    MR_MEDIA_ENCODE_TYPE_AAC = 4,
    MR_MEDIA_ENCODE_TYPE_DATA = 5,
    MR_MEDIA_ENCODE_TYPE_INVALID
};

/**
 * ��ȡ����Ƶ����
 */
MK_MediaType getMediaType1(MKCodecID codecId);

/**
 * ������Ϣ�ĳ���ӿ�
 */
class CodecInfo {
public:
    using Ptr = std::shared_ptr<CodecInfo>;

    CodecInfo() = default;
    virtual ~CodecInfo() = default;

    /**
     * ��ȡ�����������
     */
    virtual MKCodecID getCodecId() const = 0;

    /**
     * ��ȡ����Ƶ����
     */
    MK_MediaType getMediaType() const;
};

class ASFrame : public CodecInfo{
public:
	using Ptr = std::shared_ptr<ASFrame>;

    MKCodecID getCodecId() const override { return m_codecId;  }

    /**
    * ���ؽ���ʱ�������λ����
    */
    virtual uint64_t dts() const { return m_dts; }

    /**
     * ������ʾʱ�������λ����
     */
    virtual uint64_t pts() const { return dts(); }

    ///**
    // * ǰ׺���ȣ�Ʃ��264ǰ׺Ϊ0x00 00 00 01,��ôǰ׺���Ⱦ���4
    // * aacǰ׺��Ϊ7���ֽ�
    // */
    //virtual size_t prefixSize() const = 0;

    ///**
    // * �����Ƿ�Ϊ�ؼ�֡
    // */
    //virtual bool keyFrame() const = 0;

    ///**
    // * �Ƿ�Ϊ����֡��Ʃ��sps pps vps
    // */
    //virtual bool configFrame() const = 0;

    /**
     * �Ƿ���Ի���
     */
    virtual bool cacheAble() const { return true; }

    /**
     * ��֡�Ƿ���Զ���
     * SEI/AUD֡���Զ���
     * Ĭ�϶����ܶ�֡
     */
    virtual bool dropAble() const { return false; }

    /**
   * �Ƿ�Ϊ�ɽ���֡
   * sps pps��֡���ܽ���
   */
    //virtual bool decodeAble() const {
    //    if (getMediaType() != MEDIA_TYPE_VIDEO) {
    //        //����Ƶ֡�����Խ���
    //        return true;
    //    }
    //    //Ĭ�Ϸ�sps pps֡�����Խ���
    //    return !configFrame();
    //}

public:
    MKCodecID  m_codecId = MK_CODEC_ID_NONE;
    uint64_t   m_dts = 0;
    uint64_t   m_pts = 0;
    size_t     m_prefix_size = 0;
    std::string     m_buffer;
    bool       m_inserted = false;
};