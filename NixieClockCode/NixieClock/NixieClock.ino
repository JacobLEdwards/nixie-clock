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
int currentTime[3] = {};
int lastSync = 0;
int syncInterval = 3600;

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
		// Increment time.
		doTick();

		//Serial.println(inProgress);
		serviceTick = false;

		lastSync += 1;
		//Serial.println(lastSync);

		// Output Time.
		outputTime();
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

// ISR for each second.
void triggerTick()
{
	serviceTick = true;
}

void syncTime()
{
	inProgress = true;
	// Get Time String.
	String timeStamp = getInternetTime();
	// Check Success.
	if (timeStamp == "") {return;}
	// Update sync status if successful.
	lastSync = 0;

	// Update Current Time.
	parseTime(timeStamp, currentTime);

	inProgress = false;
}

// Get Timestamp from internet.
String getInternetTime()
{
	String timeStamp = "";

	HTTPClient http;
	http.begin(apiTarget);
	// Perform Request.
	int status = http.GET();
	//Serial.println(status);

	if (status == 200)
	{
		const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
		DynamicJsonDocument jsonDocument(bufferSize);
		deserializeJson(jsonDocument, http.getString());

		// Get Timestamp.
		timeStamp = jsonDocument["datetime"].as<const char*>();
		Serial.println(timeStamp);

		// Parse HH:MM:SS.
		timeStamp = timeStamp.substring(timeStamp.indexOf('T') + 1, timeStamp.indexOf('T') + 9);
		Serial.println(timeStamp);
	}
	else
	{
		Serial.println("Error.");
	}
	return timeStamp;
}

void doTick()
{
	bool carry = false;

	// Seconds.
	currentTime[2]++;
	if (currentTime[2] > 59)
	{
		carry = true;
		currentTime[2] = 0;
	}

	// Minutes.
	if ((currentTime[2] == 0) && carry)
	{
		carry = false;
		currentTime[1]++;
		if (currentTime[1] > 59)
		{
			carry = true;
			currentTime[1] = 0;
		}
	}

	// Hours.
	if ((currentTime[1] == 0) && carry)
	{
		currentTime[0]++;
		if (currentTime[0] > 23)
		{
			currentTime[0] = 0;
		}
	}
}

void parseTime(String timestamp, int* result)
{
	for (size_t i = 0; i < 9; i += 3)
	{
		// Check Char
		String part = timestamp.substring(i, i + 2);

		// Get Value.
		result[i/3] = part.toInt();
	}
}

void outputTime()
{
	bool dataBits[24] = {};
	int value = 0;
	int addr = 0;

	// Convert Current Time
	for (size_t i = 0; i < 3; i++)
	{
		// Get Section.
		String s = (String)currentTime[i];
		// Pad.
		if (currentTime[i] < 10)
		{
			s = "0" + s;
		}

		// Split into Digits
		for (size_t j = 0; j < 2; j++)
		{
			// Get Digit
			value = s.substring(j, j+1).toInt();
			
			// Convert to bits.
			dataBits[(addr * 4)] = (value >> 3) & 0x1;
			dataBits[(addr * 4) + 1] = (value >> 2) & 0x1;
			dataBits[(addr * 4) + 2] = (value >> 1) & 0x1;
			dataBits[(addr * 4) + 3] = value & 0x1;

			addr++;
		}
	}

	// Debug Output.
	for (size_t i = 0; i < 24; i++)
	{
		Serial.print(dataBits[i]);
	}

	Serial.println("");

	// Output.
	writeData(dataBits);
}

void writeData(bool* writeData)
{
	for (int i = 23; i >= 0; i--)
	{
		if (writeData[i])
		{
			digitalWrite(dataPin, HIGH);
		}
		else
		{
			digitalWrite(dataPin, LOW);
		}

		// Clock and Latch.
		digitalWrite(clockPin, HIGH);
		delay(10);
		digitalWrite(clockPin, LOW);
	}

	// Latch
	digitalWrite(latchPin, HIGH);
	delay(10);
	digitalWrite(latchPin, LOW);
}