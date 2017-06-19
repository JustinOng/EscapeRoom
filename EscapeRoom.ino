#include <SPI.h>
#include <MFRC522.h>

#define NR_OF_READERS   2

// how long to keep output high upon all readers being correct in ms
// set to -1 for infinite
#define OUTPUT_TIME 5000

// refer to https://github.com/esp8266/Arduino/blob/master/variants/d1_mini/pins_arduino.h#L49-L61 for valid pins
// https://s-media-cache-ak0.pinimg.com/originals/8c/a4/cb/8ca4cbbad2532389362bb8b5fc88df17.png for labelled pins
// probably D0, RX, D1, D2, D3, D4?
byte ssPins[] = {RX, D2};

#define OUTPUT_PIN D8
// state to write to OUTPUT_PIN when its supposed to be activated
#define OUTPUT_ACTIVE_STATE HIGH

#define LAST_SEEN_THRESHOLD 500

MFRC522 mfrc522[NR_OF_READERS];   // Create MFRC522 instance.

void setup() {
  delay(100);
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY, 1);
  Serial.println();

  // set frequency lower to attempt to solve some data corruption
  SPI.setFrequency(1000000);
  SPI.begin();

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], 255); // Init each MFRC522 card with 255 as rst pin so is not used
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
  }

  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, !OUTPUT_ACTIVE_STATE);
}

uint8_t correct_uids[6][7] = {
  {0xC7, 0xA3, 0xC5, 0xA5},
  {0x19, 0xDA, 0x69, 0x35},
  //{ 0x04, 0x1D, 0xF6, 0x0A, 0xD7, 0x49, 0x81 },
  //{ 0x04, 0x3B, 0xE5, 0x0A, 0xD7, 0x49, 0x81 },
  { 0x04, 0x09, 0xD2, 0x0A, 0xD7, 0x49, 0x81 },
  { 0x04, 0xE2, 0xF1, 0x0A, 0xD7, 0x49, 0x80 },
  { 0x04, 0x2C, 0xD6, 0x0A, 0xD7, 0x49, 0x81 },
  { 0x04, 0xC3, 0xF4, 0x0A, 0xD7, 0x49, 0x80 },
};

uint8_t reset_uid[7] = { 0x3D, 0xD7, 0xC5, 0xA5 };

void loop() {
  // array tracking last seen millis() of each reader
  static uint32_t last_seen[NR_OF_READERS] = {0};
  // variable tracking millis() when output was activated
  static uint32_t output_set = 0;
  static uint8_t is_output_set = 0;
  
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    uint8_t correct = false;
    
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
      Serial.print(F("Reader "));
      Serial.print(reader);
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card UID:"));
      dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      Serial.print("\r\n");

      correct = true;
      for(uint8_t i = 0; i < mfrc522[reader].uid.size; i++) {
        if (mfrc522[reader].uid.uidByte[i] != correct_uids[reader][i]) {
          correct = false;
          break;
        }
      }

      uint8_t is_reset_uid = true;
      for(uint8_t i = 0; i < mfrc522[reader].uid.size; i++) {
        if (mfrc522[reader].uid.uidByte[i] != reset_uid[i]) {
          is_reset_uid = false;
          break;
        }
      }

      if (is_reset_uid) output_set = 0;

      Serial.print("Correct: ");
      Serial.println(correct ? "YES":"NO");

      if (correct) {
        last_seen[reader] = millis();
      }

      // don't halt or stop crypto so that card will still be readable on next cycle
      // Halt PICC
      //mfrc522[reader].PICC_HaltA();
      // Stop encryption on PCD
      //mfrc522[reader].PCD_StopCrypto1();
    }
  }

  uint8_t all_correct = true;

  for(uint8_t i = 0; i < NR_OF_READERS; i++) {
    // if any reader last_seen is more than LAST_SEEN_THRESHOLD, then not all readers have seen their correct
    // cards recently
    if (millis() - last_seen[i] > LAST_SEEN_THRESHOLD) {
      all_correct = false;
    }
  }

  if (all_correct && !is_output_set) {
    is_output_set = 1;
    Serial.println("Enabling output=================================================");
    output_set = millis();
  }

  
  // output is set to be infinite or OUTPUT_TIME has not passed since correct set was scanned
  if ( is_output_set && 
    ( 
      (output_set > 0 && (uint32_t) OUTPUT_TIME == (uint32_t) -1) ||
      ((uint32_t) OUTPUT_TIME < (uint32_t) -1 && (millis() - output_set) < OUTPUT_TIME)
    )
  ) {
    digitalWrite(OUTPUT_PIN, OUTPUT_ACTIVE_STATE);
  }
  else {
    if (is_output_set) {
      is_output_set = 0;
      Serial.println("Disabling output================================================");
    }
    digitalWrite(OUTPUT_PIN, !OUTPUT_ACTIVE_STATE);
  }
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
