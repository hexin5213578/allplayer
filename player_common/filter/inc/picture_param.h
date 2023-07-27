#pragma once

typedef struct structPictureParameter
{
	float	brightness	= 0.0;	//亮度 The value must be a float value in range -1.0 to 1.0. The default value is "0".
	float	contrast	= 1.0;	//对比度 The value must be a float value in range -2.0 to 2.0. The default value is "1".
	float	saturation	= 1.0;	//饱和度 The value must be a float in range 0.0 to 3.0. The default value is "1". 

}PictureParams;

//unsharp
//1_mszie_x:1_msize_y:1_amount => 3-13,默认5:3-13,默认5:-2.0-5.0,默认1.0

//samrtblur
//luma_r:luma_s:luma_t => 0.1-5:-1.0,1.0:-30-30

#define EQ_FILTER_BRIGHT_CONTRAST_SATURATION "eq=brightness=%f:contrast=%f:saturation=%f"
#define EQ_FILTER_BRIGHT_CONTRAST "eq=brightness=%f:contrast=%f"
#define EQ_FILTER_BRIGHT_SATURATION "eq=brightness=%f:saturation=%f"
#define EQ_FILTER_CONTRAST_SATURATION "eq=contrast=%f:saturation=%f"
#define EQ_FILTER_BRIGHT "eq=brightness=%f"
#define EQ_FILTER_CONTRAST "eq=contrast=%f"
#define EQ_FILTER_SATURATION "eq=saturation=%f"
