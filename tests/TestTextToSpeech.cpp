//
// Created by John Andersen on 2/23/16.
//

#include "utils/UnitTest.h"
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/Config.h"
#include "services/TextToSpeech/TextToSpeech.h"

class TestTextToSpeech : UnitTest
{
public:
	//! Construction
	TestTextToSpeech() 
		: UnitTest( "TestTextToSpeech" )
		, m_GetVoicesTested(false)
		, m_ToSpeechTested( false )
		, m_ServiceStatusTested(false)
	{}

	bool m_GetVoicesTested;
	bool m_ToSpeechTested;
	bool m_ServiceStatusTested;

	virtual void RunTest()
	{
		Config config;
		Test( ISerializable::DeserializeFromFile( "./etc/tests/unit_test_config.json", &config ) != NULL );

		ThreadPool pool(1);

		TextToSpeech tts;
		Test( tts.Start() );
		tts.GetVoices( DELEGATE( TestTextToSpeech, OnGetVoices, Voices *, this ) );

		Time start;
		while( (Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_GetVoicesTested )
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_GetVoicesTested);

		tts.GetServiceStatus(DELEGATE(TestTextToSpeech, OnGetServiceStatus, const IService::ServiceStatus &, this));

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_ServiceStatusTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_ServiceStatusTested);

		tts.SetCacheEnabled(true);
		tts.ToSound( "Hello World", DELEGATE( TestTextToSpeech, OnToSpeech, Sound *, this ));

		start = Time();
		while( (Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_ToSpeechTested )
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_ToSpeechTested);
	}

	void OnGetVoices( Voices * a_pVoices )
	{
		Test( a_pVoices != NULL );
		m_GetVoicesTested = true;
		delete a_pVoices;
	}

	void OnGetServiceStatus(const IService::ServiceStatus & a_Status)
	{
		m_ServiceStatusTested = true;
	}

	void OnToSpeech( Sound * a_pSound )
	{
		Test( a_pSound != NULL );
		m_ToSpeechTested = true;
		delete a_pSound;
	}
};

TestTextToSpeech TEST_TEXT_TO_SPEECH;

