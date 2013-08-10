/*!
 The MIT License (MIT)
 
 Copyright (c) 2013 Arron Hartley
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <stdlib.h>
#include <vector>
#include <android/asset_manager.h>
#include <android/log.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "jaasl.h"

#define JAASL_ERROR_LOG(...) __android_log_print(ANDROID_LOG_ERROR, "JAASL", __VA_ARGS__)
#define JAASL_INFO_LOG(...) __android_log_print(ANDROID_LOG_INFO, "JAASL", __VA_ARGS__)
#define D_OBJ(hndl) \
	JAASLAudioFileDesc& d = d_object(hndl)

struct JAASLAudioFileDesc
{
	SLObjectItf   objectItf;
	SLPlayItf     playItf;
	SLSeekItf     seekItf;
	SLVolumeItf   volumeItf;
};

class JAASoundLibPrivate
{
private:
	friend class JAASoundLib;
	JAASoundLib*                    m_qptr;
	bool                            m_bInitialised;

public:
	SLObjectItf                     m_engineObject;
	SLEngineItf                     m_engineEngine;
	SLObjectItf                     m_outputMix;
	
	std::vector<JAASLAudioFileDesc> m_descs;
	
private:
	JAASLAudioFileDesc& d_object(unsigned int handle)
	{
		unsigned int idx = handle-1;
		return m_descs[idx];
	}
	
	static void PlayerCallback(SLPlayItf caller, void *pContext, SLuint32 event)
	{
		if(event == SL_PLAYEVENT_HEADATEND)
		{
			(*caller)->SetPlayState(caller, SL_PLAYSTATE_STOPPED);
		}
	}
	
public:

	/**************************************************************/
	
	JAASoundLibPrivate(JAASoundLib* q)
	: m_qptr(q),
	  m_bInitialised(false),
	  m_engineObject(NULL),
	  m_engineEngine(NULL),
	  m_outputMix(NULL)
	{
	}
	
	/**************************************************************/
	
	bool initialise()
	{
		// create engine
		if(slCreateEngine(&m_engineObject, 0, NULL, 0, NULL, NULL) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("slCreateEngine failed.\n");
			return false;
		}
		
		// realize the engine
		if((*m_engineObject)->Realize(m_engineObject, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Engine Realize failed.\n");
			return false;
		}
		
		// get the engine interface, which is needed in order to create other objects
		if((*m_engineObject)->GetInterface(m_engineObject, SL_IID_ENGINE, &m_engineEngine) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Engine GetInterface failed.\n");
			return false;
		}
		
		// create output mix
		if((*m_engineEngine)->CreateOutputMix(m_engineEngine, &m_outputMix, 0, NULL, NULL) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Engine CreateOutputMix failed.\n");
			return false;
		}
		
		// realize output mix
		if((*m_outputMix)->Realize(m_outputMix, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("OutputMix Realize failed.\n");
			return false;
		}
		
		m_bInitialised = true;
		return true;
	}
	
	/**************************************************************/
	
	void destroy()
	{
		// destroy player objects
		for(int i = 0; i < m_descs.size(); ++i)
		{
			JAASLAudioFileDesc& d = m_descs[i];
			
			// destroy file descriptor audio player object, and invalidate all associated interfaces
			if (d.objectItf)
			{
				(*d.objectItf)->Destroy(d.objectItf);
				d.objectItf   = NULL;
				d.playItf     = NULL;
				d.seekItf     = NULL;
				d.volumeItf   = NULL;
			}
		}
		
		// destroy output mix object, and invalidate all associated interfaces
		if (m_outputMix)
		{
			(*m_outputMix)->Destroy(m_outputMix);
			m_outputMix = NULL;
		}
		
		// destroy engine object, and invalidate all associated interfaces
		if (m_engineObject)
		{
			(*m_engineObject)->Destroy(m_engineObject);
			m_engineObject = NULL;
			m_engineEngine = NULL;
		}
	}
	
	/**************************************************************/
	
	bool loadFromAsset(AAsset* asset, unsigned int& handle)
	{
		if(!asset)
		{
			JAASL_ERROR_LOG("No asset provided.\n");
			return false;
		}
		
		handle = 0;
	
		off_t start, length;
		int fd = AAsset_openFileDescriptor(asset, &start, &length);
		
		SLObjectItf   fdPlayerObject   = NULL;
		SLPlayItf     fdPlayerPlay     = NULL;
		SLSeekItf     fdPlayerSeek     = NULL;
		SLVolumeItf   fdPlayerVolume   = NULL;
		
		// configure audio source
		SLDataLocator_AndroidFD loc_fd = { SL_DATALOCATOR_ANDROIDFD, fd, start, length };
		SLDataFormat_MIME format_mime  = { SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED };
		SLDataSource audioSrc          = { &loc_fd, &format_mime };
		
		// configure audio sink
		SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, m_outputMix };
		SLDataSink audioSnk                = { &loc_outmix, NULL };
		
		// create audio player
		const SLInterfaceID ids[2] = { SL_IID_SEEK,     SL_IID_VOLUME };
		const SLboolean req[2]     = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
		if((*m_engineEngine)->CreateAudioPlayer(m_engineEngine, &fdPlayerObject, &audioSrc, &audioSnk, 2, ids, req) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Failed to create audio player.\n");
			return false;
		}
		
		// realize the player
		if((*fdPlayerObject)->Realize(fdPlayerObject, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Failed to realize audio player.\n");
			(*fdPlayerObject)->Destroy(fdPlayerObject);
			return false;
		}
		
		// get the play interface
		if((*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_PLAY, &fdPlayerPlay) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Failed to get 'play' interface.\n");
			(*fdPlayerObject)->Destroy(fdPlayerObject);
			return false;
		}
		
		// get the seek interface
		if((*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_SEEK, &fdPlayerSeek) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Failed to get 'seek' interface.\n");
			(*fdPlayerObject)->Destroy(fdPlayerObject);
			return false;
		}
		
		// get the volume interface
		if((*fdPlayerObject)->GetInterface(fdPlayerObject, SL_IID_VOLUME, &fdPlayerVolume) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Failed to get 'volume' interface.\n");
			(*fdPlayerObject)->Destroy(fdPlayerObject);
			return false;
		}
		
		// register a callback to notify when the play has finished
		if((*fdPlayerPlay)->RegisterCallback(fdPlayerPlay, JAASoundLibPrivate::PlayerCallback, NULL) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Failed to register callback function.\n");
			(*fdPlayerObject)->Destroy(fdPlayerObject);
			return false;
		}
		
		// set mask
		if((*fdPlayerPlay)->SetCallbackEventsMask(fdPlayerPlay, SL_PLAYEVENT_HEADATEND) != SL_RESULT_SUCCESS)
		{
			JAASL_ERROR_LOG("Failed to set callback mask.\n");
			(*fdPlayerObject)->Destroy(fdPlayerObject);
			return false;
		}
		
		JAASLAudioFileDesc d;
		d.objectItf   = fdPlayerObject;
		d.playItf     = fdPlayerPlay;
		d.seekItf     = fdPlayerSeek;
		d.volumeItf   = fdPlayerVolume;
		
		handle = (unsigned int)m_descs.size()+1;
		m_descs.push_back(d);
				
		return true;
	}
	
	/**************************************************************/
	
	bool play(unsigned int handle)
	{
		if(handle == 0)
			return false;
			
		D_OBJ(handle);

		if (d.playItf)
		{
			(*d.playItf)->SetPlayState(d.playItf, SL_PLAYSTATE_PLAYING);
		}
		
		return false;
	}
	
	/**************************************************************/
	
	bool stop(unsigned int handle)
	{
		if(handle == 0)
			return false;
		
		D_OBJ(handle);
		
		if (d.playItf)
		{
			(*d.playItf)->SetPlayState(d.playItf, SL_PLAYSTATE_STOPPED);
		}
		
		return false;
	}
	
	/**************************************************************/
	
	bool pause(unsigned int handle)
	{
		if(handle == 0)
			return false;
		
		D_OBJ(handle);
		
		if (d.playItf)
		{
			(*d.playItf)->SetPlayState(d.playItf, SL_PLAYSTATE_PAUSED);
		}
		
		return false;
	}
	
	/**************************************************************/
	
	void setVolume(unsigned int handle, float gain)
	{
		if(handle == 0)
			return;
		
		D_OBJ(handle);
		
		float atten         = gain < 0.01f ? -96.0f: 20 * log10(gain);
		SLmillibel millibel = (SLmillibel)(atten * 100);
		
		if (d.volumeItf)
		{
			(*d.volumeItf)->SetVolumeLevel(d.volumeItf, millibel);
		}
	}
	
	/**************************************************************/
	
	void setLooped(unsigned int handle, bool bLoop)
	{
		if(handle == 0)
			return;
		
		D_OBJ(handle);
		
		if(d.seekItf)
		{
			(*d.seekItf)->SetLoop(d.seekItf, bLoop ? SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE,
			                      0, SL_TIME_UNKNOWN);
		}
	}
	
	/**************************************************************/
	
	void setPlayPosition(unsigned int handle, unsigned long msec)
	{
		if(handle == 0)
			return;
		
		D_OBJ(handle);
		
		if(d.seekItf)
		{
			(*d.seekItf)->SetPosition(d.seekItf, msec, SL_SEEKMODE_FAST);
		}
	}
	
	/**************************************************************/
	
	unsigned long playLength(unsigned int handle)
	{
		if(handle == 0)
			return 0;
		
		D_OBJ(handle);
		
		SLmillisecond msec = 0;
		if (d.playItf)
		{
			// the player needs to be in either pause or play state
			SLuint32 playState;
			(*d.playItf)->GetPlayState(d.playItf, &playState);
			
			if(playState != SL_PLAYSTATE_PAUSED && playState != SL_PLAYSTATE_PLAYING)
			{
				(*d.playItf)->SetPlayState(d.playItf, SL_PLAYSTATE_PAUSED);
			}
			
			// retrieve the duration
			(*d.playItf)->GetDuration(d.playItf, &msec);
			
			// reset back to original state
			if(playState != SL_PLAYSTATE_PAUSED && playState != SL_PLAYSTATE_PLAYING)
			{
				(*d.playItf)->SetPlayState(d.playItf, playState);
			}
		}
		
		return (unsigned long)msec;
	}
};

/**************************************************************/

JAASoundLib::JAASoundLib() : m_dptr(new JAASoundLibPrivate(this))
{
}

/**************************************************************/

JAASoundLib::~JAASoundLib()
{
	delete m_dptr;
}

/**************************************************************/

bool JAASoundLib::initialise()
{
	return m_dptr->initialise();
}

/**************************************************************/

void JAASoundLib::destroy()
{
	m_dptr->destroy();
}

/**************************************************************/

bool JAASoundLib::loadFromAsset(AAsset* asset, unsigned int& handle)
{
	return m_dptr->loadFromAsset(asset, handle);
}

/**************************************************************/

bool JAASoundLib::play(unsigned int handle)
{
	return m_dptr->play(handle);
}

/**************************************************************/

bool JAASoundLib::stop(unsigned int handle)
{
	return m_dptr->stop(handle);
}

/**************************************************************/

bool JAASoundLib::pause(unsigned int handle)
{
	return m_dptr->pause(handle);
}

/**************************************************************/

void JAASoundLib::setVolume(unsigned int handle, float gain)
{
	m_dptr->setVolume(handle, gain);
}

/**************************************************************/

void JAASoundLib::setLooped(unsigned int handle, bool bLoop)
{
	m_dptr->setLooped(handle, bLoop);
}

/**************************************************************/

void JAASoundLib::setPlayPosition(unsigned int handle, unsigned long msec)
{
	m_dptr->setPlayPosition(handle, msec);
}

/**************************************************************/

unsigned long JAASoundLib::playLength(unsigned int handle)
{
	return m_dptr->playLength(handle);
}

/**************************************************************/

void JAASoundLib::stopAll()
{
	std::vector<JAASLAudioFileDesc>& descs = m_dptr->m_descs;
	for(int i = 0; i < descs.size(); ++i)
	{
		const JAASLAudioFileDesc& d = descs[i];
		if (d.playItf)
		{
			(*d.playItf)->SetPlayState(d.playItf, SL_PLAYSTATE_STOPPED);
		}
	}
}
