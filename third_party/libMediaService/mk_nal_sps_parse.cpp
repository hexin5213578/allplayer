#include "mk_nal_sps_parse.h"
#include <math.h>

#include <assert.h>

const unsigned char* m_pStart;
unsigned short m_nLength;
int m_nCurrentBit;

unsigned int ReadBit()
{
    if (m_nCurrentBit > m_nLength * 8) {
        return 0;
    }
    int nIndex = m_nCurrentBit / 8;
    int nOffset = m_nCurrentBit % 8 + 1;

    m_nCurrentBit++;
    return (m_pStart[nIndex] >> (8 - nOffset)) & 0x01;
}

unsigned int ReadBits(int n)
{
    int r = 0;
    int i;
    for (i = 0; i < n; i++)
    {
        r |= (ReadBit() << (n - i - 1));
    }
    return r;
}

unsigned int ReadExponentialGolombCode()
{
    int r = 0;
    int i = 0;

    while ((ReadBit() == 0) && (i < 32))
    {
        i++;
    }

    r = ReadBits(i);
    r += (1 << i) - 1;
    return r;
}

unsigned int ReadSE()
{
    int r = ReadExponentialGolombCode();
    if (r & 0x01)
    {
        r = (r + 1) / 2;
    }
    else
    {
        r = -(r / 2);
    }
    return r;
}

