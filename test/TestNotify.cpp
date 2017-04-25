#include "TestNotify.h"
#include "kgpNotify.h"
#include <json/json.h>
#include <time.h>
#include <syslog.h>
#include "Config.h"
#include <libutils/FileUtils.h>
#include <libutils/Process.h>
#include <libutils/String.h>


CPPUNIT_TEST_SUITE_REGISTRATION(TestNotify);

using namespace Notify;
using namespace Utils;
Json::Reader reader;

string TestNotify::CreateTestMsg()
{
    Json::FastWriter writer;
    Json::Value testmsg;
    string testid;

    testid = String::UUID();
    testmsg["date"] = to_string(time(NULL));
    testmsg["message"] = "Test Message";
    testmsg["level"] = "LOG_WARNING";
    testmsg["issuer"] = "Test system";
    testmsg["id"] = testid;

    File::Write(SPOOLDIR+testid, writer.write(testmsg).c_str(),0660);
    return testid;
}

void TestNotify::setUp()
{
    // Divert logger to syslog
    openlog( LIB_NAME, LOG_PERROR, LOG_DAEMON);
    logg.SetOutputter( [](const string& msg){ syslog(LOG_INFO, "%s",msg.c_str());});
    logg.SetLogName("");

    // make sure dirs for testdata exists
    if( ! File::DirExists(SPOOLDIR))
    {
        File::MkDir(SPOOLDIR,0776);
    }
    if( ! File::DirExists(HISTORYDIR))
    {
        File::MkDir(HISTORYDIR,0776);

    }
}

void TestNotify::tearDown()
{
    Process::Exec("rm -rf " SPOOLDIR);
    Process::Exec("rm -rf " HISTORYDIR);
}

void TestNotify::testNewMsg()
{
    string id;
    NewMessage NewMsg;
    id = NewMsg.GetID();
    CPPUNIT_ASSERT_MESSAGE("ID should be empty",id=="");
}

void TestNotify::testContentMsg()
{
    string id;
    Json::Value parsedMsg;
    string testmessage = "Alert message";
    int countTriggered;

    NewMessage Msg(ltos(LOG_ALERT),testmessage);
    countTriggered = Msg.Send();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect number of notifiers triggered",VALID_ALERT_NOTIFIERS,countTriggered);
    id = Msg.GetID();
    CPPUNIT_ASSERT_MESSAGE("ID shoule not be empty",id != "");
    parsedMsg = Msg.getFilemsg(SPOOLDIR+id);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect message body",testmessage,parsedMsg["message"].asString());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect message integer loglevel",LOG_ALERT,parsedMsg["level"].asInt());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect message loglevel",ltos(LOG_ALERT),parsedMsg["levelText"].asString());
}

void TestNotify::testDetailedMsg()
{
    string id;
    Json::Value parsedMsg;
    string testmessage = "Test Error message";
    string issuer = "Donald Duck";


    NewMessage Msg;
    Msg.Details(ltos(LOG_ERR),testmessage,issuer,true,true);
    Msg.Send();
    id = Msg.GetID();

    CPPUNIT_ASSERT_MESSAGE("ID shoule not be empty",id != "");
    parsedMsg = Msg.getFilemsg(SPOOLDIR+id);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect persistance",true,parsedMsg["persistant"].asBool());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect issuer",issuer,parsedMsg["issuer"].asString());
}

void TestNotify::testNoticeMsg()
{
    string id;
    Json::Value parsedMsg;
    string testmessage = "Notice message";
    int countTriggered;

    NewMessage Msg(ltos(LOG_NOTICE),testmessage);
    countTriggered = Msg.Send();

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect number of notifiers triggered",VALID_NOTICE_NOTIFIERS,countTriggered);

    id = Msg.GetID();
    CPPUNIT_ASSERT_MESSAGE("ID shoule not be empty",id != "");
    parsedMsg = Msg.getFilemsg(SPOOLDIR+id);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect message body",testmessage,parsedMsg["message"].asString());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect message loglevel",ltos(LOG_NOTICE),parsedMsg["levelText"].asString());
}

void TestNotify::testExistingMsg()
{
    string msgid;

    msgid= this->CreateTestMsg();
    ExistingMessage msgUUID(msgid);
    CPPUNIT_ASSERT_MESSAGE("Unable to read test message",Utils::File::FileExists(SPOOLDIR+msgid));
}

void TestNotify::testAckMsg()
{
    string id;
    string testmessage = "Notice message";
    int countTriggered;

    NewMessage NewMsg(ltos(LOG_NOTICE),testmessage);
    id = NewMsg.GetID();
    NewMsg.Send();

    ExistingMessage Msg(id);
    countTriggered = Msg.Ack();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect number of notifiers triggered",VALID_NOTICE_NOTIFIERS,countTriggered);
}

void TestNotify::testAckNoMsg()
{
    string id;
    int countTriggered;

    id = String::UUID();
    ExistingMessage Msg(id);
    countTriggered = Msg.Ack();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect number of notifiers triggered",-1,countTriggered);
}


