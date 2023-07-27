#include "mjpeg.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_rtsp_connection.h"
#include "bytestream.h"

extern "C" {
#include "jpegtables.h"
}

#ifndef AV_RB24
#   define AV_RB24(x)                           \
    ((((const uint8_t*)(x))[0] << 16) |         \
     (((const uint8_t*)(x))[1] <<  8) |         \
      ((const uint8_t*)(x))[2])
#endif

#ifndef AV_RB16
#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#endif

#define AV_RB8(x)     (((const uint8_t*)(x))[0])

static const uint8_t default_quantizers[128] = {
    /* luma table */
    16,  11,  12,  14,  12,  10,  16,  14,
    13,  14,  18,  17,  16,  19,  24,  40,
    26,  24,  22,  22,  24,  49,  35,  37,
    29,  40,  58,  51,  61,  60,  57,  51,
    56,  55,  64,  72,  92,  78,  64,  68,
    87,  69,  55,  56,  80,  109, 81,  87,
    95,  98,  103, 104, 103, 62,  77,  113,
    121, 112, 100, 120, 92,  101, 103, 99,

    /* chroma table */
    17,  18,  18,  24,  21,  24,  47,  26,
    26,  47,  99,  66,  56,  66,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
};

static void create_default_qtables(uint8_t* qtables, uint8_t q)
{
    int factor = q;
    int i;
    uint16_t S;

    if (q <= 1) {
        factor = 1;
    }
    else if(q > 99){
        factor = 99;
    }

    if (q < 50) {
        S = 5000 / factor;
    }
    else {
        S = 200 - factor * 2;
    }

    for (i = 0; i < 128; i++) {
        int val = (default_quantizers[i] * S + 50) / 100;

        /* Limit the quantizers to 1 <= q <= 255. */
        if (val < 1) {
            val = 1;
        }
        else if(val > 255){
            val = 255;
        }
        qtables[i] = val;
    }
}

static int jpeg_create_huffman_table(PutByteContext* p, int table_class,
    int table_id, const uint8_t* bits_table,
    const uint8_t* value_table)
{
    int i, n = 0;

    bytestream2_put_byte(p, table_class << 4 | table_id);

    for (i = 1; i <= 16; i++) {
        n += bits_table[i];
        bytestream2_put_byte(p, bits_table[i]);
    }

    for (i = 0; i < n; i++) {
        bytestream2_put_byte(p, value_table[i]);
    }
    return n + 17;
}

static void jpeg_put_marker(PutByteContext* pbc, int code)
{
    bytestream2_put_byte(pbc, 0xff);
    bytestream2_put_byte(pbc, code);
}

static int jpeg_create_header(uint8_t* buf, int size, uint32_t type, uint32_t w,
    uint32_t h, const uint8_t* qtable, int nb_qtable,
    int dri)
{
    PutByteContext pbc;
    uint8_t* dht_size_ptr;
    int dht_size, i;

    bytestream2_init_writer(&pbc, buf, size);

    /* Convert from blocks to pixels. */
    w <<= 3;
    h <<= 3;

    /* SOI */
    jpeg_put_marker(&pbc, SOI);

    /* JFIF header */
    jpeg_put_marker(&pbc, APP0);
    bytestream2_put_be16(&pbc, 16);
    bytestream2_put_buffer(&pbc, (const uint8_t*)"JFIF", 5);
    bytestream2_put_be16(&pbc, 0x0201);
    bytestream2_put_byte(&pbc, 0);
    bytestream2_put_be16(&pbc, 1);
    bytestream2_put_be16(&pbc, 1);
    bytestream2_put_byte(&pbc, 0);
    bytestream2_put_byte(&pbc, 0);

    if (dri) {
        jpeg_put_marker(&pbc, DRI);
        bytestream2_put_be16(&pbc, 4);
        bytestream2_put_be16(&pbc, dri);
    }

    /* DQT */
    jpeg_put_marker(&pbc, DQT);
    bytestream2_put_be16(&pbc, 2 + nb_qtable * (1 + 64));

    for (i = 0; i < nb_qtable; i++) {
        bytestream2_put_byte(&pbc, i);

        /* Each table is an array of 64 values given in zig-zag
         * order, identical to the format used in a JFIF DQT
         * marker segment. */
        bytestream2_put_buffer(&pbc, qtable + 64 * i, 64);
    }

    /* DHT */
    jpeg_put_marker(&pbc, DHT);
    dht_size_ptr = pbc.buffer;
    bytestream2_put_be16(&pbc, 0);

    dht_size = 2;
    dht_size += jpeg_create_huffman_table(&pbc, 0, 0, avpriv_mjpeg_bits_dc_luminance,
        avpriv_mjpeg_val_dc);
    dht_size += jpeg_create_huffman_table(&pbc, 0, 1, avpriv_mjpeg_bits_dc_chrominance,
        avpriv_mjpeg_val_dc);
    dht_size += jpeg_create_huffman_table(&pbc, 1, 0, avpriv_mjpeg_bits_ac_luminance,
        avpriv_mjpeg_val_ac_luminance);
    dht_size += jpeg_create_huffman_table(&pbc, 1, 1, avpriv_mjpeg_bits_ac_chrominance,
        avpriv_mjpeg_val_ac_chrominance);
    AV_WB16(dht_size_ptr, dht_size);

    /* SOF0 */
    jpeg_put_marker(&pbc, SOF0);
    bytestream2_put_be16(&pbc, 17); /* size */
    bytestream2_put_byte(&pbc, 8); /* bits per component */
    bytestream2_put_be16(&pbc, h);
    bytestream2_put_be16(&pbc, w);
    bytestream2_put_byte(&pbc, 3); /* number of components */
    bytestream2_put_byte(&pbc, 1); /* component number */
    bytestream2_put_byte(&pbc, (2 << 4) | (type ? 2 : 1)); /* hsample/vsample */
    bytestream2_put_byte(&pbc, 0); /* matrix number */
    bytestream2_put_byte(&pbc, 2); /* component number */
    bytestream2_put_byte(&pbc, 1 << 4 | 1); /* hsample/vsample */
    bytestream2_put_byte(&pbc, nb_qtable == 2 ? 1 : 0); /* matrix number */
    bytestream2_put_byte(&pbc, 3); /* component number */
    bytestream2_put_byte(&pbc, 1 << 4 | 1); /* hsample/vsample */
    bytestream2_put_byte(&pbc, nb_qtable == 2 ? 1 : 0); /* matrix number */

    /* SOS */
    jpeg_put_marker(&pbc, SOS);
    bytestream2_put_be16(&pbc, 12);
    bytestream2_put_byte(&pbc, 3);
    bytestream2_put_byte(&pbc, 1);
    bytestream2_put_byte(&pbc, 0);
    bytestream2_put_byte(&pbc, 2);
    bytestream2_put_byte(&pbc, 17);
    bytestream2_put_byte(&pbc, 3);
    bytestream2_put_byte(&pbc, 17);
    bytestream2_put_byte(&pbc, 0);
    bytestream2_put_byte(&pbc, 63);
    bytestream2_put_byte(&pbc, 0);

    /* Return the length in bytes of the JPEG header. */
    return bytestream2_tell_p(&pbc);
}


