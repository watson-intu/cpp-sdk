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
#include "utils/Delegate.h"
#include "services/SpeechToText/SpeechToText.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"
#include <iterator>
#include <iostream>
#include <fstream>

class TestSTTResponseTime : UnitTest
{
public:
	//! Construction
	TestSTTResponseTime() : 
		UnitTest( "TestSTTResponseTime" ),
		m_ChunkSz( 4000 ),
		m_Bits( 16 ),
		m_Channels( 1 ),
		m_Rate( 16000 ),
		m_bComplete( false ),
		m_fStartTime( 0.0f ),
		m_fResponseTime( 0.0f ),
		m_Samples( 20 )
	{}

	int						m_ChunkSz;
	int						m_Bits;
	int						m_Channels;
	int						m_Rate;
	int						m_Samples;
	bool 					m_bComplete;
	double					m_fStartTime;
	double					m_fResponseTime;

	virtual void RunTest()
	{
		bool F = false;
		double spin_time = float(m_ChunkSz) / ( 2.0 * float(m_Rate) );

		// Start STT service
		Config config;
		Log::Debug("TestSTTResponseTime", "Deserializing service configs");		
		Test( ISerializable::DeserializeFromFile( "./etc/tests/unit_test_config.json", &config ) != NULL );
		ThreadPool pool(1);

		SpeechToText stt;
		Log::Debug("TestSTTResponseTime", "Starting STT service");		
		Test( stt.Start() );

		// Spin...
		Spin( F, 1.0 );

		std::vector<std::string> models;
		models.push_back( "en-US_BroadbandModel" );
		stt.SetRecognizeModels( models );
		
		Log::Debug("TestSTTResponseTime", "Opening websocket");
		Test( stt.StartListening( DELEGATE(
			TestSTTResponseTime, OnResponse, RecognizeResults *, this ) ) );

		// Spin...
		Spin( F, 1.0 );

		//! Get all raw files from folder of interest
		Log::Debug("TestSTTResponseTime", "Loading all raw audio files...");
		std::string dir = "./etc/tests/stt-timing-samples";
        std::vector<std::string> audioTestFiles;
	    boost::filesystem::directory_iterator end_iter;
	    for ( boost::filesystem::directory_iterator dir_itr( dir ); dir_itr != end_iter; ++dir_itr )
    	{
	    	try
		    {
			    if ( boost::filesystem::is_regular_file( dir_itr->status() ) )
			    {
			    	std::cout << "Importing raw audio file: " << dir_itr->path().filename() << "\n";
				    audioTestFiles.push_back(dir_itr->path().filename().string());
			    }
		    }
		    catch ( const std::exception & ex )
		    {
			    Log::Status("TestSTTResponseTime", "Caught Exception while getting file names: %s", ex.what() );
		    }
		}

		//! Setup CSV and run test N times
		std::ofstream csv_writer("STT_Response_Time.csv");

		for (int k = 0; k < m_Samples; k++)
		{
			Log::Debug("TestSTTResponseTime", "Running trial number: %d of %d", k, m_Samples);
			csv_writer << Time().GetFormattedTime("%d-%m-%Y %H:%M:%S") << ',';
			Log::Debug("TestSTTResponseTime", "Time stamped new row in csv");
									
			//! Run test on each sample
			for ( std::vector<std::string>::const_iterator it = audioTestFiles.begin(); it != audioTestFiles.end(); it++ )
			{
				// Will time from the beginning of the audio clip and manually adjust for the actual end of speech in each file...
				m_fStartTime = Time().GetEpochTime();			
				m_bComplete = false;

				std::string fullPath = dir + "/" + (*it);
				FILE * pFile = fopen( fullPath.c_str(), "rb" );
				fseek (pFile , 0 , SEEK_END);
				int sz = ftell (pFile);
				rewind (pFile);
				Log::Debug("TestSTTResponseTime", "Loading raw audio: %s, size: %d", fullPath.c_str(), sz );
				
				int i = 0;
				char buffer[m_ChunkSz];
				while ( i < sz )
				{
					// Create SpeechAudioData chunk
					SpeechAudioData temp;
					temp.m_Bits = m_Bits;
					temp.m_Channels = m_Channels;
					temp.m_Rate = m_Rate;
					temp.m_Level = 0.5;

					// Read from buffer
					int read = fread(buffer, sizeof(char), sizeof(buffer) / sizeof(char) , pFile );

					if (read == m_ChunkSz )
					{
						temp.m_PCM = std::string(buffer, m_ChunkSz);				
					}
					else if ( read > 0)
					{
						std::string pcm = std::string(buffer, read);
						pcm.append(m_ChunkSz - read, '\0'); 
						temp.m_PCM = pcm;                 
					}
					// Send to STT and spin
					stt.OnListen(temp);
					Spin( F, spin_time );

					i += read;
				}

				// Start time and send empty audio until final response
				Log::Debug("TestSTTResponseTime", "Finished actual audio - sending empty and spinning until response");
				while ( !m_bComplete )
				{
					// Create SpeechAudioData chunk
					SpeechAudioData temp;
					temp.m_Bits = m_Bits;
					temp.m_Channels = m_Channels;
					temp.m_Rate = m_Rate;
					temp.m_Level = 0.5;
					temp.m_PCM = std::string(m_ChunkSz, '\0');

					stt.OnListen(temp);
					Spin( F, spin_time );
				}  
				csv_writer << m_fResponseTime << ',';

				fclose( pFile );
			}
			csv_writer << '\n';
		}
		csv_writer.close();
	}

	void OnResponse( RecognizeResults * a_pResults )
	{
		Log::Debug("TestSTTResponseTime", "OnResponse");
		if( a_pResults != NULL && a_pResults->m_Results.size() > 0 )
		{
			for( size_t k=0;k<a_pResults->m_Results.size() && !m_bComplete;++k)
			{
				const std::vector<SpeechAlt> & alternatives = a_pResults->m_Results[k].m_Alternatives;
				for(size_t i=0;i<alternatives.size();++i)
				{
					const std::string & transcript = alternatives[i].m_Transcript;
					double confidence = alternatives[i].m_Confidence;
					Log::Status("Conversation", "----- Recognized Text: %s (%g)", transcript.c_str(), confidence) ;
				}
				if( a_pResults->m_Results[k].m_Final )
				{
					m_bComplete = true;
					m_fResponseTime = Time().GetEpochTime() - m_fStartTime;
					Log::Debug("TestSTTResponseTime", "Latency was: %.3f", m_fResponseTime);
				}
			}
		}
	}

};

TestSTTResponseTime TEST_STT_RESPONSE_TIME;

