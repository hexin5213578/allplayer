#include "avs_refresh_loop.h"

static double computeTargetDelay(double delay, VideoState* is) {
#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

	double last = delay;
	double sync_threshold, diff = 0;
	if (kSyncVideoMaster != is->getMasterSyncType()) {
		auto vidc = is->vidclk.getClock();
		auto extc = is->getMasterClock();
		diff = vidc - extc;
		if (is->speed < 0.0) {
			diff *= -1;
		}
		//diff = is->vidclk.getClock() - is->getMasterClock();
		sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
			if (diff <= -sync_threshold) {
				delay = FFMAX(0, delay + diff);
			}
			else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
				delay = delay + diff;
			}
			else if (diff >= sync_threshold) {
				delay = 2 * delay;
			}
		}
		
		if (is->speed < 0.0 && abs(diff) > 1.0) {
			AS_LOG(AS_LOG_WARNING, "last_dutration:%lf, diff:%lf, computed delay %lf.", last, diff, delay);
		}
	}
	return delay;
}


#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10
/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

static void checkExternalClockSpeed(VideoState* is) {
	if (is->video_stream >= 0 && is->pictq->remainingNb() <= EXTERNAL_CLOCK_MIN_FRAMES) {
		is->extclk.setClockSpeed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
	}
	else if ((is->video_stream < 0 || is->pictq->remainingNb() > EXTERNAL_CLOCK_MAX_FRAMES) &&
		(is->audio_stream < 0 || is->sampq->remainingNb() > EXTERNAL_CLOCK_MAX_FRAMES)) {
		is->extclk.setClockSpeed(FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
	}
	else {
		double speed = is->extclk.speed;
		if (speed != 1.0) {
			is->extclk.setClockSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
		}
	}
}

int AvsRefreshLoop::setView(AvsVideoView::Ptr view) {
	if (view != m_view) {
		m_surfaceDiff = true;
	}

	if (m_surfaceDiff && m_view) {
		m_view = nullptr;
	}

	m_view = view;

#ifdef  _WIN32
	m_surfaceDiff = false;
#endif //  _WIN32
	return 0;
}

int AvsRefreshLoop::zoomOut(void* hwnd, int top, int right, int bottom, int left) {
	if (!m_view || !hwnd) {
		AS_LOG(AS_LOG_WARNING, "set zoom out fail, invalid argument.");
		return AVERROR(EINVAL);
	}

	std::unique_lock<decltype(m_zoomMutex)> lock(m_zoomMutex);
	if (m_zoomView && hwnd != m_zoomView->getHwnd()) {
		ViewFactory::getInstance()->detroyViewView(m_zoomView->getHwnd());
		m_zoomView = nullptr;
	}

	if (!m_zoomView) {
		m_zoomView = ViewFactory::getInstance()->createVideoView(hwnd);
		if (!m_zoomView) {
			return AVERROR(EFAULT);
		}
	}

	m_zoomView->setMarginBlankRatio(top, right, bottom, left);
	m_zooming = true;
	return 0;
}

int AvsRefreshLoop::zoonIn() {
	std::unique_lock<decltype(m_zoomMutex)> lock(m_zoomMutex);
	if (m_zooming && m_zoomView) {
		m_zoomView->clear();
	}
	return 0;
}

void AvsRefreshLoop::stopRun() {
	AvsThreadBase::stopRun();
	wait();
}

void AvsRefreshLoop::videoRefresh()
{
	double time, remaining_time = 0.0;
	double reference = getRelativeTime();
	int fps = 0;

	while (!m_bIsExit) {
		if (remaining_time > 0.0) {
			AVSleep(remaining_time);
		}
		remaining_time = 0.01;

		if (!m_vidState->paused && kSyncExternalClock == m_vidState->getMasterSyncType() && m_vidState->realtime) {
			checkExternalClockSpeed(m_vidState);
		}

		if (m_vidState->video_stream >= 0) {
		retry:
			if (0 == m_vidState->pictq->remainingNb()) {
				if (true) {
					remaining_time = 0.01;
					m_vidState->force_refresh = 1;
					goto display;
				}
			}
			else {
				double last_duration, duration, delay;
				MyFrame::Ptr vp, lastvp;

				/* dequeue the picture */
				lastvp = m_vidState->pictq->peekLast();
				vp = m_vidState->pictq->peek();

				if (vp->serial != m_vidState->videoq->serial()) {
					m_vidState->pictq->queueNext();
					goto retry;
				}

				if (lastvp->serial != vp->serial) {
					m_vidState->frame_timer = getRelativeTime();
				}
				
				if (m_vidState->paused) {
					m_vidState->force_refresh = 1;
					goto display;
				}

				last_duration = m_vidState->vpDuration(lastvp, vp);

				if (!m_vidState->realtime && m_vidState->speed != 1.0) {
					last_duration = last_duration / m_vidState->speed;
					if (m_vidState->speed < 0 && last_duration >= 0.10) {
						AS_LOG(AS_LOG_WARNING, "last dur: %llf, latvp: %llf, vp:%llf, speed:%llf.", last_duration, 
							lastvp->pts, vp->pts, m_vidState->speed);
					}

					static const int kTolerance = 50;
					//���������뻺�����ǰ����gop��frame��ֱ��������
					if (last_duration > 1.0 && m_vidState->pictq->remainingNb() > kTolerance) {
						m_vidState->pictq->queueNext();
						goto retry;
					}

				}
				delay = computeTargetDelay(last_duration, m_vidState);
				time = getRelativeTime();
				//remaining time for wait, continue
				if (time < m_vidState->frame_timer + delay) {
					remaining_time = FFMIN(m_vidState->frame_timer + delay - time, remaining_time);
					goto display;
				}

				m_vidState->frame_timer += delay;
				if (delay > 0 && (time - m_vidState->frame_timer > AV_SYNC_THRESHOLD_MAX)) {
					m_vidState->frame_timer = time;
				}

				//todo: lock mutex
				m_vidState->vidclk.setClock(vp->pts, vp->serial);
				m_vidState->extclk.sync2Slave(&m_vidState->vidclk);

				if (m_vidState->pictq->remainingNb() > 1) {
					MyFrame::Ptr nextvp = m_vidState->pictq->peekNext();
					duration = m_vidState->vpDuration(vp, nextvp) / m_vidState->speed;
					duration = FFMAX(0.04, duration);
					if ((kSyncVideoMaster != m_vidState->getMasterSyncType()) && (time > m_vidState->frame_timer + duration)) {
						//m_vidState->frame_drops_late++;
						m_stallEventNum++;
						m_stallDuration += duration;
						m_vidState->pictq->queueNext();
						goto retry;
					}
				}

				m_vidState->pictq->queueNext();
				m_vidState->force_refresh = 1;
			}

		display:
			if (!m_bIsExit && m_vidState->pictq->rShown() && m_vidState->force_refresh) {
				MyFrame::Ptr vf = m_vidState->pictq->peekLast();
				render(vf);
				if (!m_vidState->paused) {   //����ͣ״̬�²ż���֡��?
					++fps;
					auto current = getRelativeTime();
					if (current - reference >= 1.0) {
						reference = current;
						m_fps = fps, fps = 0;
					}
				}
			}
			m_vidState->force_refresh = 0;
		}
	}
}

void AvsRefreshLoop::render(MyFrame::Ptr vf)
{
#if ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
	if ((!m_view && m_hwnd) || m_surfaceDiff) {
		m_view.reset();
		ViewFactory::getInstance()->setRenderType(OPENGL);
		m_view = std::shared_ptr<AvsVideoView>(ViewFactory::getInstance()->createVideoView(m_hwnd), [](AvsVideoView* view) {
			if (view) {
				ViewFactory::getInstance()->detroyViewView(view->getHwnd());
			}
			});
		m_surfaceDiff = false;
	}
#endif

#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
	if (!m_view) {
		return;
	}
#endif
    MyFrame::Ptr frame = vf;

	implDraw(m_view.get(), frame->frame);
    
	if (m_zooming) {
		std::unique_lock<decltype(m_zoomMutex)> lock(m_zoomMutex);
		if (m_zoomView) {
			implDraw(m_zoomView, vf->frame);
		}
	}

	if (!m_rendered) {
		m_rendered = true;
		m_vidState->sendStatus(STREAM_STATUS_KEYFRAME, "render first frame");
	}

	long tns = m_duration;
	if (0 == tns && m_vidState->ic) {
		tns = m_vidState->ic->duration / 1000000LL;
	}

	if (!m_vidState->realtime) {
		if (abs(vf->pts - m_lastOutputTS) >= 1.00) {
			m_lastOutputTS = vf->pts;
			m_vidState->sendData(m_lastOutputTS, tns);
		}
	}
}

int AvsRefreshLoop::implDraw(AvsVideoView* view, AVFrame* frame) 
{
	if (!view->setTexturePara(frame->width, frame->height, (AvsVideoView::Format)frame->format)) {
		m_vidState->sendStatus(SET_TEXTURE_FAILED, "");
		return AVERROR(EINVAL);
	}

#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
	view->drawFrame(frame);
#else
	view->drawFrame(frame, m_surfaceWidth, m_surfaceHeight);
#endif
	return 0;
}
