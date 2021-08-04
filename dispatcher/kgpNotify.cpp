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

int ltoi(string level)
{
   if(level == "LOG_EMERG")     return LOG_EMERG;
   if(level == "LOG_ALERT")     return LOG_ALERT;
   if(level == "LOG_CRIT")      return LOG_CRIT;
   if(level == "LOG_ERR")       return LOG_ERR;
   if(level == "LOG_WARNING")   return LOG_WARNING;
   if(level == "LOG_NOTICE")    return LOG_NOTICE;
   if(level == "LOG_INFO")      return LOG_INFO;
   if(level == "LOG_DEBUG")     return LOG_DEBUG;
   return -1;
}

string ltos(int level)
{
    return LogLevels[level];
}

/*   ---- CLASS 'Message' ---  */
Message::Message()
{

}

int Message::ResetNotifiers(int loglevel)
{
    //printf("Reset Notifiers on loglevel: %s\n",ltos(loglevel).c_str());
    return this->TrigNotifiers("",loglevel);
}

int Message::TrigNotifiers(string id, int loglevel)
{
    int triggercount = 0;
    int success;
    string retmsg;

    list<string> dirs = File::Glob(NOTIFIERSDIR "LOG_*");
    for( const string& dir: dirs)
    {
        if(File::DirExists(dir) && (File::GetFileName(dir) == ltos(loglevel)) )
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
    }
    else
    {
        logg << Logger::Error << "Error reading message file" << lend;
    }
    return parsedMsg;

}
int Message::CleanUp()
{
    return this->CleanUp(false);
}

int Message::CleanUp(bool boot)
{
    Json::Value parsedMsg;
    time_t msgtime;
    int countRemoved=0;
    bool persistantmsg=false;
    bool clearonboot=false;

    list<string> messages = File::Glob(HISTORYDIR "*");
    for( const string& message: messages)
    {
        //logg << Logger::Debug << "Read file" << File::GetFileName(message) << lend;
        parsedMsg = this->getFilemsg(message);
        if( parsedMsg.isMember("date") && parsedMsg["date"].isString() ) {
            msgtime = stol(parsedMsg["date"].asString());
            if( time(NULL) > (msgtime + MAX_HISTORY) )
            {
                logg << Logger::Debug << "Removing message " << File::GetFileName(message) << lend;
                File::Delete(message);
                countRemoved++;
            }
        }
        else
        {
            logg << Logger::Debug << "Unable to read date, removing from histroy: " << File::GetFileName(message) << lend;
            File::Delete(message);
            countRemoved++;
        }
    }

    messages = File::Glob(SPOOLDIR "*");
    for( const string& message: messages)
    {
        if(File::FileExists(message))
        {
            //logg << Logger::Debug << "Read file" << File::GetFileName(message) << lend;
            parsedMsg = this->getFilemsg(message);
            if( parsedMsg.isMember("date") && parsedMsg["date"].isString() ) {
                msgtime = stol(parsedMsg["date"].asString());
                if( parsedMsg.isMember("persistant") ) {
                    persistantmsg = parsedMsg["persistant"].asBool();
                }
                if( parsedMsg.isMember("clearonboot") ) {
                    clearonboot = parsedMsg["clearonboot"].asBool();
                }
                if(
                        (!persistantmsg) && (
                            (time(NULL) > (msgtime + MAX_ACTIVE))
                            && (ltoi(parsedMsg["level"].asString()) >= LOG_AUTOREMOVE )
                            || (boot && clearonboot) )
                )
                {
                    logg << Logger::Debug << "Archiving message " << File::GetFileName(message) << lend;
                    File::Move(message, HISTORYDIR+File::GetFileName(message));
                    countRemoved++;
                }
                else
                {
                    logg << Logger::Debug << "Not removing: " << File::GetFileName(message) << lend;
                }
            }
            else
            {
                logg << Logger::Debug << "Unable to read date, archiving" << File::GetFileName(message) << lend;
                File::Move(message, HISTORYDIR+File::GetFileName(message));
                countRemoved++;
            }
        }
    }

    return countRemoved;

}



/*   ---- CLASS 'NewMessage' ---  */

/*   ---  Public functions ---  */

NewMessage::NewMessage()
{
}

NewMessage::NewMessage(string level, string message)
{
    this->Details(level,message,"",false, true);
}

NewMessage::NewMessage(int level, string message)
{
    this->Details(ltos(level),message,"",false, true);
}


void NewMessage::Details(string level, const string& message, string issuer, bool persistant, bool clearonboot)
{
    this->log_level = ltoi(level);
    this->body = message;
    this->persistant = persistant;
    this->id = String::UUID();
    this->date = time(NULL);
    this->issuer = issuer;
    this->clearonboot = clearonboot;
}

int NewMessage::Send()
{
    Json::Value jsonMsg;

    jsonMsg["date"] = to_string(this->date);
    jsonMsg["message"] = this->body;
    jsonMsg["levelText"] = ltos(this->log_level);
    jsonMsg["level"] = this->log_level;
    jsonMsg["issuer"] = this->issuer;
    jsonMsg["id"] = this->id;
    jsonMsg["persistant"] = this->persistant;
    jsonMsg["clearonboot"] = this->clearonboot;

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
    printf("Log level: %s\n",ltos(this->log_level).c_str());
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

int ExistingMessage::Ack()
{
    Json::Value parsedMsg;
    int countTriggers;

    if( (this->id != "") && File::FileExists(SPOOLDIR+this->id) ) {
        parsedMsg = this->getFilemsg(SPOOLDIR+this->id);
        logg << Logger::Debug << "Ack message, archiving to history: " << this->id << lend;
        File::Move(SPOOLDIR+this->id,HISTORYDIR+this->id);
        countTriggers = this->ResetNotifiers(parsedMsg["level"].asInt());
        this->CleanUp();
        return countTriggers;
    }
    else
    {
        return -1;
    }
}


} // END NS


