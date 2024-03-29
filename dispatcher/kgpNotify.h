#ifndef KGPNOTIFY_H
#define KGPNOTIFY_H

#include "Config.h"
#include <syslog.h>
#include <time.h>

#include <unistd.h>
#include <libutils/Logger.h>
#include <libutils/FileUtils.h>
#include <libutils/String.h>
#include <libutils/Process.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Notify
{

using namespace std;

extern const vector<string> LogLevels;

string ltos(int level);
int ltoi(const string& level);

class Message
{
public:
	Message() = default;
    /**
     * @brief Trigger notifiers
     * @return The number of notifiers triggered
     */
	int TrigNotifiers(const string& id, int loglevel);
    int ResetNotifiers(int loglevel);

    /**
     * @brief Read a message 'id' from file
     * @return the message as a Json object
     */
	json getFilemsg(const string& path);

    /**
     * @brief Deletes messages from archive that are older than MAX_DAYS old
     * @return the number of messages deleted
     */
    int CleanUp();
    int CleanUp(bool boot);

};

class NewMessage:public Message
{
private:
    /**
     * @brief Variable to hold the unique identifier for the message
     */
    string id;
    string body;
    int log_level;
    string issuer;
    time_t date;
    bool persistant;
    bool clearonboot;

public:

    /**
     * @brief Creates a new empty message.
     * @return the new object
     */
	NewMessage() = default;

    /**
     * @brief Creates a new message with log_level (as string) and body set
     * @return the new object id in 'id'
     */
    NewMessage(string level, string message);

    /**
     * @brief Creates a new message with log_level (as int) and body set
     * @return the new object id in 'id'
     */
    NewMessage(int level, string message);

    /**
     * @brief Enter details of message
     * @return none
     */
	void Details(const string& level, const string& message, const string &issuer, bool persistant, bool clearonboot);

    /**
     * @brief Write message to spool queue and trigger notifiers
     * @return The number of notifiers triggered
     */
    int Send();

    /**
     * @brief Acknowledge (move to history) the message with
     * @return The number of notifiers triggered
     */
    int Ack();

    /**
     * @brief Get the id of the message
     * @return The message id
     */
    string GetID();

    /**
     * @brief Print the message to std out
     * @return none
     */
    void Dump();

	virtual ~NewMessage() = default;

};

class ExistingMessage:public Message
{
private :
    string id;

public:

    /**
     * @brief Creates a new empty message with uuid set.
     * @return the new objext
     * Use for functions that operates on messages that exist on file.
     */
	ExistingMessage(const string &id);

    /**
     * @brief Acknowledge (move to history) the message
     * @return The number of notifiers triggered
     */
    int Ack();
};

}

#endif // KGPNOTIFY_H

