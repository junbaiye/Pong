// Jeney Lao - 83093656
// Junbai Ye - 87724993

/*
	MILESTONE 5

	?        Milestone 1:
	The physics engine that tracks the ball’s movement was calculated by a code that 
	Junbai included. Jeney worked on recording the connected player’s ID. We both had 
	the IP address and port of the game server to connect. Jeney added the text field 
	to enter the user’s game ID. Jeney also kept track of the number of consecutive 
	hits the player makes, along with the number of hits versus the number of total 
	times the ball was presented to the player; she made this display on the screen. 
	Junbai worked on denying server to incoming requests to start a game when one 
	game is already in progress.

	?        Milestone 2:
	From Milestone 1, the text fields to enter the IP address and port of the game 
	server to connect to, along with the text field to enter the user’s game ID, 
	were already present. The scoreboard, implemented by Jeney, shows the IDs and 
	scores of the two players. However, the scoreboard does not actually work with 
	two players. One of the player’s is played by the computer, which is not what 
	this milestone demands. Junbai incorporate a physics engine that tracks the ball’s 
	movement and tried to implement stopping the game when one of the players leave, 
	but it wasn’t successful during this time.

	?        Milestone 3:
	Junbai successfully coded the server to stop the game from running when one of 
	the players leave. The players can also play against each other (worked on by 
	both Jeney and Junbai.) Junbai also added artificial latency to each message 
	being sent or received. It was random, with there being 1 to 10 seconds delay 
	to each message. Jeney calculated the latency between the server and the clients, 
	although it wasn’t completely accurate.

	?        Milestone 4:
	Junbai improved the pong ball’s physics and movement. The latency calculated by 
	Jeney wasn’t completely accurate, but it did not need synchronization between 
	machines since it did not calculate a result between two different system times.

	?        Milestone 5:
	Jeney improved the latency calculated between the server and the clients. It is 
	more accurate than the latency calculated in Milestone 4.

	Jeney and Junbai both worked on the summary together.
*/


#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include "websocket.h"

#include <Windows.h>
#include <stdio.h>

#include <chrono>
#include <queue>

using namespace std;

// GLOBAL VARIABLES HERE
webSocket server;
int start = 0;
int ballX = 200, ballY = 200, ballxs = 1, ballys = 0;

int player1Pos = 175, player2Pos = 175;
int player1Speed = 0, player2Speed = 0, player1Score = 0, player2Score = 0;

std::string clientID1, clientID2;

float pingTotal1, pingTotal2, pingAverage1, pingAverage2;
int pingCount1, pingCount2;

auto startTime1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
auto endTime1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
auto startTime2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
auto endTime2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

time_t clientSTime1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
time_t clientSTime2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

vector<std::string> messages;

struct delayMessage
{
	time_t timeStamp;
	std::string t1;
	long long clientTime;
	int des;
	string message;
};

queue<delayMessage> delayM, newM;
//queue<delayMessage> newM;

// NTP
#define ReverseEndianInt(x) ((x) = \
    ((x)&0xff000000) >> 24 |       \
    ((x)&0x00ff0000) >> 8  |       \
    ((x)&0x0000ff00) << 8  |       \
    ((x)&0x000000ff) << 24)

/**
* NTP Fixed-Point Timestamp Format.
* From [RFC 5905](http://tools.ietf.org/html/rfc5905).
*/
struct Timestamp {
	unsigned int seconds;   /**< Seconds since Jan 1, 1900. */
	unsigned int fraction;  /**< Fractional part of seconds. Integer number of 2^-32 seconds. */

	/**
	* Reverses the Endianness of the timestamp.
	* Network byte order is big endian, so it needs to be switched before
	* sending or reading.
	*/
	void ReverseEndian(void);

	/**
	* Convert to time_t.
	* Returns the integer part of the timestamp in unix time_t format,
	* which is seconds since Jan 1, 1970.
	*/
	time_t to_time_t(void);
};

