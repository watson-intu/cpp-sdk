add_definitions(" -DAUDIOIMPL_IS_REMOTE -DNAO_ENABLED -DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")
qi_create_lib(wdc SHARED
	services/Dialog/Dialog.cpp
	services/Alchemy/Alchemy.cpp
	services/NaturalLanguageClassifier/NaturalLanguageClassifier.cpp
	services/RelationshipExtraction/RelationshipExtraction.cpp
	services/SpeechToText/SpeechToText.cpp
	services/TextToSpeech/TextToSpeech.cpp
	services/IService.cpp
	tests/TestAlchemy.cpp
	tests/TestDataCache.cpp
    tests/TestDelegate.cpp
    tests/TestFactory.cpp
	tests/TestGetMac.cpp
    tests/TestISerializable.cpp
    tests/TestRTTI.cpp
	tests/TestSpeechToText.cpp
	tests/TestTextToSpeech.cpp
	tests/TestNaturalLanguageClassifier.cpp
    tests/TestThreadPool.cpp
	tests/TestTimerPool.cpp
	tests/TestURL.cpp
	tests/TestWebClient.cpp
	tests/TestWebServer.cpp
    tests/UnitTest.cpp
    utils/Config.cpp
	utils/DataCache.cpp
	utils/Form.cpp
	utils/GetMac.cpp
    utils/ISerializable.cpp
    utils/Library.cpp
    utils/Log.cpp
    utils/Sound.cpp
    utils/StringUtil.cpp
    utils/ThreadPool.cpp
    utils/TimerPool.cpp
	utils/Time.cpp
	utils/URL.cpp
	utils/UniqueID.cpp
	utils/WebClient.cpp
	utils/WebSocketFramer.cpp
        )
if (CMAKE_HOST_APPLE)
	message(STATUS "WDC: Running on Apple")
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl)
else (CMAKE_HOST_APPLE)
	message(STATUS "WDC: NOT running on Apple")
	qi_use_lib(wdc dl jsoncpp tinythread++ crossguid base64 boost boost_filesystem boost_thread boost_system openssl rt)
endif (CMAKE_HOST_APPLE)
qi_stage_lib(wdc)
