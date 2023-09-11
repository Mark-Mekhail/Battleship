/*
 * WebSocketClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <WebSocketsClient.h>
#include <Hash.h>

#define RES_COMPLETE 0x24
#define NULL_CHAR 0x00
#define TRUE 0x31
#define FALSE 0x30

// response opcodes
#define ROOM_CREATED 0x31     // 1
#define START_SETUP 0x32      // 2
#define START_GAME 0x33       // 3
#define FIRED 0x34            // 4
#define OPPONENT_FIRED 0x35   // 5
#define OPPONENT_LEFT 0x36    // 6
#define GAME_RESET 0x37       // 7
#define CHECK_WSCON 0x38      // 8
#define ERROR 0x39            // 9
#define SHIPS_SET 0x40        // @

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

//const char* ssid     = "CPEN391";
//const char* password = "57055954";

const char* ssid     = "Ritam";
const char* password = "ritam1234";

//#define Serial Serial1

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  char onConnect[] = "connected$";
  char onDisconnect[] = "disconnected$";
	switch(type) {
		case WStype_DISCONNECTED:
			//Serial.printf("[WSc] Disconnected!: %s\n");
      serialPrintDelay(onDisconnect);
      
			break;
		case WStype_CONNECTED: 
			//Serial.printf("[WSc] Connected to url: %s\n", payload);
      serialPrintDelay(onConnect);
			break;
		case WStype_TEXT:
			//Serial.printf("[WSc] get text: %s\n", payload);
      handleResponse(payload);
			break;
		case WStype_BIN:
			//Serial.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);
			break;
    case WStype_PING:
      // pong will be send automatically
      //Serial.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      //Serial.printf("[WSc] get pong\n");
      break;
    }

}

void handleResponse(uint8_t * payload) {
  static StaticJsonDocument<256> json_doc;
  const auto deser_err = deserializeJson(json_doc, payload);
  if (!deser_err) {
    if (json_doc["type"] == "ROOM_CREATED") {
      char res[7];
      res[0] = ROOM_CREATED;
      itoa(json_doc["roomId"],(res + 1),10);
      res[5] = RES_COMPLETE;
      res[6] = NULL_CHAR;
      serialPrintDelay(res);
    } else if(json_doc["type"] == "START_SETUP") {
      char res[3];
      res[0] = START_SETUP;
      res[1] = RES_COMPLETE;
      res[2] = NULL_CHAR;
      serialPrintDelay(res);
    } else if(json_doc["type"] == "START_GAME") {
      char res[4];
      res[0] = START_GAME;
      res[1] = json_doc["yourTurn"] ? TRUE : FALSE;
      res[2] = RES_COMPLETE;
      res[3] = NULL_CHAR;
      serialPrintDelay(res);
    } else if(json_doc["type"] == "FIRED" || json_doc["type"] == "OPPONENT_FIRED") {
      char res[10];
      res[0] = json_doc["type"] == "FIRED" ? FIRED : OPPONENT_FIRED;
      res[1] = json_doc["hit"] ? TRUE : FALSE;
      itoa(json_doc["x"], (res + 2), 10);
      itoa(json_doc["y"], (res + 3), 10);
      res[4] = json_doc["sunk"] ? TRUE : FALSE;
      if (json_doc["sunk"]) {
        itoa(json_doc["start"]["x"], (res + 5), 10);
        itoa(json_doc["start"]["y"], (res + 6), 10);
        itoa(json_doc["end"]["x"], (res + 7), 10);
        itoa(json_doc["end"]["y"], (res + 8), 10);
        res[9] = RES_COMPLETE;
        res[10] = NULL_CHAR;
      } else {
        res[5] = RES_COMPLETE;
        res[6] = NULL_CHAR;
      }
      serialPrintDelay(res);
    } else if(json_doc["type"] == "OPPONENT_LEFT") {
        char res[3];
        res[0] = OPPONENT_LEFT;
        res[1] = RES_COMPLETE;
        res[2] = NULL_CHAR;
        serialPrintDelay(res);
    } else if(json_doc["type"] == "GAME_RESET") {
        char res[3];
        res[0] = GAME_RESET;
        res[1] = RES_COMPLETE;
        res[2] = NULL_CHAR;
        serialPrintDelay(res);
    } else if(json_doc["type"] == "ERROR") {
        char res[150];
        const char* message = json_doc["message"];
        sprintf(res, "9: %s$", message);
        serialPrintDelay(res);
    } else if(json_doc["type"] == "SHIPS_SET") {
        char res[3];
        res[0] = SHIPS_SET;
        res[1] = RES_COMPLETE;
        res[2] = NULL_CHAR;
        serialPrintDelay(res);
    }
  }
}

void handleRequest( StaticJsonDocument<364> json_doc) {
  if (json_doc["type"] == "SET_SHIPS") {
    setShips(json_doc);
  } else if(json_doc["type"] == "CHECK_WSCON") {
    char res[4];
    res[0] = CHECK_WSCON;
    res[1] = WiFi.status() == WL_CONNECTED ? TRUE : FALSE;
    res[2] = RES_COMPLETE;
    res[3] = NULL_CHAR;
    serialPrintDelay(res);
  } else {
    char buffer[450];
    serializeJson(json_doc, buffer);
    //Serial.println(buffer);
    webSocket.sendTXT(buffer);
    //Serial.println();
  }  
}

void serialPrintDelay(char *buffer) {
  for (int i = 0; i < strlen(buffer); i++) {
    Serial.print(buffer[i]);
    delayMicroseconds(70);
  } 
}

void setShips(StaticJsonDocument<364> receivedDoc) {
  StaticJsonDocument<600> doc;
  int battleshipPos[4];
  int cruiserPos[4];
  int submarinePos[4];
  int destroyerPos[4];
  copyArray(receivedDoc["b"], battleshipPos);
  copyArray(receivedDoc["c"], cruiserPos);
  copyArray(receivedDoc["s"], submarinePos);
  copyArray(receivedDoc["d"], destroyerPos);

  StaticJsonDocument<525> array;
  JsonArray shipPositions = array.to<JsonArray>();
  
  DynamicJsonDocument battleship(125);
  JsonObject root = battleship.to<JsonObject>();
  JsonObject start = root.createNestedObject("start");
  JsonObject end = root.createNestedObject("end");
  root["type"] = "battleship"; 
  start["x"] = battleshipPos[0];
  start["y"] = battleshipPos[1];  
  end["x"] = battleshipPos[2];
  end["y"] = battleshipPos[3];
  shipPositions.add(battleship);

  DynamicJsonDocument cruiser(125);
  root = cruiser.to<JsonObject>();
  start = root.createNestedObject("start");
  end = root.createNestedObject("end");
  root["type"] = "cruiser"; 
  start["x"] = cruiserPos[0];
  start["y"] = cruiserPos[1];  
  end["x"] = cruiserPos[2];
  end["y"] = cruiserPos[3];
  shipPositions.add(cruiser);

  DynamicJsonDocument submarine(125);
  root = submarine.to<JsonObject>();
  start = root.createNestedObject("start");
  end = root.createNestedObject("end");
  root["type"] = "submarine"; 
  start["x"] = submarinePos[0];
  start["y"] = submarinePos[1];  
  end["x"] = submarinePos[2];
  end["y"] = submarinePos[3];
  shipPositions.add(submarine);

  DynamicJsonDocument destroyer(125);
  root = destroyer.to<JsonObject>();
  start = root.createNestedObject("start");
  end = root.createNestedObject("end");
  root["type"] = "destroyer"; 
  start["x"] = destroyerPos[0];
  start["y"] = destroyerPos[1];  
  end["x"] = destroyerPos[2];
  end["y"] = destroyerPos[3];
  shipPositions.add(destroyer);

  doc["type"] = "SET_SHIPS";
	doc["shipPositions"] = shipPositions;
	
  
  char data[400];
	serializeJson(doc, data);
	webSocket.sendTXT(data);
}

void setup() {
	// Serial.begin(921600);
	Serial.begin(115200);

	//Serial.setDebugOutput(true);
	Serial.setDebugOutput(false);

	Serial.println();
	Serial.println();
	Serial.println();

	for(uint8_t t = 4; t > 0; t--) {
		Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
		Serial.flush();
		delay(1000);
	}

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //Serial.println("");
  //Serial.println("[SETUP] WiFi connected");  
  //Serial.println("[SETUP] IP address: ");
  //Serial.println(WiFi.localIP());
  //Serial.print("[SETUP] Netmask: ");
  //Serial.println(WiFi.subnetMask());
  //Serial.print("[SETUP] Gateway: ");
  //Serial.println(WiFi.gatewayIP());

	// server address, port and URL
	webSocket.begin("ec2-3-135-189-6.us-east-2.compute.amazonaws.com", 8080, "/");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
  
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);

}

int charCount = 0;
  int writeCount = 0;

void loop() {
	webSocket.loop();

  static StaticJsonDocument<364> json_doc;
  
  if(Serial.available()) {
    const auto deser_err = deserializeJson(json_doc, Serial);
      if (!deser_err) {
        //Serial.print(F("Recevied valid json document with "));
        //Serial.print(json_doc.size());
        //Serial.println(F(" elements."));
        //Serial.println(F("Pretty printed back at you:"));
        handleRequest(json_doc);
    }
  }
}
