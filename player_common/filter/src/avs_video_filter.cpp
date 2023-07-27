#include "avs_video_filter.h"
#include "as_log.h"
#include "as_config.h"

#define	FILTER_PARAM_LEN	128
const int FF_ERROR_LEN = 128;

std::unordered_set<std::string>	AvsVideoFilter::SUPPORT_PARAMS{ "brightness","contrast","saturation", "smartblur","unsharp", "hqdn3d" };

std::unordered_map<std::string, Param_Restraint> AvsVideoFilter::restraints;

std::unordered_set<std::string> eq_params{ "brightness", "contrast", "saturation" };
std::unordered_set<std::string> blur_params{ "smartblur" };
std::unordered_set<std::string> sharp_params{ "unsharp" };
std::unordered_set<std::string> denoise_params{ "hqdn3d" };

#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#define SPRINTF_PARAM_TO_BUFFER(p,i,d)	_snprintf_s(cbuf,96, "%d", m_paramMap.count(p) ? m_paramMap[p].ivalue : d);

#define ADD_PARAM_TO_JSON(p,i,root)	do {	\
										int ivalue = m_paramMap.count(p) ? m_paramMap[p].ivalue : floatMapToInt(restraints[p].def_value ,restraints[p].min_value, restraints[p].max_value);   \
										_snprintf_s(cbuf, 96, "%d", ivalue);	\
										cJSON* json = cJSON_CreateString((char*)&cbuf[0]);		\
										cJSON_AddItemToObject(root, p, json);		\
									}while(0)

#define ADD_DEFUALT_PARAM_TO_JSON(p, root) do {	\
											int ivalue = floatMapToInt(restraints[p].def_value ,restraints[p].min_value, restraints[p].max_value);   \
											_snprintf_s(cbuf, 96, "%d", ivalue);	\
											cJSON* json = cJSON_CreateString((char*)&cbuf[0]);		\
											cJSON_AddItemToObject(root, p, json);		\
										}while(0)

#else

#define SPRINTF_PARAM_TO_BUFFER(p,i,d)	memset(cbuf,128,0); sprintf(cbuf, "%d", m_paramMap.count(p) ? m_paramMap[p].ivalue : d);

#define ADD_PARAM_TO_JSON(p,i,root)	do {	\
									int ivalue = m_paramMap.count(p) ? m_paramMap[p].ivalue : floatMapToInt(restraints[p].def_value ,restraints[p].min_value, restraints[p].max_value);   \
                                    memset(cbuf,128,0);     \
									sprintf(cbuf, "%d", ivalue);	\
									cJSON* json = cJSON_CreateString((char*)&cbuf[0]);		\
									cJSON_AddItemToObject(root, p, json);		\
								}while(0)

#define ADD_DEFUALT_PARAM_TO_JSON(p, root) do {	\
									int ivalue = floatMapToInt(restraints[p].def_value ,restraints[p].min_value, restraints[p].max_value);   \
									sprintf(cbuf, "%d", ivalue);	\
									cJSON* json = cJSON_CreateString((char*)&cbuf[0]);		\
									cJSON_AddItemToObject(root, p, json);		\
								}while(0)

#endif

AvsVideoFilter::AvsVideoFilter()
	:AvsFilter(),
	m_lastWidth(0),
	m_lastHeight(0)
{
}

void AvsVideoFilter::adjust_pic_param(std::string& paraName, float* fvalue,int fsize, int32_t ivalue)
{
	if (eq_params.count(paraName)) 
	{
		Picture_Param_ST picParam{ {0.0 ,0.0, 0.0}, ivalue, true };
		m_paramMap.insert(std::make_pair("eq", picParam));
	}

	if (0 == m_paramMap.count(paraName))
	{
		Picture_Param_ST picParam{ {0,0,0}, ivalue, true };
		for (int i = 0; i < fsize; ++i) {
			picParam.fvalues[i] = fvalue[i];
		}
		m_paramMap.insert(std::make_pair(paraName, picParam));
	}
	else 
	{
		for (int i = 0; i < fsize; ++i) {
			m_paramMap[paraName].fvalues[i] = fvalue[i];
		}
		m_paramMap[paraName].ivalue = ivalue;
		m_paramMap[paraName].apply = true;
	}
}

float intMapToFloat(int iValue, float min, float max)
{
	float delta = max - min;
	float result = delta * iValue / 100.f + min;
	return result;
}

int floatMapToInt(float fvlaue, float min, float max)
{
	int iRet = (fvlaue - min) / (max - min) * 100;
	return iRet;
}

void AvsVideoFilter::init_restraints()
{
	static bool inited = false;
	if (inited) {
		return;
	}
	
	Param_Restraint restraint;
	restraint.def_value = 0.0f;
	restraint.min_value = -1.0f;
	restraint.max_value = 1.0f;
	restraints["brightness"] = restraint;

	restraint.def_value = 1.0f;
	restraint.min_value = -2.0f;
	restraint.max_value = 2.0f;
	restraints["contrast"] = restraint;

	restraint.def_value = 1.0f;
	restraint.min_value = 0.0f;
	restraint.max_value = 3.0f;
	restraints["saturation"] = restraint;

	restraint.def_value = 5.0f;
	restraint.min_value = 3.0f;
	restraint.max_value = 13.0f;
	restraints["luma_x"] = restraint;
	restraints["luma_y"] = restraint;

	restraint.def_value = 0.0f;
	restraint.min_value = -2.0f;
	restraint.max_value = 5.0f;
	restraints["luma_amount"] = restraint;
	inited = true;
}

