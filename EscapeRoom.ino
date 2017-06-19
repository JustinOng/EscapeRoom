#include <SPI.h>
#include <MFRC522.h>
 
constexpr uint8_t RST_PIN = 10;     // Configurable, see typical pin layout above
constexpr uint8_t SS_0_PIN = 9;   // Configurable, take a unused pin, only HIGH/LOW required, must be diffrent to SS 2
constexpr uint8_t SS_1_PIN = 8;    // Configurable, take a unused pin, only HIGH/LOW required, must be diffrent to SS 1
constexpr uint8_t SS_2_PIN = 7;
constexpr uint8_t SS_3_PIN = 6;
constexpr uint8_t SS_4_PIN = 5;
constexpr uint8_t SS_5_PIN = 4;
 
String card0 = "04 1D F6 0A D7 49 81";
String card1 = "04 3B E5 0A D7 49 81";
String card2 = "04 09 D2 0A D7 49 81";
String card3 = "04 E2 F1 0A D7 49 80";
String card4 = "04 2C D6 0A D7 49 81";
String card5 = "04 C3 F4 0A D7 49 80";
 
constexpr uint8_t NR_OF_READERS = 6;
 
byte ssPins[] = {SS_0_PIN, SS_1_PIN, SS_2_PIN, SS_3_PIN, SS_4_PIN, SS_5_PIN};
String cards[]= {card0,card1,card2,card3,card4,card5};
 
 
MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.
 
int DELAY_TIME_MILLI = 1000;
 
/**
 * Initialize.
 */
void setup() {
 
  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
 
  SPI.begin();        // Init SPI bus
 
  initialize_readers();
}

void initialize_readers() {
   
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
  }
}
 
/**
 * Main loop.
 */
void loop() {
 
  // iterate through readers. If all readers are in proximity of their paired
  // card then passed will remain true. One false match will set passed to false.
  // Preventing the lock from releasing.
  bool passed = true;
  for (uint8_t readerNum = 0; readerNum < NR_OF_READERS; readerNum++) {
    passed = passed && isMatching(mfrc522[readerNum],cards[readerNum],readerNum);
    Serial.print("reading Reader - ");
    Serial.print(readerNum);
    Serial.print(" - ");
    Serial.println(passed);
  }
  
  
  if (passed == true) {
    Serial.println("Winner Winner Chicken Dinner!");
  }
  
  delay(DELAY_TIME_MILLI);
}
 
bool isMatching(MFRC522 reader, String card, int readerNum) {
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
//    MFRC522::StatusCode result = reader.PICC_RequestA(bufferATQA, &bufferSize);
    // Look for new cards
//    if (MFRC522::STATUS_OK == result && reader.PICC_ReadCardSerial()) {
    if (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial()) {
      Serial.print(F("Reader "));
      Serial.print(readerNum);
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card UID:"));
      String uid = getCardUid(reader);
      Serial.print(uid);

      if(uid == card) {
        Serial.println("match");
        return true;
      } else {
        Serial.print(uid);
        Serial.println(" - not a match");
      }
    }
    return false;
}
 
/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
 
String getCardUid(MFRC522 reader) {
  String content= "";
  byte letter;
  for (byte i = 0; i < reader.uid.size; i++) 
  {
     content.concat(String(reader.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(reader.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content.substring(1);
}
 
 
 
 
 
 