int MJPEGTrack::handleRtpPacket(as_msg_block* block)
{
    char* buff = block->base();
    auto len = block->length();

    int ret = 0;
    mk_rtp_packet rtp;
    if ((ret = rtp.ParsePacket(buff, len)) || (ret = checkValid(rtp)))
        return ret;

    return m_organizer.insertRtpPacket(block);
}

int32_t MJPEGTrack::handleRtp(uint8_t* rtpData, uint32_t rtpLen)
{
    return AS_ERROR_CODE_OK;
}

int32_t MJPEGTrack::handleRtpList(RTP_PACK_QUEUE& rtpFrameList)
{
    if (0 == rtpFrameList.size()) {
        return AS_ERROR_CODE_FAIL;
    }

    int32_t ret = AS_ERROR_CODE_OK;
    if (m_conn->get_client_frags() > 1) {
        ret = AS_ERROR_CODE_INVALID;
    }
    else {
        ret = handleOneFrame(rtpFrameList);
    }
    return ret;
}

int32_t MJPEGTrack::handleOneFrame(RTP_PACK_QUEUE& rtpFrameList)
{
    //AS_LOG(AS_LOG_DEBUG, "handleMJPEGFrame rtp packet list:[%d].",rtpFrameList.size());
    mk_rtp_packet rtpPacket;
    uint32_t      rtpLen, rtpHeadLen, rtpPayloadLen, timeStam;
    char* rtpData;

    uint32_t bufflen, ulTotaldatalen = checkFrameTotalDataLen(rtpFrameList);
    char* recBuf = m_conn->handle_connection_databuf(ulTotaldatalen, bufflen);
    if (!recBuf) 
    {
        AS_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_MEM;
    }
   
    uint8_t type, q, width, height;
    const uint8_t* qtables = nullptr;
    uint16_t restartInterval = 0, restartCount = 0, qtable_len;
    uint32_t off, m_ulRecvLen = 0;
    int dri = 0;

    for (uint32_t i = 0; i < rtpFrameList.size(); i++) 
    {
        rtpData = rtpFrameList[i].pRtpMsgBlock->base();
        rtpLen = rtpFrameList[i].pRtpMsgBlock->length();
        if (rtpPacket.ParsePacket(rtpData, rtpLen)) 
        {
            AS_LOG(AS_LOG_ERROR, "handleMJPEGFrame frame list, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }
        rtpHeadLen = rtpPacket.GetHeadLen();
        timeStam = rtpPacket.GetTimeStamp();
        rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();

        if (rtpPayloadLen < 8) 
        {
            AS_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, DataLen invalid.rtpLen[%d]", rtpPayloadLen);
            return AS_ERROR_CODE_INVALID;
        }

        rtpData = &rtpData[rtpHeadLen];
        /* Parse the main JPEG header. */
        off = AV_RB24(rtpData + 1);  /* fragment byte offset */
        type = AV_RB8(rtpData + 4);   /* id of jpeg decoder params */
        q = AV_RB8(rtpData + 5);   /* quantization factor (or table id) */
        width = AV_RB8(rtpData + 6);   /* frame width in 8 pixel blocks */
        height = AV_RB8(rtpData + 7);   /* frame height in 8 pixel blocks */
        rtpData += 8;
        rtpPayloadLen -= 8;

        if (type & 0x40) 
        {
            if (rtpPayloadLen < 4) 
            {
                AS_LOG(AS_LOG_ERROR, "too short RTP/JPEG packet.");
                return AS_ERROR_CODE_INVALID;
            }
            dri = AV_RB16(rtpData);
            rtpData += 4;
            rtpPayloadLen -= 4;
            type &= ~0x40;
        }

        if (type > 1) 
        {
            return AS_ERROR_CODE_INVALID;
        }

        //Restart Interval 64 - 127
        if (type > 63 && type < 128) 
        {
            restartInterval = AV_RB16(rtpData);
            rtpPayloadLen -= 4;
        }

        if (0 == off) 
        {
            /* Start of JPEG data packet. */
            uint8_t new_qtables[128];

            if (q > 127) 
            {
                uint8_t precision;
                if (rtpPayloadLen < 4) 
                {
                    AS_LOG(AS_LOG_ERROR, "Too short RTP/JPEG packet.");
                    return AS_ERROR_CODE_INVALID;
                }

                /* The first byte is reserved for future use. */
                precision = AV_RB8(rtpData + 1);    /* size of coefficients */
                qtable_len = AV_RB16(rtpData + 2);   /* length in bytes */
                rtpData += 4;
                rtpPayloadLen -= 4;

                if (precision) 
                {
                    AS_LOG(AS_LOG_WARNING, "Only 8-bit precision is supported.");
                }
                
                if (qtable_len > 0)
                {
                    if (rtpPayloadLen < qtable_len) 
                    {
                        AS_LOG(AS_LOG_ERROR, "Too short RTP/JPEG packet.");
                        return AS_ERROR_CODE_INVALID;
                    }

                    qtables = (const uint8_t*)rtpData;
                    rtpData += qtable_len;
                    rtpPayloadLen -= qtable_len;
                    if (q < 255) 
                    {
                        if (qtables_len_[q - 128] && (qtables_len_[q - 128] != qtable_len || 
                            memcmp(qtables, &qtables_[q - 128][0], qtable_len))) 
                        {
                            AS_LOG(AS_LOG_WARNING, "Quantization tables for q=%d changed.", q);
                        }
                        else if (!qtables_len_[q - 128] && qtable_len <= 128) 
                        {
                            memcpy(&qtables_[q - 128][0], qtables, qtable_len);
                            qtables_len_[q - 128] = qtable_len;
                        }
                    }
                }
                else 
                {
                    if (255 == q) 
                    {
                        AS_LOG(AS_LOG_ERROR, "Invalid RTP/JPEG packet. Quantization tables not found.");
                        return AS_ERROR_CODE_INVALID;
                    }

                    if (q - 128 >= 128) 
                    {
                        return AS_ERROR_CODE_INVALID;
                    }
                    if (!qtables_len_[q - 128]) 
                    {
                        AS_LOG(AS_LOG_ERROR, "No quantization tables known for q=%d yet.", q);
                        return AS_ERROR_CODE_INVALID;
                    }
                    qtables = &qtables_[q - 128][0];
                    qtable_len = qtables_len_[q - 128];
                }
            }
            else 
            { /* q <= 127 */
                if (0 == q || q > 99) 
                {
                    AS_LOG(AS_LOG_ERROR, "Reserved q value %d.", q);
                    return AS_ERROR_CODE_INVALID;
                }
                create_default_qtables(new_qtables, q);
                qtables = new_qtables;
                qtable_len = sizeof(new_qtables);
            }

            timeStam = rtpPacket.GetTimeStamp();
            m_ulRecvLen += jpeg_create_header((uint8_t*)recBuf, bufflen, type, width, height, qtables, qtable_len / 64, dri);

            //MK_LOG(AS_LOG_DEBUG, "##handle nalu:[%d],rtpHeadLen[%d] pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        }
        memcpy(&recBuf[m_ulRecvLen], rtpData, rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }

    uint8_t buf[2] = { 0xff, EOI };
    memcpy(&recBuf[m_ulRecvLen], buf, 2);
    m_ulRecvLen += 2;

    MediaDataInfo dataInfo;
    memset(&dataInfo, 0x0, sizeof(dataInfo));
    packMediaInfo(dataInfo, 1, timeStam);
    m_conn->handle_connection_media(&dataInfo, m_ulRecvLen);
    return AS_ERROR_CODE_OK;
}