std::string AvsVideoFilter::get_default_params()
{
	init_restraints();

	char* result = "";
	cJSON* root = cJSON_CreateObject();
	if (!root) {
		return result;
	}

	char cbuf[FILTER_PARAM_LEN] = { 0 };
	ADD_DEFUALT_PARAM_TO_JSON("brightness", root);
	ADD_DEFUALT_PARAM_TO_JSON("contrast", root);
	ADD_DEFUALT_PARAM_TO_JSON("saturation", root);

	cJSON* unsharp = cJSON_CreateObject();
	ADD_DEFUALT_PARAM_TO_JSON("luma_x", unsharp);
	ADD_DEFUALT_PARAM_TO_JSON("luma_y", unsharp);
	ADD_DEFUALT_PARAM_TO_JSON("luma_amount", unsharp);
	cJSON_AddItemToObject(root, "unsharp", unsharp);

#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
	_snprintf_s(cbuf, 96, "%d", 0);
#else
	sprintf(cbuf, "%d", 0);
#endif
	cJSON* dn3d = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "hqdn3d", dn3d);
	char* filterStr = cJSON_Print(root);
	cJSON_Delete(root);
	std::string filter1(filterStr);
	free(filterStr);
	return filter1;
}

std::string AvsVideoFilter::get_picture_params()
{
	char* result = "";
	cJSON* root = cJSON_CreateObject();
	if (!root) {
		return result;
	}

	char cbuf[FILTER_PARAM_LEN] = { 0 };
	ADD_PARAM_TO_JSON("brightness",0, root);
	ADD_PARAM_TO_JSON("contrast", 0, root);
	ADD_PARAM_TO_JSON("saturation", 0, root);

	cJSON* unsharp = cJSON_CreateObject();
	ADD_PARAM_TO_JSON("luma_x", 0, unsharp);
	ADD_PARAM_TO_JSON("luma_y", 0, unsharp);
	ADD_PARAM_TO_JSON("luma_amount", 0, unsharp);
	cJSON_AddItemToObject(root, "unsharp", unsharp);

	SPRINTF_PARAM_TO_BUFFER("hqdn3d", 0, 0);
	cJSON* dn3d = cJSON_CreateString((char*)&cbuf[0]);
	cJSON_AddItemToObject(root, "hqdn3d", dn3d);
	char* filterStr = cJSON_Print(root);
	std::string filter1(filterStr);
	cJSON_Delete(root);
	free(filterStr);
	return filter1;
}

int AvsVideoFilter::parse_filter_params(std::string & params)
{
	init_restraints();

	cJSON* paramRoot = cJSON_Parse(params.c_str());
	if (!paramRoot) {
		return AVERROR(EINVAL);
	}

	int paramSize = cJSON_GetArraySize(paramRoot);
	int parseSuccess = 0;
	float fValue = 0.0;

	for (int i = 0; i < paramSize; ++i) {
		cJSON* param = cJSON_GetArrayItem(paramRoot, i);
		std::string para_string(param->string);
		if (SUPPORT_PARAMS.count(para_string)) {
			if (blur_params.count(para_string) || sharp_params.count(para_string)) {
				float fValues[3] = { 0 };
				int sharpSize = cJSON_GetArraySize(param);
				if (sharpSize != 3) {
					continue;
				}
				for (int j = 0; j < sharpSize; ++j) {
					cJSON *jsonVal = cJSON_GetArrayItem(param, j);
					int32_t setVal = atoi(jsonVal->valuestring);
					fValues[j] = intMapToFloat(setVal, restraints[jsonVal->string].min_value, restraints[jsonVal->string].max_value);
					std::string name = std::string(jsonVal->string);
					adjust_pic_param(name, &fValues[j], 1, setVal);
				}
				adjust_pic_param(para_string, fValues, sharpSize, 0);
			}
			else {
				int32_t setVal = atoi(param->valuestring);
				fValue = intMapToFloat(setVal, restraints[para_string].min_value, restraints[para_string].max_value);
				adjust_pic_param(para_string, &fValue, 1, setVal);
			}
		}
		else {
			continue;
		}
		++parseSuccess;
	}
	m_filterChanged = (parseSuccess > 0);
	cJSON_Delete(paramRoot);
	return parseSuccess;
}

