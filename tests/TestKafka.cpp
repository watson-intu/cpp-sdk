/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#include "utils/UnitTest.h"
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/Config.h"
#include "services/Kafka/KafkaConsumer.h"

#include <fstream>

class TestKafka : UnitTest
{
public:
	//! Construction
	TestKafka() : UnitTest("TestKafka"),
		m_nMessageCount(0),
		m_bConsumeTested(false)
	{}

	int m_nMessageCount;
	bool m_bConsumeTested;

	virtual void RunTest()
	{
		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		ThreadPool pool(1);

		KafkaConsumer consumer;
		if ( config.FindServiceConfig( consumer.GetServiceId() ) != NULL )
		{
			Test( consumer.Start() );

			consumer.Subscribe( "graphChange", 0,
				DELEGATE(TestKafka, OnGraphChanges, std::string *, this));
			//Spin(m_bConsumeTested, 300.0f);
			//Test(m_bConsumeTested);

			Test( consumer.Stop() );
		}
	}

	void OnGraphChanges(std::string * a_pMessage)
	{
		Test( a_pMessage != NULL );
		Log::Debug("TestKafka", "OnGraphChanges(): %s", a_pMessage->c_str());

		m_nMessageCount += 1;
		if ( m_nMessageCount > 5 )
			m_bConsumeTested = true;

		delete a_pMessage;
	}

};

TestKafka TEST_KAFKA;
