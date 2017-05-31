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


#ifndef WDC_KAFKA_SERVICE_H
#define WDC_KAFKA_SERVICE_H

#include "utils/IService.h"
#include "UtilsLib.h"

//! low-level service interface for communicating with Kafka
class UTILS_API KafkaConsumer : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<KafkaConsumer>			SP;
	typedef boost::weak_ptr<KafkaConsumer>				WP;

	//! Construction
	KafkaConsumer();

	//! ISerializable
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual bool Stop();

	//! Mutators
	bool				Subscribe( const std::string & a_Topic, int a_Partition,
							Delegate<std::string *> a_MessageCallback );
	bool				Unsubscribe( const std::string & a_Topic, bool a_bBlocking = false );

protected:
	//! Types
	typedef std::vector<std::string>		StringList;

	struct Subscription
	{
		Subscription() : m_bActive( false ), m_bStopped(false), m_Partition( 0 )
		{}

		volatile bool		m_bActive;
		volatile bool		m_bStopped;
		std::string			m_Topic;
		int					m_Partition;
		Delegate<std::string *>
							m_Callback;
	};
	typedef std::map<std::string,Subscription>	SubscriptionMap;

	//! Data
	SubscriptionMap		m_SubscriptionMap;

	void ConsumeThread( Subscription * a_pSub );
	bool Consume( Subscription * a_pSub );
};

#endif
