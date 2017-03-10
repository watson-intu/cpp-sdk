/* ***************************************************************** */
/*                                                                   */
/* IBM Confidential                                                  */
/* OCO Source Materials                                              */
/*                                                                   */
/* (C) Copyright IBM Corp. 2001, 2014                                */
/*                                                                   */
/* The source code for this program is not published or otherwise    */
/* divested of its trade secrets, irrespective of what has been      */
/* deposited with the U.S. Copyright Office.                         */
/*                                                                   */
/* ***************************************************************** */

#ifndef WDC_KAFKA_SERVICE_H
#define WDC_KAFKA_SERVICE_H

#include "services/IService.h"
#include "WDCLib.h"

//! low-level service interface for communicating with Kafka
class WDC_API KafkaConsumer : public IService
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
	bool				Subscribe( const std::string & a_Topic, 
							Delegate<std::string *> a_MessageCallback );
	bool				Unsubscribe( const std::string & a_Topic, bool a_bBlocking = false );

protected:
	//! Types
	typedef std::vector<std::string>		StringList;

	struct Subscription
	{
		Subscription() : m_bActive( false ), m_bStopped(false)
		{}

		volatile bool		m_bActive;
		volatile bool		m_bStopped;
		std::string			m_Topic;
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
