//
// Created by John Andersen on 2/23/16.
//

#include "utils/UnitTest.h"
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/Config.h"
#include "services/SpeechToText/SpeechToText.h"

class TestSpeechToText : UnitTest
{
public:
	//! Construction
	TestSpeechToText() 
		: UnitTest( "TestSpeechToText" )
		, m_GetModelsTested(false)
		, m_ServiceStatusTested(false)
	{}

	bool m_GetModelsTested;
	bool m_ServiceStatusTested;

	virtual void RunTest()
	{
		Config config;
		Test( ISerializable::DeserializeFromFile( "./etc/tests/unit_test_config.json", &config ) != NULL );

		ThreadPool pool(1);

		SpeechToText stt;
		if ( config.IsConfigured( stt.GetServiceId() ) )
		{
			Test( stt.Start() );

			stt.GetModels( DELEGATE( TestSpeechToText, OnGetModels, SpeechModels *, this ) );
			Spin( m_GetModelsTested);
			Test(m_GetModelsTested);

			stt.GetServiceStatus(DELEGATE(TestSpeechToText, OnGetServiceStatus, const IService::ServiceStatus &, this));
			Spin(m_ServiceStatusTested);
			Test(m_ServiceStatusTested);

			Test( stt.Stop() );
		}
		else
		{
			Log::Status( "TestSpeechToText", "Skipping test." );
		}
	}

	void OnGetServiceStatus(const IService::ServiceStatus & a_Status)
	{
		m_ServiceStatusTested = true;
	}

	void OnGetModels( SpeechModels * a_pModels )
	{
		Test( a_pModels != NULL );
		m_GetModelsTested = true;
	}

};

TestSpeechToText TEST_SPEECH_TO_TEXT;