int AvsVideoFilter::generate_filter_description()
{
	char eq_filter[FILTER_PARAM_LEN * 3] = { 0 };
	bool hasPre = false;
	if (m_paramMap.count("eq")) {
		hasPre = true;
		float fbright = (m_paramMap.count("brightness") && m_paramMap["brightness"].apply) ? m_paramMap["brightness"].fvalues[0] : 0.0f;
		float fcontrast = m_paramMap.count("contrast") && m_paramMap["contrast"].apply ? m_paramMap["contrast"].fvalues[0] : 1.0f;
		float fsaturation = m_paramMap.count("saturation") && m_paramMap["saturation"].apply ? m_paramMap["saturation"].fvalues[0] : 1.0f;
		snprintf(eq_filter, sizeof(eq_filter), "eq=brightness=%f:contrast=%f:saturation=%f", fbright, fcontrast, fsaturation);
	}

	if (m_paramMap.count("smartblur") && m_paramMap["smartblur"].apply) {
		/*if (hasPre) 
			snprintf(eq_filter, sizeof(eq_filter), "%s,smartblur=%s", eq_filter, m_paramMap["smartblur"].svalue.c_str());
		else 
			snprintf(eq_filter, sizeof(eq_filter), "smartblur=%s", m_paramMap["smartblur"].svalue.c_str());
		
		hasPre = true;*/
	}

	if (m_paramMap.count("unsharp") && m_paramMap["unsharp"].apply) {
		if (hasPre) {
			snprintf(eq_filter, sizeof(eq_filter), "%s,unsharp=%d:%d:%f", eq_filter, int(m_paramMap["luma_x"].fvalues[0]),
				int(m_paramMap["luma_y"].fvalues[0]), m_paramMap["luma_amount"].fvalues[0]);
		}
		else {
			snprintf(eq_filter, sizeof(eq_filter), "unsharp=%d:%d:%f", int(m_paramMap["luma_x"].fvalues[0]),
				int(m_paramMap["luma_y"].fvalues[0]), m_paramMap["luma_amount"].fvalues[0]);
		}
		hasPre = true;
	}

	if (m_paramMap.count("hqdn3d") && m_paramMap["hqdn3d"].apply) {
		if (hasPre) {
			snprintf(eq_filter, sizeof(eq_filter), "%s,hqdn3d", eq_filter);
		}
		else {
			snprintf(eq_filter, sizeof(eq_filter), "hqdn3d");
		}
		hasPre = true;
	}

	m_filterDescr = eq_filter;
	return 0;
}

int AvsVideoFilter::configure_filtergraph(const char* filtergraph, AVFilterContext* source_ctx, AVFilterContext* sink_ctx)
{
	int ret = AvsFilter::configure_filtergraph(filtergraph, source_ctx, sink_ctx);
	if (0 == ret) {
		for (auto& param : m_paramMap)
			param.second.apply = false;	
	}
	return ret;
}

bool AvsVideoFilter::need_reconfig(AVFrame* frame)
{
	return m_graph &&
		(m_filterChanged || (m_lastWidth != frame->width) || (m_lastHeight != frame->height) || (m_lastFormat != frame->format));
}

int AvsVideoFilter::configure_filters(const char* filters, AVFrame* frame)
{
	if (need_reconfig(frame)) {
		AS_LOG(AS_LOG_INFO, "biz[%ld] frame change from size:%dx%d format:%s to size:%dx%d format:%s", 
			m_bizId,
			m_lastWidth, m_lastHeight,
			(const char*)av_x_if_null(av_get_pix_fmt_name((AVPixelFormat)m_lastFormat), "none"),
			frame->width, frame->height,
			(const char*)av_x_if_null(av_get_pix_fmt_name((AVPixelFormat)frame->format), "none"));

		int ret;
		char buffersrc_args[FILTER_PARAM_LEN];
		AVFilterContext* filt_src = NULL, * filt_out = NULL, * last_filter = NULL;

		snprintf(buffersrc_args, sizeof(buffersrc_args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
			frame->width, frame->height, frame->format,
			1, 25);

		char err_buf[FF_ERROR_LEN] = { 0 };

		do {
			if ((ret = avfilter_graph_create_filter(&filt_src, avfilter_get_by_name("buffer"), "acplay_buffer", buffersrc_args, NULL, m_graph)) < 0) {
				AS_LOG(AS_LOG_ERROR, "avfilter_graph_create_filter(buffer) err: %d.", ret);
				break;
			}

			ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink"), "acplay_buffersink", NULL, NULL, m_graph);
			if (ret < 0) {
				AS_LOG(AS_LOG_ERROR, "avfilter_graph_create_filter(buffersink) err: %d.", ret);
				break;
			}

			if ((ret = configure_filtergraph(filters, filt_src, filt_out)) < 0) {
				break;
			}

		} while (0);
		
		if (ret < 0) {
			av_strerror(ret, err_buf, FF_ERROR_LEN - 1);
			AS_LOG(AS_LOG_ERROR, "biz[%ld] configure video filters error, %s.", m_bizId, err_buf);
			return ret;
		}

		m_buffersrc_ctx = filt_src;
		m_buffersink_ctx = filt_out;
		m_lastWidth = frame->width;
		m_lastHeight = frame->height;
		m_lastFormat = (AVPixelFormat)frame->format;
		return ret;
	}
	return -1;
}