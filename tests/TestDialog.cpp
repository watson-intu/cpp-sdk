/**
* Copyright 2016 IBM Corp. All Rights Reserved.
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


#include "tests/UnitTest.h"
#include "utils/Log.h"
#include "utils/Time.h"
#include "utils/Config.h"
#include "utils/UniqueID.h"
#include "services/Dialog/Dialog.h"

class TestDialog : UnitTest
{
public:
	//! Construction
	TestDialog() : UnitTest("TestDialog"),
		m_ConversationId(0),
		m_ClientId(0),
		m_GetDialogsTested(false),
		m_ConverseTested(false),
		m_ResponseTested(false),
		m_UploadTested(false),
		m_DeleteTested(false),
		m_ServiceStatusTested(false)
	{}

	std::string m_DialogName;
	std::string m_DialogId;

	int m_ConversationId;
	int m_ClientId;
	bool m_GetDialogsTested;
	bool m_ConverseTested;
	bool m_ResponseTested;
	bool m_UploadTested;
	bool m_DeleteTested;
	bool m_ServiceStatusTested;

	virtual void RunTest()
	{
		Config config;
		Test(ISerializable::DeserializeFromFile("./etc/tests/unit_test_config.json", &config) != NULL);

		ThreadPool pool(1);

		m_DialogName = UniqueID().Get().substr(0, 20);

		Log::Status("TestDialog", "Testing UploadDialog");

		Dialog dialog;
		Test(dialog.Start());
		Test(dialog.UploadDialog(m_DialogName, "./etc/tests/pizza_sample.xml",
			DELEGATE(TestDialog, OnUploadDialog, const Json::Value &, this)) != NULL);
		Time start;
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_UploadTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_UploadTested);

		Log::Status("TestDialog", "Testing GetServiceStatus");

		dialog.GetServiceStatus(DELEGATE(TestDialog, OnGetServiceStatus, const IService::ServiceStatus &, this));

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_ServiceStatusTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}

		Test(m_ServiceStatusTested);

		Log::Status("TestDialog", "Testing Converse Hello");

		dialog.Converse(m_DialogId, "Hello",
			DELEGATE(TestDialog, OnConverse,const Json::Value &, this));

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_ConverseTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		Test(m_ConverseTested);

		Log::Status("TestDialog", "Testing Converse Large");

		dialog.Converse(m_DialogId, "Large",
			DELEGATE(TestDialog, OnResponse, const Json::Value &, this), m_ConversationId, m_ClientId);
		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_ResponseTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		Test(m_ResponseTested);

		Log::Status("TestDialog", "Testing GetDialogs");

		dialog.GetDialogs(
			DELEGATE(TestDialog, OnGetDialogs, Dialogs *, this));

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_GetDialogsTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		Test(m_GetDialogsTested);

		Log::Status("TestDialog", "Testing DeleteDialog");

		dialog.DeleteDialog(m_DialogId,
			DELEGATE(TestDialog, OnDeleteDialog, const Json::Value &, this));

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 30.0 && !m_DeleteTested)
		{
			pool.ProcessMainThread();
			tthread::this_thread::yield();
		}
		Test(m_DeleteTested);
	}

	void OnUploadDialog(const Json::Value & value)
	{
		m_DialogId = value["dialog_id"].asString();
		m_UploadTested = true;
	}

	void OnGetServiceStatus(const IService::ServiceStatus & a_Status)
	{
		m_ServiceStatusTested = true;
	}

	void OnConverse(const Json::Value & a_Response)
	{
		Test(!a_Response.isNull());
		m_ConversationId = a_Response["conversation_id"].asInt();
		m_ClientId = a_Response["client_id"].asInt();
		m_ConverseTested = true;
	}

	void OnResponse(const Json::Value & a_Response)
	{
		Test(!a_Response.isNull());
		Test(a_Response["response"].size() > 0);
		Test(a_Response["conversation_id"].asInt() == m_ConversationId);
		Test(a_Response["client_id"].asInt() == m_ClientId);
		m_ResponseTested = true;
	}

	void OnGetDialogs(Dialogs * a_pDialogs)
	{
		Test(a_pDialogs != NULL);
		m_GetDialogsTested = true;
	}

	void OnDeleteDialog(const Json::Value & value)
	{
		m_DeleteTested = true;
	}
};

TestDialog TEST_DIALOG;