/**
* A Network Time Protocol Message.
* From [RFC 5905](http://tools.ietf.org/html/rfc5905).
*/
struct NTPMessage {
	unsigned char mode : 3;           /**< Mode of the message sender. 3 = Client, 4 = Server */
	unsigned char version : 2;        /**< Protocol version. Should be set to 3. */
	unsigned char leap : 2;           /**< Leap seconds warning. See the [RFC](http://tools.ietf.org/html/rfc5905#section-7.3) */
	unsigned char stratum;          /**< Servers between client and physical timekeeper. 1 = Server is Connected to Physical Source. 0 = Unknown. */
	unsigned char poll;             /**< Max Poll Rate. In log2 seconds. */
	unsigned char precision;        /**< Precision of the clock. In log2 seconds. */
	unsigned int sync_distance;     /**< Round-trip to reference clock. NTP Short Format. */
	unsigned int drift_rate;        /**< Dispersion to reference clock. NTP Short Format. */
	unsigned char ref_clock_id[4];  /**< Reference ID. For Stratum 1 devices, a 4-byte string. For other devices, 4-byte IP address. */
	Timestamp ref;                  /**< Reference Timestamp. The time when the system clock was last updated. */
	Timestamp orig;                 /**< Origin Timestamp. Send time of the request. Copied from the request. */
	Timestamp rx;                   /**< Recieve Timestamp. Reciept time of the request. */
	Timestamp tx;                   /**< Transmit Timestamp. Send time of the response. If only a single time is needed, use this one. */


	/**
	* Reverses the Endianness of all the timestamps.
	* Network byte order is big endian, so they need to be switched before
	* sending and after reading.
	*
	* Maintaining them in little endian makes them easier to work with
	* locally, though.
	*/
	void ReverseEndian(void);

	/**
	* Recieve an NTPMessage.
	* Overwrites this object with values from the received packet.
	*/
	int recv(int sock);

	/**
	* Send an NTPMessage.
	*/
	int sendto(int sock, struct sockaddr_in* srv_addr);

	/**
	* Zero all the values.
	*/
	void clear();
};


void Timestamp::ReverseEndian(void) {
	ReverseEndianInt(seconds);
	ReverseEndianInt(fraction);
}

time_t Timestamp::to_time_t(void) {
	return (seconds - ((70 * 365 + 17) * 86400)) & 0x7fffffff;
}

void NTPMessage::ReverseEndian(void) {
	ref.ReverseEndian();
	orig.ReverseEndian();
	rx.ReverseEndian();
	tx.ReverseEndian();
}

int NTPMessage::recv(int sock) {
	int ret = ::recv(sock, (char*)this, sizeof(*this), 0);
	ReverseEndian();
	return ret;
}

int NTPMessage::sendto(int sock, struct sockaddr_in* srv_addr) {
	ReverseEndian();
	int ret = ::sendto(sock, (const char*)this, sizeof(*this), 0, (sockaddr*)srv_addr, sizeof(*srv_addr));
	ReverseEndian();
	return ret;
}

void NTPMessage::clear() {
	memset(this, 0, sizeof(*this));
}
void dns_lookup(const char *host, sockaddr_in *out)
{
	struct addrinfo *result;
	int ret = getaddrinfo(host, "ntp", NULL, &result);
	for (struct addrinfo *p = result; p; p = p->ai_next)
	{
		if (p->ai_family != AF_INET)
			continue;

		memcpy(out, p->ai_addr, sizeof(*out));
	}
	freeaddrinfo(result);
}

void delaySend(int des, string message)
{
	int random = rand() % 10 + 1;
	delayMessage temp;
	temp.timeStamp = time(NULL) + random;
	temp.des = des;

	if (message.substr(0, message.find(":")) == "paddle")
	{
		std::size_t found = message.find(":");
		found = message.find(":", found + 1);
		found = message.find(":", found + 1);

		temp.message = message.substr(0, found);
		message = message.substr(found + 1);
		temp.t1 = message.substr(0, message.find(":"));
		message = message.substr(message.find(":") + 1);
		temp.clientTime = stoll(message.c_str());
	}
	else
	{
		temp.message = message;
	}
	delayM.push(temp);
}

