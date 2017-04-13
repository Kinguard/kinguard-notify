#include "kgpNotify.h"

using namespace Utils;
using namespace std;

namespace Notify {

const vector<string> LogLevels {
    "LOG_EMERG",
    "LOG_ALERT",
    "LOG_CRIT",
    "LOG_ERR",
    "LOG_WARNING",
    "LOG_NOTICE",
    "LOG_INFO",
    "LOG_DEBUG"
};

/*   ---- CLASS 'Message' ---  */
int Message::ResetNotifiers(string loglevel)
{
    //printf("Reset Notifiers on loglevel: %s\n",loglevel.c_str());
    return this->TrigNotifiers("",loglevel);
}

int Message::TrigNotifiers(string id, string loglevel)
{
    int triggercount = 0;
    int success;
    string retmsg;

    list<string> dirs = File::Glob(NOTIFIERSDIR "LOG_*");
    for( const string& dir: dirs)
    {
        if(File::DirExists(dir) && (File::GetFileName(dir) == loglevel) )
        {
            list<string> notifiers = File::Glob(dir+"/*");
            for( const string& notifier: notifiers)
            {
                //printf("Notifier '%s'' in Dir '%s', id: %s\n",notifier.c_str(),File::GetFileName(dir).c_str(),id.c_str());
                tie(success,retmsg) = Process::Exec(notifier+" "+id);
                if ( success ) {
                    triggercount++;
                }
                else
                {
                    logg << Logger::Warning << "Notifier '"<< File::GetFileName(notifier) <<"' did not exit successfully." << lend;
                }


            }
        }

    }
    return triggercount;
}
Json::Value Message::getFilemsg(string id) {
    string jsonMsg;
    Json::Value parsedMsg;
    bool parsingSuccessful;

    if(File::FileExists(id))
    {
        jsonMsg = File::GetContentAsString(id);
        parsingSuccessful =  reader.parse(jsonMsg,parsedMsg);
        return parsedMsg;
    }
    else
    {
        logg << Logger::Error << "Error reading message file" << lend;
    }

}

/*   ---- CLASS 'NewMessage' ---  */

/*   ---  Public functions ---  */

NewMessage::NewMessage()
{
}

NewMessage::NewMessage(string level, string message)
{
    this->Details(level,message,"",false);
}

NewMessage::NewMessage(int level, string message)
{
    this->Details(LogLevels[level],message,"",false);
}


void NewMessage::Details(string level, const string& message, string issuer, bool persistant)
{
    this->log_level = level;
    this->body = message;
    this->persistant = persistant;
    this->id = String::UUID();
    this->date = time(NULL);
    this->issuer = issuer;
}

int NewMessage::Send()
{
    Json::Value jsonMsg;

    jsonMsg["date"] = to_string(this->date);
    jsonMsg["message"] = this->body;
    jsonMsg["level"] = this->log_level;
    jsonMsg["issuer"] = this->issuer;
    jsonMsg["id"] = this->id;
    jsonMsg["persistant"] = this->persistant;

    File::Write(SPOOLDIR+id, writer.write(jsonMsg).c_str(),0660);
    if (File::FileExists(SPOOLDIR+id))
    {
        return this->TrigNotifiers(id,this->log_level);
    }
    else
    {
        logg << Logger::Crit << "Failed to write message " << this->id << " to message queue."<< lend;
        return false;
    }

}


string NewMessage::GetID()
{
    return this->id;
}

void NewMessage::Dump()
{
    printf("ID: %s\n",this->id.c_str());
    printf("Message:\n%s\n",this->body.c_str());
    printf("Time: %d\n",(int)this->date);
    printf("Issuer: %s\n",this->issuer.c_str());
    printf("Log level: %s\n",this->log_level.c_str());
}

NewMessage::~NewMessage()
{

}

/* -------- Existing Message Class ------------  */

ExistingMessage::ExistingMessage(string id)
{
    this->id = id;
}

/* --- Private  --- */

/* --- Public  --- */

int ExistingMessage::CleanUp()
{
    Json::Value parsedMsg;
    time_t msgtime;
    int countRemoved=0;
    bool persistantmsg;

    list<string> messages = File::Glob(HISTORYDIR "*");
    for( const string& message: messages)
    {
        //logg << Logger::Debug << "Read file" << File::GetFileName(message) << lend;
        parsedMsg = this->getFilemsg(message);
        msgtime = stol(parsedMsg["date"].asString());
        if( time(NULL) > (msgtime + MAX_HISTORY) )
        {
            logg << Logger::Debug << "Removing message " << File::GetFileName(message) << lend;
            File::Delete(message);
            countRemoved++;
        }

    }

    messages = File::Glob(SPOOLDIR "*");
    for( const string& message: messages)
    {
        //logg << Logger::Debug << "Read file" << File::GetFileName(message) << lend;
        parsedMsg = this->getFilemsg(message);
        msgtime = stol(parsedMsg["date"].asString());
        persistantmsg = parsedMsg["persistant"].asBool();
        if(persistantmsg)
        {
            logg << Logger::Debug << "Do not remove: " << File::GetFileName(message) << lend;
        }
        if( (time(NULL) > (msgtime + MAX_ACTIVE)) && (!persistantmsg) )
        {
            logg << Logger::Debug << "Archiving message " << File::GetFileName(message) << lend;
            File::Move(message, HISTORYDIR+File::GetFileName(message));
            countRemoved++;
        }

    }

    return countRemoved;

}
int ExistingMessage::Ack()
{
    Json::Value parsedMsg;
    int countTriggers;

    if( (this->id != "") && File::FileExists(SPOOLDIR+this->id) ) {
        parsedMsg = this->getFilemsg(SPOOLDIR+this->id);
        File::Move(SPOOLDIR+this->id,HISTORYDIR+this->id);
        countTriggers = this->ResetNotifiers(parsedMsg["level"].asString());
        this->CleanUp();
        return countTriggers;
    }
    else
    {
        return 0;
    }
}


} // END NS