void TestNotify::testCleanup()
{
    string testmessage;
    string aid,hid;
    time_t oldtime;
    int removed;

    // write a message that is older than MAX_ACTIVE time
    aid = String::UUID();
    oldtime = time(NULL) - ACTIVETIME;
    testmessage = "{\"date\":\""+to_string(oldtime)+"\",\"id\":\""+aid+"\",\"issuer\":\"\",\"level\":\"LOG_NOTICE\",\"message\":\"Notice message\",\"persistant\":false,\"clearonboot\":true}";
    //printf("MSG: %s\n",testmessage.c_str());
    File::Write(SPOOLDIR "/"+aid,testmessage,0660);

    // write a message that is older than MAX_HISTORY time
    hid = String::UUID();
    oldtime = time(NULL) - HISTORYTIME;
    testmessage = "{\"date\":\""+to_string(oldtime)+"\",\"id\":\""+hid+"\",\"issuer\":\"\",\"level\":\"LOG_NOTICE\",\"message\":\"Notice message\",\"persistant\":false,\"clearonboot\":true}";
    //printf("MSG: %s\n",testmessage.c_str());
    File::Write(HISTORYDIR+hid,testmessage,0660);

    Notify::Message Msg;
    removed = Msg.CleanUp();
    CPPUNIT_ASSERT_MESSAGE("'History'' message not removed",Utils::File::FileExists(HISTORYDIR+hid) == false);

    CPPUNIT_ASSERT_MESSAGE("'Archive' message still in SPOOLDIR",Utils::File::FileExists(SPOOLDIR+aid) == false);
    CPPUNIT_ASSERT_MESSAGE("'Archive' message not in HISTORYDIR",Utils::File::FileExists(HISTORYDIR+aid));


    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect number of messages deleted",REMOVEHISTCOUNT+ARCHIVECOUNT,removed);
}

void TestNotify::testCleanupPersistance()
{
    string testmessage;
    string pid;
    time_t oldtime;
    int removed;

    // write a message that is older than MAX_ACTIVE time but with persistant set to 'true'
    pid = String::UUID();
    oldtime = time(NULL) - ACTIVETIME;
    testmessage = "{\"date\":\""+to_string(oldtime)+"\",\"id\":\""+pid+"\",\"issuer\":\"\",\"level\":\"LOG_NOTICE\",\"message\":\"Notice message\",\"persistant\":true,\"clearonboot\":true}";
    //printf("MSG: %s\n",testmessage.c_str());
    File::Write(SPOOLDIR "/"+pid,testmessage,0660);

    Notify::Message Msg;
    removed = Msg.CleanUp();

    CPPUNIT_ASSERT_MESSAGE("'Persistant' message not in SPOOLDIR",Utils::File::FileExists(SPOOLDIR+pid));

}

void TestNotify::testCleanupLevel()
{
    string nid,wid;
    time_t oldtime;
    string testmessage = "Notice message, should be removed";
    int removed;

    // write a message that is older than MAX_ACTIVE time and with level set to "LOG_NOTICE"
    nid = String::UUID();
    oldtime = time(NULL) - ACTIVETIME;
    testmessage = "{\"date\":\""+to_string(oldtime)+"\",\"id\":\""+nid+"\",\"issuer\":\"\",\"level\":\"LOG_NOTICE\",\"message\":\"Notice message\",\"persistant\":false,\"clearonboot\":true}";
    //printf("MSG: %s\n",testmessage.c_str());
    File::Write(SPOOLDIR "/"+nid,testmessage,0660);


    // write a message that is older than MAX_ACTIVE time but with level higher than "LOG_AUTOREMOVE"
    wid = String::UUID();
    oldtime = time(NULL) - ACTIVETIME;
    testmessage = "{\"date\":\""+to_string(oldtime)+"\",\"id\":\""+wid+"\",\"issuer\":\"\",\"level\":\"LOG_WARNING\",\"message\":\"Notice message\",\"persistant\":false,\"clearonboot\":true}";
    //printf("MSG: %s\n",testmessage.c_str());
    File::Write(SPOOLDIR "/"+wid,testmessage,0660);

    Notify::Message Msg;
    removed = Msg.CleanUp();

    CPPUNIT_ASSERT_MESSAGE("'Notice' message still in SPOOLDIR",Utils::File::FileExists(SPOOLDIR+nid) == false);

    CPPUNIT_ASSERT_MESSAGE("'Warning' message not in SPOOLDIR",Utils::File::FileExists(SPOOLDIR+wid));
}

void TestNotify::testCleanupBoot()
{
    string id1,id2;
    int removed;

    NewMessage KeepMsg;
    KeepMsg.Details(ltos(LOG_NOTICE),"Keep on boot","Test system",false,false);
    id1 = KeepMsg.GetID();
    KeepMsg.Send();

    NewMessage ClearMsg;
    ClearMsg.Details(ltos(LOG_NOTICE),"Remove on boot","Test system",false,true);
    id2 = ClearMsg.GetID();
    ClearMsg.Send();

    removed = ClearMsg.CleanUp(true);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Incorrect number of messages removed",1,removed);

    CPPUNIT_ASSERT_MESSAGE("'Keep' message not in SPOOLDIR",Utils::File::FileExists(SPOOLDIR+id1));

    CPPUNIT_ASSERT_MESSAGE("'Remove' message still in SPOOLDIR",Utils::File::FileExists(SPOOLDIR+id2) == false);

}