/* called when a client connects */
void openHandler(int clientID){
    ostringstream os;
    os << "Stranger " << clientID << " has joined.";

    vector<int> clientIDs = server.getClientIDs();
    for (int i = 0; i < clientIDs.size(); i++){
        if (clientIDs[i] != clientID)
            server.wsSend(clientIDs[i], os.str());
    }

	if (clientID != 0)
		if (clientID != 1)
			server.wsClose(clientID);
	server.wsSend(clientID, "Welcome!");

	if (clientIDs.size() == 2)
	{
		server.wsSend(clientIDs[0], "go1:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":1:0");
		server.wsSend(clientIDs[1], "go2:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":1:0");
		start = 1;
	}
}

/* called when a client disconnects */
void closeHandler(int clientID){
    ostringstream os;
	if (clientID == 0)
	{
		os << clientID1 << " has left." ;
	}
	else
	{
		os << clientID2 << " has left.";
	}

    vector<int> clientIDs = server.getClientIDs();
    for (int i = 0; i < clientIDs.size(); i++){
        if (clientIDs[i] != clientID)
            server.wsSend(clientIDs[i], os.str());
    }
	cout << clientIDs.size() << endl;
	if (clientIDs.size() == 1)
	{
		server.wsSend(clientID, "stop");
		start = 0;
	}
	start = 0;
    player1Pos = 175, player2Pos = 175;

	player1Score = 0, player2Score = 0;
}

/* called when a client sends a message to the server */
void messageHandler(int clientID, string message){
	ostringstream os;

	if (clientID == 0)
	{
		os << clientID1 << " says: " << message;
	}
	else
	{
		os << clientID2 << " says: " << message;
	}

	if (message.substr(0, message.find(":")) == "T2")
	{
		message = message.substr(message.find(":") + 1);
		if (clientID == 0)
		{
			pingCount1++;
			endTime1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			pingTotal1 += ((endTime1 - startTime1 - stoll(message))/2);
		}
		else if (clientID = 1)
		{
			pingCount2++;
			endTime2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			pingTotal2 += ((endTime2 - startTime2 - stoll(message))/2);
		}
		if (pingCount1 == 50)
		{
			pingAverage1 = pingTotal1 / pingCount1;
			pingCount1 = 0;
			pingTotal1 = 0;
			std::cout << clientID1 << "'s PING is: " << pingAverage1 << std::endl;
		}
		if (pingCount2 == 50)
		{
			pingAverage2 = pingTotal2 / pingCount2;
			pingCount2 = 0;
			pingTotal2 = 0;
			std::cout << clientID2 << "'s PING is: " << pingAverage2 << std::endl;
		}
	}
	else
	{
		if (message.substr(0, message.find(":")) == "1")
		{
			message = message.substr(message.find(":") + 1);
			player1Pos = atoi(message.substr(0, message.find(":")).c_str());
			message = message.substr(message.find(":") + 1);
			clientSTime1 = stoll(message.c_str());
			delaySend(1, "paddle:1:" + to_string(player1Pos) + ":" + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) +
				":" +  to_string(clientSTime1));

		}
		else if (message.substr(0, message.find(":")) == "2")
		{
			message = message.substr(message.find(":") + 1);
			player2Pos = atoi(message.substr(0, message.find(":")).c_str());
			message = message.substr(message.find(":") + 1);
			clientSTime2 = stoll(message.c_str());
			delaySend(0, "paddle:2:" + to_string(player2Pos) + ":" + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) +
				":" + to_string(clientSTime2));

		}
		else if (message.substr(0, message.find(":")) == "NAME")
		{
			message = message.substr(message.find(":") + 1);
			if (clientID == 0)
			{
				clientID1 = message;
			}
			else if (clientID == 1)
			{
				clientID2 = message;
			}
		}
		else if (message.substr(0, message.find(":")) == "1s")
		{
			message = message.substr(message.find(":") + 1);
			player1Speed = atoi(message.c_str());
		}
		else if (message.substr(0, message.find(":")) == "2s")
		{
			message = message.substr(message.find(":") + 1);
			player2Speed = atoi(message.c_str());
		}
		else
		{
			vector<int> clientIDs = server.getClientIDs();
			for (int i = 0; i < clientIDs.size(); i++){
				 if (clientIDs[i] != clientID)
				  server.wsSend(clientIDs[i], os.str());
			}
		}
	}
}

