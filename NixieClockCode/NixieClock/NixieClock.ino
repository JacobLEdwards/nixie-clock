#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

// Wifi.
const char* ssid = "";
const char* password = "";

// Time Output Pins
const int dataPin  = 12;
const int latchPin = 13;
const int clockPin = 14;

// Time Variables
bool timeData[24] = {};
String timeStamp = "";
int lastSync = 0;
int syncInterval = 10;

// Time API
const char* apiTarget = "http://worldtimeapi.org/api/timezone/Europe/London";
bool inProgress = false;

// Timer.
Ticker ticker;
bool serviceTick = false;

void setup() {

	// Serial connection
	Serial.begin(115200);
	// WiFi connection
	WiFi.begin(ssid, password);

	Serial.println("");
	Serial.println("Connecting.");
	// Wait for the WiFI connection completion
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("Connected.");

	// Initialise Pins
	pinMode(dataPin, OUTPUT);
	pinMode(latchPin, OUTPUT);
	pinMode(clockPin, OUTPUT);

	// Get Time for first loop.
	if (WiFi.status() == WL_CONNECTED) {
		syncTime();
	}

	// Timer
	ticker.attach(1, triggerTick);
}

void loop() {
	// Check current time progress.
	if (serviceTick)
	{
		// Increment time and display.
		doTick();

		Serial.println(inProgress);
		serviceTick = false;

		lastSync += 1;
		Serial.println(lastSync);
	}

	// Check Sync Interval.
	if ((lastSync >= syncInterval) && !(inProgress))
	{
		// Get Current Time.
		if (WiFi.status() == WL_CONNECTED) {
			syncTime();
		}
		else
		{
			Serial.println("Network Error.");
		}
	}
}

void triggerTick()
{
	serviceTick = true;
}

void syncTime()
{
	inProgress = true;
	// Get Time String.
	timeStamp = getInternetTime();
	// Check Success.
	if (timeStamp == "") {return;}
	// Update sync status if successful.
	lastSync = 0;
	// Parse To 3-Byte array.

	inProgress = false;
}

String getInternetTime()
{
	HTTPClient http;
	http.begin(apiTarget);
	// Perform Request.
	int status = http.GET();
	Serial.println(status);

	if (status > 0)
	{
		const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
		DynamicJsonDocument jsonDocument(bufferSize);
		deserializeJson(jsonDocument, http.getString());

		// Get Timestamp.
		timeStamp = jsonDocument["utc_datetime"].as<const char*>();
		Serial.println(timeStamp);
	}
	else
	{
		timeStamp = "";
		Serial.println("Error.");
	}
	return timeStamp;
}

void doTick()
{

}

void convertToBits()
{

}

void writeTime(bool* timeData) 
{
	for (int i = 0; i < 24; i++)
	{
		if (timeData[i])
		{
			digitalWrite(dataPin, HIGH);
		}
		else
		{
			digitalWrite(dataPin, LOW);
		}

		// Clock and Latch.
		digitalWrite(clockPin, HIGH);
		delay(3);
		digitalWrite(clockPin, LOW);

		digitalWrite(latchPin, HIGH);
		delay(3);
		digitalWrite(latchPin, LOW);

		// Wait
		delay(5);
	}
}