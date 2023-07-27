#pragma once

#include "avs_player_common.h"
#include "avs_video_view.h" 
#include <unordered_map>

class ViewFactory
{
public:
	static ViewFactory* getInstance();
	
	void setRenderType(RENDER_TYPE type);
	
	AvsVideoView* createVideoView(void* window);
	
	AvsVideoView* createD3DVideoView(void* window);
	
	bool detroyViewView(void* window);
	
	void releaseVideoViews();

	RENDER_TYPE getRenderType() { return m_renderType; }

protected:
	ViewFactory();
	virtual ~ViewFactory() = default;

private:
	RENDER_TYPE		m_renderType;
	std::unordered_map<void*, AvsVideoView*> m_videoViews;
};
