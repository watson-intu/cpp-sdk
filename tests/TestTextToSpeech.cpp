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
		if ( config.IsConfigured( tts.GetServiceId() ) )
		{
			Test( tts.Start() );

			tts.GetVoices( DELEGATE( TestTextToSpeech, OnGetVoices, Voices *, this ) );

			Spin(m_GetVoicesTested );
			Test(m_GetVoicesTested);

			tts.GetServiceStatus(DELEGATE(TestTextToSpeech, OnGetServiceStatus, const IService::ServiceStatus &, this));
			Spin(m_ServiceStatusTested);
			Test(m_ServiceStatusTested);

			tts.SetCacheEnabled(true);
			tts.ToSound( "Hello World", DELEGATE( TestTextToSpeech, OnToSpeech, Sound *, this ));
			Spin(m_ToSpeechTested);
			Test(m_ToSpeechTested);

			Test( tts.Stop() );
		}
		else
		{
			Log::Status( "TestTextToSpeech", "Skipping tests." );
		}
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