void h264_parse_sps(const unsigned char* pStart, unsigned short nLen, int& width, int& height, int& fps)
{
    m_pStart = pStart;
    m_nLength = nLen;
    m_nCurrentBit = 0;
    fps = 0;
    width = 0;
    height = 0;

    int frame_crop_left_offset = 0;
    int frame_crop_right_offset = 0;
    int frame_crop_top_offset = 0;
    int frame_crop_bottom_offset = 0;

    int profile_idc = ReadBits(8);
    int constraint_set0_flag = ReadBit();
    int constraint_set1_flag = ReadBit();
    int constraint_set2_flag = ReadBit();
    int constraint_set3_flag = ReadBit();
    int constraint_set4_flag = ReadBit();
    int constraint_set5_flag = ReadBit();
    int reserved_zero_2bits = ReadBits(2);
    int level_idc = ReadBits(8);
    int seq_parameter_set_id = ReadExponentialGolombCode();


    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118)
    {
        int chroma_format_idc = ReadExponentialGolombCode();

        if (chroma_format_idc == 3)
        {
            int residual_colour_transform_flag = ReadBit();
        }
        int bit_depth_luma_minus8 = ReadExponentialGolombCode();
        int bit_depth_chroma_minus8 = ReadExponentialGolombCode();
        int qpprime_y_zero_transform_bypass_flag = ReadBit();
        int seq_scaling_matrix_present_flag = ReadBit();

        if (seq_scaling_matrix_present_flag)
        {
            int i = 0;
            for (i = 0; i < 8; i++)
            {
                int seq_scaling_list_present_flag = ReadBit();
                if (seq_scaling_list_present_flag)
                {
                    int sizeOfScalingList = (i < 6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    int j = 0;
                    for (j = 0; j < sizeOfScalingList; j++)
                    {
                        if (nextScale != 0)
                        {
                            int delta_scale = ReadSE();
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    int log2_max_frame_num_minus4 = ReadExponentialGolombCode();
    int pic_order_cnt_type = ReadExponentialGolombCode();
    if (pic_order_cnt_type == 0)
    {
        int log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode();
    }
    else if (pic_order_cnt_type == 1)
    {
        int delta_pic_order_always_zero_flag = ReadBit();
        int offset_for_non_ref_pic = ReadSE();
        int offset_for_top_to_bottom_field = ReadSE();
        int num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode();
        int i;
        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            ReadSE();
            //sps->offset_for_ref_frame[ i ] = ReadSE();
        }
    }
    int max_num_ref_frames = ReadExponentialGolombCode();
    int gaps_in_frame_num_value_allowed_flag = ReadBit();
    int pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
    int pic_height_in_map_units_minus1 = ReadExponentialGolombCode();
    int frame_mbs_only_flag = ReadBit();
    if (!frame_mbs_only_flag)
    {
        int mb_adaptive_frame_field_flag = ReadBit();
    }
    int direct_8x8_inference_flag = ReadBit();
    int frame_cropping_flag = ReadBit();
    if (frame_cropping_flag)
    {
        frame_crop_left_offset = ReadExponentialGolombCode();
        frame_crop_right_offset = ReadExponentialGolombCode();
        frame_crop_top_offset = ReadExponentialGolombCode();
        frame_crop_bottom_offset = ReadExponentialGolombCode();
    }
    int vui_parameters_present_flag = ReadBit();
    pStart++;

    if (vui_parameters_present_flag) {
        int aspect_ratio_info_present_flag = ReadBit();
        if (aspect_ratio_info_present_flag)
        {
            int aspect_ratio_idc = ReadBits(8);
            if (aspect_ratio_idc == 255)
            {
                int sar_width = ReadBits(16);
                int sar_height = ReadBits(16);
            }
        }
        int overscan_info_present_flag = ReadBit();
        if (overscan_info_present_flag)
            int overscan_appropriate_flagu = ReadBit();
        int video_signal_type_present_flag = ReadBit();
        if (video_signal_type_present_flag)
        {
            int video_format = ReadBits(3);
            int video_full_range_flag = ReadBit();
            int colour_description_present_flag = ReadBit();
            if (colour_description_present_flag)
            {
                int colour_primaries = ReadBits(8);
                int transfer_characteristics = ReadBits(8);
                int matrix_coefficients = ReadBits(8);
            }
        }
        int chroma_loc_info_present_flag = ReadBit();
        if (chroma_loc_info_present_flag)
        {
            int chroma_sample_loc_type_top_field = ReadExponentialGolombCode();
            int chroma_sample_loc_type_bottom_field = ReadExponentialGolombCode();
        }
        int timing_info_present_flag = ReadBit();
        if (timing_info_present_flag)
        {
            int num_units_in_tick = ReadBits(32);
            int time_scale = ReadBits(32);
            if (num_units_in_tick) {
                fps = time_scale / (2 * num_units_in_tick);
            }
        }
    }

    width = ((pic_width_in_mbs_minus1 + 1) * 16) - frame_crop_bottom_offset * 2 - frame_crop_top_offset * 2;
    height = ((2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16) - (frame_crop_right_offset * 2) - (frame_crop_left_offset * 2);
}

#define __int64 long long   
#define LONG long   
#define WORD        unsigned short   
#define DWORD       unsigned int   
#define BYTE        unsigned char   
#define LPBYTE char*   
#define _TCHAR char   
typedef unsigned char uint8;   
   
typedef unsigned short uint16;   
   
typedef unsigned long uint32;   
typedef unsigned __int64 uint64;   
typedef signed char int8;   
typedef signed short int16;   
typedef signed long int32;   
typedef signed __int64 int64;   
typedef unsigned int UINT32;   
typedef  int INT32;   
       
class NALBitstream   
{   
    public:   
    NALBitstream() : m_data(nullptr), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0) {};   
    NALBitstream(void * data, int len) { Init(data, len); };   
    void Init(void * data, int len) { m_data = (LPBYTE)data; m_len = len; m_idx = 0; m_bits = 0; m_byte = 0; m_zeros = 0; };   
        
    BYTE GetBYTE()   
    {   
        //printf("m_idx=%d,m_len=%d\n", m_idx, m_len);   
        if ( m_idx >= m_len )   
            return 0;   
        BYTE b = m_data[m_idx++];   
       
        // to avoid start-code emulation, a byte 0x03 is inserted   
        // after any 00 00 pair. Discard that here.   
        if ( b == 0 )   
        {   
            m_zeros++;   
            if ( (m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03) )   
            {     
                m_idx++;   
                m_zeros=0;   
            }   
        }    
        else    
        {   
            m_zeros = 0;   
           
        }   
        return b;   
    };   
   
   
    UINT32 GetBit()    
    {   
        //printf("m_bits=%d\n", m_bits);   
        if (m_bits == 0)    
        {   
            m_byte = GetBYTE();   
            m_bits = 8;      
        }   
        m_bits--;   
        return (m_byte >> m_bits) & 0x1;   
    };   
       
    UINT32 GetWord(int bits)    
    {   
        UINT32 u = 0;   
        while ( bits > 0 )   
        {   
            u <<= 1;   
            u |= GetBit();   
            bits--;   
        }   
        return u;   
    };   
    UINT32 GetUE()    
    {   
   
        // Exp-Golomb entropy coding: leading zeros, then a one, then   
        // the data bits. The number of leading zeros is the number of   
        // data bits, counting up from that number of 1s as the base.   
        // That is, if you see   
        //      0001010   
        // You have three leading zeros, so there are three data bits (010)   
        // counting up from a base of 111: thus 111 + 010 = 1001 = 9   
        int zeros = 0;   
        while (m_idx < m_len && GetBit() == 0 ) zeros++;   
        return GetWord(zeros) + ((1 << zeros) - 1);   
    };   
   
   
    INT32 GetSE()   
    {   
       
        // same as UE but signed.   
        // basically the unsigned numbers are used as codes to indicate signed numbers in pairs   
        // in increasing value. Thus the encoded values   
        //      0, 1, 2, 3, 4   
        // mean   
        //      0, 1, -1, 2, -2 etc   
        UINT32 UE = GetUE();   
        bool positive = UE & 1;   
        INT32 SE = (UE + 1) >> 1;   
        if ( !positive )   
        {   
            SE = -SE;   
        }   
        return SE;   
    };   
   
   
    private:   
    LPBYTE m_data;   
    int m_len;   
    int m_idx;   
    int m_bits;   
    BYTE m_byte;   
    int m_zeros;   
};   
   
   
int  h265_decode_sps(char* buf, unsigned int nLen, int &width, int &height, int &fps)   
{   
    if (nLen < 20)   
    {    
        return -1;   
    }   
       
    NALBitstream bs(buf, nLen);   
       
    // seq_parameter_set_rbsp()   
    bs.GetWord(4);// sps_video_parameter_set_id   
    int sps_max_sub_layers_minus1 = bs.GetWord(3); // "The value of sps_max_sub_layers_minus1 shall be in the range of 0 to 6, inclusive."   
    if (sps_max_sub_layers_minus1 > 6)    
    {   
		return -1;
    }   
    bs.GetWord(1);// sps_temporal_id_nesting_flag   
    // profile_tier_level( sps_max_sub_layers_minus1 )  
    int  general_profile_idc = 0;
    int  general_level_idc = 0;
    {   
        bs.GetWord(2);// general_profile_space   
        bs.GetWord(1);// general_tier_flag   
        general_profile_idc = bs.GetWord(5);// general_profile_idc   
        bs.GetWord(32);// general_profile_compatibility_flag[32]   
        bs.GetWord(1);// general_progressive_source_flag   
        bs.GetWord(1);// general_interlaced_source_flag   
        bs.GetWord(1);// general_non_packed_constraint_flag   
        bs.GetWord(1);// general_frame_only_constraint_flag   
        bs.GetWord(44);// general_reserved_zero_44bits   
        general_level_idc   = bs.GetWord(8);// general_level_idc   
        uint8 sub_layer_profile_present_flag[6] = {0};   
        uint8 sub_layer_level_present_flag[6]   = {0};   
        for (int i = 0; i < sps_max_sub_layers_minus1; i++)    
        {   
            sub_layer_profile_present_flag[i]= bs.GetWord(1);   
            sub_layer_level_present_flag[i]= bs.GetWord(1);   
        }   
        if (sps_max_sub_layers_minus1 > 0)    
        {   
            for (int i = sps_max_sub_layers_minus1; i < 8; i++)    
            {   
                uint8 reserved_zero_2bits = bs.GetWord(2);   
            }   
        }   
        for (int i = 0; i < sps_max_sub_layers_minus1; i++)    
        {   
            if (sub_layer_profile_present_flag[i])    
            {   
                bs.GetWord(2);// sub_layer_profile_space[i]   
                bs.GetWord(1);// sub_layer_tier_flag[i]   
                bs.GetWord(5);// sub_layer_profile_idc[i]   
                bs.GetWord(32);// sub_layer_profile_compatibility_flag[i][32]   
                bs.GetWord(1);// sub_layer_progressive_source_flag[i]   
                bs.GetWord(1);// sub_layer_interlaced_source_flag[i]   
                bs.GetWord(1);// sub_layer_non_packed_constraint_flag[i]   
                bs.GetWord(1);// sub_layer_frame_only_constraint_flag[i]   
                bs.GetWord(44);// sub_layer_reserved_zero_44bits[i]   
            }   
            if (sub_layer_level_present_flag[i])    
            {   
                bs.GetWord(8);// sub_layer_level_idc[i]   
            }   
        }   
    }   
    uint32 sps_seq_parameter_set_id= bs.GetUE(); // "The  value  of sps_seq_parameter_set_id shall be in the range of 0 to 15, inclusive."   
    if (sps_seq_parameter_set_id > 15)    
    {   
		return -1;
    }   
    uint32 chroma_format_idc = bs.GetUE(); // "The value of chroma_format_idc shall be in the range of 0 to 3, inclusive."   
    if (sps_seq_parameter_set_id > 3)   
    {   
		return -1;
    }   
    if (chroma_format_idc == 3)    
    {   
        bs.GetWord(1);// separate_colour_plane_flag   
    }   
    width  = bs.GetUE(); // pic_width_in_luma_samples   
    height  = bs.GetUE(); // pic_height_in_luma_samples   
    if (bs.GetWord(1))    
    {// conformance_window_flag   
        bs.GetUE();  // conf_win_left_offset   
        bs.GetUE();  // conf_win_right_offset   
        bs.GetUE();  // conf_win_top_offset   
        bs.GetUE();  // conf_win_bottom_offset   
    }   
    uint32 bit_depth_luma_minus8= bs.GetUE();   
    uint32 bit_depth_chroma_minus8= bs.GetUE();   
    if (bit_depth_luma_minus8 != bit_depth_chroma_minus8)    
    {   
		return -1;
    }      
    return 0;   
}   