/* called once per select() loop */
void periodicHandler(){
    static time_t next = time(NULL) + 1;
    time_t current = time(NULL);

	if (current >= next){
		ostringstream os;
		string timestring = ctime(&current);
		timestring = timestring.substr(0, timestring.size() - 1);
		os << timestring;

		vector<int> clientIDs = server.getClientIDs();
		for (int i = 0; i < clientIDs.size(); i++){
			if (start == 1)
			{
				ballX += ballxs * 30;
				ballY += ballys * 30;

					if (ballY > 395) //hit the top
					{
						ballY = 395;
						ballys = -ballys;

						//server.wsSend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						//server.wsSend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						//	delaySend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						//	delaySend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));

						}
					if (ballY < 5) //hit the btm
					{
						ballY = 5;
						ballys = -ballys;

						//server.wsSend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						//server.wsSend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						//delaySend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						//delaySend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));

						}

					if (ballX < 30)
					{
						if (player1Pos < ballY  && ballY < player1Pos + 50) //player1 hit the ball
						{
							ballxs = 1;
							ballys += (player1Speed / 2);
							ballX += ballxs * 30;

							//server.wsSend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//server.wsSend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));

							}
						else // player1 miss
						{
							ballX = 200;
							ballY = 200;
							ballxs = 1;
							ballys = 0;

							player2Score++;
							//server.wsSend(clientIDs[0], clientID1 + "'s score is " + std::to_string(player1Score));
							//server.wsSend(clientIDs[0], clientID2 + "'s score is " + std::to_string(player2Score));
							//server.wsSend(clientIDs[1], clientID1 + "'s score is " + std::to_string(player1Score));
							//server.wsSend(clientIDs[1], clientID2 + "'s score is " + std::to_string(player2Score));

							//server.wsSend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//server.wsSend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));

							cout << clientID1 << "'s score is " << player1Score << "." << endl;
							cout << clientID2 << "'s score is " << player2Score << "." << endl;
						}
					}
					
					if (ballX > 370)
					{
						if (player2Pos < ballY  && ballY < player2Pos + 50) //player2 hit the ball
						{
							ballxs = -1;
							ballys += (player2Speed / 2);
							ballX += ballxs * 30;

							//server.wsSend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//server.wsSend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						
						}
						else //player2 miss
						{
							ballX = 200;
							ballY = 200;
							ballxs = 1;
							ballys = 0;

							player1Score++;
							//server.wsSend(clientIDs[0], clientID1 + "'s score is " + std::to_string(player1Score));
							//server.wsSend(clientIDs[0], clientID2 + "'s score is " + std::to_string(player2Score));
							//server.wsSend(clientIDs[1], clientID1 + "'s score is " + std::to_string(player1Score));
							//server.wsSend(clientIDs[1], clientID2 + "'s score is " + std::to_string(player2Score));

							//server.wsSend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//server.wsSend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
							//delaySend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						
						
							cout << clientID1 << "'s score is " << player1Score << "." << endl;
							cout << clientID2 << "'s score is " << player2Score << "." << endl;
						}
					}
				//server.wsSend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
				//server.wsSend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));			
				
				delaySend(clientIDs[0], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
				delaySend(clientIDs[1], "ball:" + std::to_string(ballX) + ":" + std::to_string(ballY) + ":" + std::to_string(ballxs) + ":" + std::to_string(ballys));
						
				startTime1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				startTime2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

				delaySend(clientIDs[0], "sum1:" + std::to_string(player1Score) + ":" + std::to_string(player2Score) + ":" + clientID2);
				delaySend(clientIDs[1], "sum2:" + std::to_string(player1Score) + ":" + std::to_string(player2Score) + ":" + clientID1);
			}
		}
		next = time(NULL) + 1;
	
		int size = delayM.size();

		for (int i = 0; i < size; i++)
		{
			delayMessage temp2 = delayM.front();
			if (temp2.timeStamp <= time(NULL))
			{
				if (temp2.message.substr(0, temp2.message.find(":")) == "paddle")
				{
					long long t1 = stoll(temp2.t1.c_str());
					auto t2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					long long result = t2 - t1;

					if (temp2.des == 0)
					{
						server.wsSend(1, "T:" + to_string(result) + ":" + to_string(temp2.clientTime));
						startTime2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
						server.wsSend(1, "T1");
					}
					else if (temp2.des == 1)
					{
						server.wsSend(0, "T:" + to_string(result) + ":" + to_string(temp2.clientTime));
						startTime1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
						server.wsSend(0, "T1");
					}
				}

				server.wsSend(temp2.des, temp2.message);
				delayM.pop();
			}
			//else
			//{
			//	newM.push(temp2);
			//	delayM.pop();
			//}
		}
		//delayM = newM;
		//int size2 = delayM.size();
		//for (int i = 0; i < size2; i++)
		//{
		//	newM.pop();
		//}
		
		/*
		WSADATA wsaData;
		DWORD ret = WSAStartup(MAKEWORD(2, 0), &wsaData);

		char *host = "pool.ntp.org"; 
		//char *host = "time.nist.gov";

		NTPMessage msg;
		msg.clear();
		msg.version = 3;
		msg.mode = 3 

		NTPMessage response;
		response.clear();

		int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		sockaddr_in srv_addr;
		memset(&srv_addr, 0, sizeof(srv_addr));
		dns_lookup(host, &srv_addr); 

		msg.sendto(sock, &srv_addr);
		response.recv(sock);

		time_t t = response.tx.to_time_t();
		char *s = ctime(&t);
		printf("The time is %s", s);
		WSACleanup();
		*/
    }
}

int main(int argc, char *argv[]){
    int port;

    cout << "Please set server port: ";
    cin >> port;

    /* set event handler */
    server.setOpenHandler(openHandler);
    server.setCloseHandler(closeHandler);
    server.setMessageHandler(messageHandler);
    server.setPeriodicHandler(periodicHandler);

	/* start the chatroom server, listen to ip '127.0.0.1' and port '8000' */
	server.startServer(port);


    return 1;
}