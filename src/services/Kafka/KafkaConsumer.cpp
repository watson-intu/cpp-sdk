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
#include "librdkafka/src/rdkafka.h"

KafkaConsumer::KafkaConsumer() : IService("KafkaConsumerV1"), 
	m_bActive(false),
	m_bThreadStopped(false),
	m_pKafkaConfig(NULL),
	m_pKafkaConsumer(NULL)
{}

void KafkaConsumer::Serialize(Json::Value & json)
{
	IService::Serialize(json);
	json["m_HostName"] = m_HostName;
}

void KafkaConsumer::Deserialize(const Json::Value & json)
{
	IService::Deserialize(json);
	if (json["m_HostName"].isString())
		m_HostName = json["m_HostName"].asString();
}

bool KafkaConsumer::Start()
{
	char error[512];

	if (!IService::Start())
		return false;
	if (m_bActive)
		return false;
	m_bActive = true;

	m_pKafkaConfig = rd_kafka_conf_new();
	if ( m_pConfig->m_CustomMap.find( "client.id" ) != m_pConfig->m_CustomMap.end() )
	{
		if (rd_kafka_conf_set(m_pKafkaConfig, "client.id", m_pConfig->m_CustomMap["client.id"].c_str(),
			error, sizeof(error)) != RD_KAFKA_CONF_OK)
		{
			Log::Error("Kafka", "Failed to set client.id: %s", error);
			return false;
		}
	}
	if ( m_pConfig->m_CustomMap.find( "group.id" ) != m_pConfig->m_CustomMap.end() )
	{
		if (rd_kafka_conf_set(m_pKafkaConfig, "group.id", m_pConfig->m_CustomMap["group.id"].c_str(),
			error, sizeof(error)) != RD_KAFKA_CONF_OK)
		{
			Log::Error("Kafka", "Failed to set group.id: %s", error);
			return false;
		}
	}
	if ( m_pConfig->m_CustomMap.find( "bootstrap.servers" ) != m_pConfig->m_CustomMap.end() )
	{
		if (rd_kafka_conf_set(m_pKafkaConfig, "bootstrap.servers", m_pConfig->m_URL.c_str(),
			error, sizeof(error)) != RD_KAFKA_CONF_OK)
		{
			Log::Error("Kafka", "Failed to set client.id: %s", error);
			return false;
		}
	}

	m_pKafkaConsumer = rd_kafka_new(RD_KAFKA_CONSUMER,m_pKafkaConfig,error,sizeof(error));
	if ( m_pKafkaConsumer == NULL )
	{
		Log::Error( "KafkaConsumer", "Failed to create consumer: %s", error) );
		return false;
	}
			

	return true;
}

bool KafkaConsumer::Stop()
{
	if (!m_bActive)
		return false;

	m_bActive = false;
	while(! m_bThreadStopped )
		boost::this_thread::sleep( boost::posix_time::milliseconds(5) );

	if ( m_pKafkaConsumer != NULL )
	{
		rd_kafka_destroy( m_pKafkaConsumer );
		m_pKafkaConsumer = NULL;
	}
	if ( m_pKafkaConfig != NULL )
	{
		rd_kafka_conf_destroy( m_pKafkaConfig );
		m_pKafkaConfig = NULL;
	}

	return IService::Stop();
}

void KafkaConsumer::ConsumeThread()
{

}

