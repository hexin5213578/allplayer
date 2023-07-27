#pragma once
#include "avs_filter.h"

typedef struct _stPictureParam
{
	float			fvalues[3];
	int32_t			ivalue;
	bool			apply;
}Picture_Param_ST;
typedef std::unordered_map<std::string, Picture_Param_ST> Picture_Params_MAP;

typedef struct _strParamRestraint 
{
	float	def_value;
	float	min_value;
	float	max_value;
}Param_Restraint;

class AvsVideoFilter : public AvsFilter
{
public:
	AvsVideoFilter();
	virtual ~AvsVideoFilter() = default;

	std::string get_picture_params();

	int parse_filter_params(std::string& params) override;
	
	static void init_restraints();

	static std::string get_default_params();

protected:
	bool need_reconfig(AVFrame* frame) override;

	int configure_filters(const char* filters, AVFrame* frame) override;

	int	generate_filter_description() override;

	int	configure_filtergraph(const char* filtergraph, AVFilterContext* source_ctx, AVFilterContext* sink_ctx) override;

	static std::unordered_set<std::string>	SUPPORT_PARAMS;
	static std::unordered_map<std::string, Param_Restraint> restraints;

private:
	void adjust_pic_param(std::string& paraName, float* fvalue, int fsize, int32_t ivalue);

private:
	Picture_Params_MAP	m_paramMap;
	int					m_lastWidth;
	int					m_lastHeight;
};

