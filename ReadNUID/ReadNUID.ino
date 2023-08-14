#include <SPI.h>
#include <MFRC522.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>
#include <aREST.h>

#define SS_PIN D8
#define RST_PIN D0

aREST rest = aREST();

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

const char* ap_ssid = "IOT_MODULE1";  // Enter SSID here
const char* ap_password = "12345678";  //Enter Password here

IPAddress local_ip(192,168,171,100);
IPAddress gateway(192,168,171,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(8,8,4,4);

WiFiServer server(81);

void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);
String load_from_file(String file_name);
bool write_to_file(String file_name, String contents);

void setup_wifi();

/* REST Functions */
int do_action(String Command);
int reset_module();
int restart_module();

String get_ssid();
String get_password();
String get_server();
String get_local_ip();
String get_gateway();
String get_subnet();
String get_primaryDNS();
String get_secondaryDNS();

int set_ssid(String Command);
int set_password(String Command);
int set_server(String Command);
int set_local_ip(String Command);
int set_gateway(String Command);
int set_subnet(String Command);
int set_primaryDNS(String Command);
int set_secondaryDNS(String Command);

void setup() { 
  Serial.begin(115200);

  rest.function("do_action", do_action);
  rest.function("reset_module", reset_module);
  rest.function("restart_module", restart_module);
  rest.function("set_ssid", set_ssid);
  rest.function("set_password", set_password);
  rest.function("set_server", set_server);
  rest.function("set_local_ip", set_local_ip);
  rest.function("set_gateway", set_gateway);
  rest.function("set_subnet", set_subnet);
  rest.function("set_primaryDNS", set_primaryDNS);
  rest.function("set_secondaryDNS", set_secondaryDNS);

  Serial.println("");
  Serial.println("");
  Serial.println("===========================================");
  Serial.println("Booting....");
  delay(1000);

  LittleFS.begin();

  setup_wifi();

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}
 
void loop() {

  WiFiClient client = server.available();

  
  if(!client.available()){
    delay(1);
  } else {
    rest.handle(client);
  }


  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  // if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
  //   piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
  //   piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
  //   Serial.println(F("Your tag is not of type MIFARE Classic."));
  //   return;
  // }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

}

int do_action(String Command)
{
  Serial.println("Reseting module....");
  set_ssid("");
  set_password("");
  LittleFS.format();
  ESP.restart();
  return 0;
}

int reset_module(String Command)
{
  Serial.println("Reseting module....");
  set_ssid("");
  set_password("");
  LittleFS.format();
  ESP.restart();
  return 0;
}

int restart_module(String Command)
{
  Serial.println("Restating module....");
  ESP.restart();
  return 0;
}

String get_ssid() { return load_from_file("ssid.cfg"); };
String get_password() { return load_from_file("password.cfg"); };
String get_server() { return load_from_file("server.cfg"); };
String get_local_ip() { return load_from_file("local_ip.cfg"); };
String get_gateway() { return load_from_file("gateway.cfg"); };
String get_subnet() { return load_from_file("subnet.cfg"); };
String get_primaryDNS() { return load_from_file("primaryDNS.cfg"); };
String get_secondaryDNS() { return load_from_file("secondaryDNS.cfg"); };

int set_ssid(String Command) { return write_to_file("ssid.cfg", Command); }
int set_password(String Command) { return write_to_file("password.cfg", Command); }
int set_server(String Command) { return write_to_file("server.cfg", Command); }
int set_local_ip(String Command) { return write_to_file("local_ip.cfg", Command); }
int set_gateway(String Command) { return write_to_file("gateway.cfg", Command); }
int set_subnet(String Command) { return write_to_file("subnet.cfg", Command); }
int set_primaryDNS(String Command) { return write_to_file("primaryDNS.cfg", Command); }
int set_secondaryDNS(String Command) { return write_to_file("secondaryDNS.cfg", Command); }

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

String load_from_file(String file_name) {
  String result = "";
  
  File this_file = LittleFS.open(file_name, "r");
  if (!this_file) { // failed to open the file, retrn empty result
    return result;
  }
  while (this_file.available()) {
      result += (char)this_file.read();
  }
  
  this_file.close();
  return result;
}

bool write_to_file(String file_name, String contents) {  
  File this_file = LittleFS.open(file_name, "w");
  if (!this_file) { // failed to open the file, return false
    return false;
  }
  int bytesWritten = this_file.print(contents);
 
  if (bytesWritten == 0) { // write failed
      return false;
  }
   
  this_file.close();
  return true;
}

void setup_wifi()
{
  String saved_ssid = get_ssid();
  String saved_password = get_password();
  String saved_local_ip = load_from_file("localip.cfg");
//  String saved_server = load_from_file("server.cfg");

  if (saved_ssid == "") {
    Serial.print("Start using default configuration AP id : ");
    Serial.print(ap_ssid);
    Serial.print(" and password : ");
    Serial.println(ap_password);
    WiFi.softAP(ap_ssid, ap_password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
  } else {
    Serial.print("Start using saved configuration AP id : ");
    Serial.print(saved_ssid);
    Serial.print(" and password : ");
    Serial.println(saved_password);
    if(saved_local_ip != "") {
      local_ip.fromString(get_local_ip());
      gateway.fromString(get_gateway());
      subnet.fromString(get_subnet());
      primaryDNS.fromString(get_primaryDNS());
      secondaryDNS.fromString(get_secondaryDNS());
      if (!WiFi.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
      }
    }
    WiFi.begin(saved_ssid, saved_password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("Connected to WiFi");
    Serial.println("Starting Server.....");
    server.begin();
    Serial.println("Server started.");
  }
}