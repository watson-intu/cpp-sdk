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

struct rd_kafka_conf_s;
typedef struct rd_kafka_conf_s rd_kafka_conf_t;
struct rd_kafka_s;
typedef struct rd_kafka_s rd_kafka_t;
struct rd_kafka_topic_partition_list_s;
typedef struct rd_kafka_topic_partition_list_s rd_kafka_topic_partition_list_t;

//! low-level service interface for communicating with Kafka
class WDC_API KafkaConsumer : public IService
{
public:
	RTTI_DECL();

	//! Types
	typedef boost::shared_ptr<Kafka>			SP;
	typedef boost::weak_ptr<Kafka>				WP;

	//! Construction
	KafkaConsumer();

	//! ISerializable
	virtual void Serialize(Json::Value & json);
	virtual void Deserialize(const Json::Value & json);

	//! IService interface
	virtual bool Start();
	virtual bool Stop();

	//! Accessors

	//! Mutators

protected:
	//! Types
	typedef std::vector<std::string>		StringList;

	//! Data
	volatile bool		m_bActive;
	volatile bool		m_bThreadStopped;
	std::string			m_HostName;

	rd_kafka_conf_t *	m_pKafkaConfig;
	rd_kafka_t *		m_pKafkaConsumer;
	rd_kafka_topic_partition_list_t *
						m_pTopics;

	void ConsumeThread();
};

#endif
