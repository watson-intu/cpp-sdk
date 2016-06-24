//
// Created by John Andersen on 2/23/16.
//

#include "tests/UnitTest.h"
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
		, m_RecognizeTested( false )
		, m_ServiceStatusTested(false)
	{}

	bool m_GetModelsTested;
	bool m_RecognizeTested;
	bool m_ServiceStatusTested;

	virtual void RunTest()
	{
		Config config;
		Test( ISerializable::DeserializeFromFile( "./etc/tests/unit_test_config.json", &config ) != NULL );

		ThreadPool pool(1);

		SpeechToText stt;
		Test( stt.Start() );
		Test( stt.GetModels( DELEGATE( TestSpeechToText, OnGetModels, SpeechModels *, this ) ) != NULL );

		Time start;
		while( (Time().GetEpochTime() - start.GetEpochTime()) < 300.0 && !m_GetModelsTested )
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_GetModelsTested);
		//Test(m_RecognizeTested); <-- this is never set to true. A mistake?

		stt.GetServiceStatus(DELEGATE(TestSpeechToText, OnGetServiceStatus, const IService::ServiceStatus &, this));

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_ServiceStatusTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_ServiceStatusTested);
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

