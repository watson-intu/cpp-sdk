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

#include "KafkaConsumer.h"

#define LIBRDKAFKA_STATICLIB
#include "librdkafka/src/rdkafka.h"

RTTI_IMPL( KafkaConsumer, IService );
REG_SERIALIZABLE( KafkaConsumer );

KafkaConsumer::KafkaConsumer() : IService("KafkaConsumerV1")
{}

void KafkaConsumer::Serialize(Json::Value & json)
{
	IService::Serialize(json);
}

void KafkaConsumer::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
}

bool KafkaConsumer::Start()
{
	if (!IService::Start())
		return false;

	return true;
}

bool KafkaConsumer::Stop()
{
	for( SubscriptionMap::iterator iSub = m_SubscriptionMap.begin(); 
		iSub != m_SubscriptionMap.end(); ++iSub )
	{
		Unsubscribe( iSub->first, true );
	}
	m_SubscriptionMap.clear();

	return IService::Stop();
}

bool KafkaConsumer::Subscribe( const std::string & a_Topic, 
	Delegate<std::string *> a_MessageCallback )
{
	Subscription * pSub = &m_SubscriptionMap[ a_Topic ];
	if ( pSub->m_bActive )
		return false;		// already active

	pSub->m_bActive = true;
	pSub->m_bStopped = false;
	pSub->m_Topic = a_Topic;

	ThreadPool::Instance()->InvokeOnThread<Subscription *>( DELEGATE( KafkaConsumer, ConsumeThread, Subscription *, this ), pSub );
	return true;
}

bool KafkaConsumer::Unsubscribe( const std::string & a_Topic, bool a_bBlocking /*= false*/ )
{
	Subscription * pSub = &m_SubscriptionMap[ a_Topic ];
	if (! pSub->m_bActive )
		return false;		// already active

	pSub->m_bActive = false;
	if ( a_bBlocking )
	{
		while(! pSub->m_bStopped )
		{
			ThreadPool::Instance()->ProcessMainThread();
			boost::this_thread::sleep( boost::posix_time::milliseconds(5) );
		}
	}

	return true;
}


void KafkaConsumer::ConsumeThread( Subscription * a_pSub )
{
	try {
		if (! Consume( a_pSub ) )
			Log::Error( "KafkaConsumer", "Failed to run consumer." );
	}
	catch( const std::exception & ex )
	{
		Log::Error( "KafkaConsumer", "Caught Exception: %s", ex.what() );
	}
}

bool KafkaConsumer::Consume( Subscription * a_pSub )
{
	char error[500];

	rd_kafka_conf_t * pConfig = rd_kafka_conf_new();

	for( ServiceConfig::CustomMap::const_iterator iConfig = m_pConfig->m_CustomMap.begin(); 
		iConfig != m_pConfig->m_CustomMap.end(); ++iConfig )
	{
		if (rd_kafka_conf_set(pConfig, iConfig->first.c_str(), iConfig->second.c_str(),	error, sizeof(error)) != RD_KAFKA_CONF_OK)
			Log::Warning("Kafka", "Failed to set %s to %s: %s", iConfig->first.c_str(), iConfig->second.c_str(), error);
	}

	rd_kafka_t * pConsumer = rd_kafka_new(RD_KAFKA_CONSUMER,pConfig,error,sizeof(error));
	if ( pConsumer == NULL )
	{
		Log::Error( "KafkaConsumer", "Failed to create consumer: %s", error );
		return false;
	}

	rd_kafka_topic_conf_t * pTopicConf = rd_kafka_topic_conf_new();
	rd_kafka_topic_t * pTopic = rd_kafka_topic_new(pConsumer, a_pSub->m_Topic.c_str(), pTopicConf );

	int partition = RD_KAFKA_PARTITION_UA;
	int64_t start_offset = RD_KAFKA_OFFSET_END;

	if ( rd_kafka_consume_start( pTopic, partition, start_offset ) != 0 )
	{
		Log::Error( "KafkaConsumer", "Failed to start consuming." );
		return false;
	}

	while( a_pSub->m_bActive )
	{
		rd_kafka_poll( pConsumer, 250 );

		rd_kafka_message_t * pMsg = rd_kafka_consume( pTopic, partition, 250 );
		if (! pMsg )
			continue;

		if ( pMsg->err == 0 )
		{
			std::string * pPayload = new std::string( (const char *)pMsg->payload, pMsg->len );
			ThreadPool::Instance()->InvokeOnMain<std::string *>( a_pSub->m_Callback, pPayload );
		}
		else
		{
			Log::Error( "KafkaConsumer", "Consume error %s for topic %s", 
				rd_kafka_message_errstr( pMsg ), rd_kafka_topic_name( pMsg->rkt ) );
		}

		rd_kafka_message_destroy( pMsg );
	}

	rd_kafka_consume_stop( pTopic, partition );
	while (rd_kafka_outq_len(pConsumer) > 0)
		rd_kafka_poll(pConsumer, 10);

	rd_kafka_topic_destroy( pTopic );
	rd_kafka_destroy( pConsumer );
	rd_kafka_conf_destroy( pConfig );

	a_pSub->m_bStopped = true;
	return true;
}

