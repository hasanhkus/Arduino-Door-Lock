#include <MFRC522.h>

#include <Servo.h>

#include <DS3231.h>

#include <SD.h>
#include <SPI.h>

#include <LiquidCrystal.h>
LiquidCrystal lcd(5, 2, A0, A1, A2, A3);

//rfid pins
int RST_PIN = 9;
int SSS_PIN = 10;
MFRC522 rfid(SSS_PIN, RST_PIN);

//rfid tags that stored in memory
byte cardIDs[5][4] {
  {9, 9, 9, 9},
  {9, 9, 9, 9},
  {9, 9, 9, 9},
  {9, 9, 9, 9},
  {9, 9, 9, 9}
};

//rfid id read from rfid module
byte cardID[4] = {0};

//names that stored in memory
char allNames[5][10] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

//person Id matched with rfid
int personID = -1;

bool isDoorOpened;
bool isDoorLocked;

//setting real time clock
DS3231  rtc(SDA, SCL);

//files
File myFile;
File cardIDFile;
File namesFile;

//sd card select pin
int CS_SD = 3;

//initializing servo
Servo ServoMotor;

void setup() {
  pinMode(7, INPUT_PULLUP);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading System");
  delay(1500);

  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  rtc.begin();

  ServoMotor.attach(6);

  //locking door when initializing system
  LockedPosition(true);

  initializeSDCard();

  getCardIDs();
  getNames();

  lcd.clear();

  //showCardIDsAndNames(); //uncomment to see device memory
  readSD();              //uncomment to see sdcard logs
}


void loop()
{
  for (int i = 0; i < 10; i++) {
    isDoorOpened = digitalRead(7);
    delay(50);
    if (isDoorOpened == false) break;
  }

  //Serial.println(isDoorOpened);
  if (isDoorOpened && isDoorLocked) {
    // Enables SD card chip select pin
    digitalWrite(CS_SD, LOW);
    lcd.setCursor(0, 1);
    lcd.print("Writing to File");
    delay(500);
    if (SD.begin(CS_SD))
    {
      Serial.println("SD card is ready to write.");
    } else
    {
      Serial.println("SD card initialization failed");
      return;
    }

    if (! SD.exists("Logs")) {
      SD.mkdir("Logs");
    }
    // Create/Open file
    myFile = SD.open("Logs/test.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
      Serial.println("Writing to file...");
      // Write to file
      myFile.print(" Alarm - ");

      myFile.print(rtc.getDateStr());
      myFile.print(" - ");
      myFile.print(rtc.getTimeStr());
      myFile.print(" - ");
      myFile.println(rtc.getDOWStr());

      myFile.close(); // close the file
      Serial.println("Done.");

    }
    // if the file didn't open, print an error:
    else {
      Serial.println("error opening test.txt");
    }
    // Disables SD card chip select pin
    digitalWrite(CS_SD, HIGH);
  }

  //if door is unlocked and door is closed, this will lock the door automatically
  if ( !isDoorOpened && !isDoorLocked) {
    LockedPosition(true);
  }

  //displaying clock on lcd
  displayClock();
  delay(1000);

  // rfid
  if (! rfid.PICC_IsNewCardPresent())return;
  if (! rfid.PICC_ReadCardSerial())return;

  cardID[0] = rfid.uid.uidByte[0];
  cardID[1] = rfid.uid.uidByte[1];
  cardID[2] = rfid.uid.uidByte[2];
  cardID[3] = rfid.uid.uidByte[3];

  if (isValidID()) {
    if (!isDoorLocked)LockedPosition(true);
    else {
      LockedPosition(false);
      displayName();
      logAccessGranted();
    }
  }
  else {
    Serial.println("Unauthorized Tag");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Unauthorized Tag");
    delay(1500);
    LockedPosition(true);
    logAccessDenied();
  }
}

//Locks the door via servo motor
void LockedPosition(int locked)
{
  if (locked)
  {
    ServoMotor.write(10);
    isDoorLocked = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door is Locked");
  }
  else
  {
    ServoMotor.write(90);
    isDoorLocked = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door is unlocked");
  }
}

//reads sd card and prints to serial monitor
void readSD() {
  // Enables SD card chip select pin
  digitalWrite(CS_SD, LOW);
  delay(200);
  // Reading the file
  myFile = SD.open("Logs/test.txt");
  if (myFile) {

    Serial.println("Read:");
    // Reading the whole file
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    Serial.println("");
    myFile.close();
  }
  else {
    Serial.println("error opening test.txt");
  }
  // Enables SD card chip select pin
  digitalWrite(CS_SD, HIGH);
}

//Logs access denied message to sd card
void logAccessDenied() {
  // Enables SD card chip select pin
  digitalWrite(CS_SD, LOW);
  lcd.setCursor(0, 1);
  lcd.print("Writing to File");
  delay(500);
  if (SD.begin(CS_SD))
  {
    Serial.println("SD card is ready to write.");
  } else
  {
    Serial.println("SD card initialization failed");
    return;
  }

  if (! SD.exists("Logs")) {
    SD.mkdir("Logs");
  }
  // Create/Open file
  myFile = SD.open("Logs/test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.println("Writing to file...");
    // Write to file
    myFile.print(" Access Denied - ");

    myFile.print(rtc.getDateStr());
    myFile.print(" - ");
    myFile.print(rtc.getTimeStr());
    myFile.print(" - ");
    myFile.println(rtc.getDOWStr());

    myFile.close(); // close the file
    Serial.println("Done.");

  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
  // Disables SD card chip select pin
  digitalWrite(CS_SD, HIGH);
}

//Logs access granted message to sd card
void logAccessGranted() {
  // Enables SD card chip select pin
  digitalWrite(CS_SD, LOW);
  lcd.setCursor(0, 1);
  lcd.print("Writing to File");
  delay(500);
  if (SD.begin(CS_SD))
  {
    Serial.println("SD card is ready to write.");
  } else
  {
    Serial.println("SD card initialization failed");
    return;
  }


  if (! SD.exists("Logs")) {
    SD.mkdir("Logs");
  }
  // Create/Open file
  myFile = SD.open("Logs/test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.println("Writing to file...");
    // Write to file
    myFile.print(" Access Granted - ");
    myFile.print(allNames[personID]);
    myFile.print(" - ");
    myFile.print(rtc.getDateStr());
    myFile.print(" - ");
    myFile.print(rtc.getTimeStr());
    myFile.print(" - ");
    myFile.println(rtc.getDOWStr());

    myFile.close(); // close the file
    Serial.println("Done.");
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
  // Enables SD card chip select pin
  digitalWrite(CS_SD, HIGH);
}

/*//Prints tag ID uncomment to use
void displayTagID() {
  rfid.PICC_ReadCardSerial();
  Serial.print("ID Numarası: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(rfid.uid.uidByte[i]);
    Serial.print(" ");
  }
  Serial.print("\n");
}
*/
//reads card ID's into memory
void getCardIDs() {
  // Enables SD card chip select pin
  digitalWrite(CS_SD, LOW);
  delay(200);
  if (! SD.exists("settings")) {
    SD.mkdir("settings");
  }
  // Create/Open file
  cardIDFile = SD.open("settings/cardIDs.TXT");
  if (cardIDFile) {
    Serial.println("reading card IDs");
    char readData;
    String kartHane = " ";
    char readByte[3] = {'\0', '\0', '\0'};
    // Reading the whole file
    while (cardIDFile.available()) {

      for (int j = 0; j < 5; j++) {
        for (int i = 0; i < 4; i++) {
          kartHane = " ";
          readByte[0] = '\0'; readByte[1] = '\0'; readByte[2] = '\0';
          for (int k = 0; k < 3; k++) {
            readData = cardIDFile.read();

            if (readData == '\r' && k == 0) {
              readData = cardIDFile.read();
              if (readData == '\n')
                readData = cardIDFile.read();
            }

            if (readData == '\r' && k != 0) {
              readByte[k] = '\0';
              break;
            }
            if (readData == '\n' && k == 0) {
              readData = cardIDFile.read();
            }
            if (readData == '\n' && k != 0) {
              readByte[k] = '\0';
              break;
            }
            if (readData == '-' && k == 0) {
              readData = cardIDFile.read();
            }
            if (readData == '-' && k != 0) {
              readByte[k] = '\0';
              break;
            }
            readByte[k] = readData;
          }
          kartHane = String(readByte);
          cardIDs[j][i] = byte(kartHane.toInt());

          //check if it's end of line and skip eol escape characters
          if (i == 3) {
            readData = cardIDFile.read();
            if (readData == '\r')
              readData = cardIDFile.read();
          }
        }
      }
      cardIDFile.close();
    }
    Serial.println("succesfully read card Ids");
  }
  else {
    Serial.println("error opening cardIDs.txt");
  }
  // Enables SD card chip select pin
  digitalWrite(CS_SD, HIGH);
}
bool isValidID() {
  for (int i = 0; i < 5; i++) {
    if (
      cardID[0] == cardIDs[i][0] &&
      cardID[1] == cardIDs[i][1] &&
      cardID[2] == cardIDs[i][2] &&
      cardID[3] == cardIDs[i][3])
    {
      personID = i;
      return true;
    }
  }
  return false;
}

//reads names from sd card into memory
void getNames() {
  // Enables SD card chip select pin
  digitalWrite(CS_SD, LOW);
  delay(200);

  if (! SD.exists("settings")) {
    SD.mkdir("settings");
  }
  // Create/Open file
  namesFile = SD.open("settings/names.TXT");
  if (namesFile) {
    Serial.println("reading card IDs");

    while (namesFile.available()) {
      char readData;
      //String nameRead = " ";
      char readName[10] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
      // Reading the whole file

      for (int i = 0; i < 5; i++) {
        //nameRead = " ";
        readName[0] = '\0'; readName[1] = '\0'; readName[2] = '\0'; readName[3] = '\0'; readName[4] = '\0'; readName[5] = '\0';
        readName[6] = '\0'; readName[7] = '\0'; readName[8] = '\0'; readName[9] = '\0';
        for (int k = 0; k < 10; k++) {
          readData = namesFile.read();
          if (readData == '\r') {
            readData = namesFile.read();
          }
          if (readData == '\n' || readData == EOF ) {
            //readData = namesFile.read();
            break;
          }
          if (readData == '-' && k == 0) {
            readData = namesFile.read();
          }
          else if (readData == '-') {
            readName[k] = '\0';
            break;
          }
          readName[k] = readData;
        }
        strcpy(allNames[i], readName);
        Serial.println("Name written to memory: ");
        Serial.println(allNames[i]);
      }
      namesFile.close();
    }
    Serial.println("succesfully read card Names");
  }
  else {
    Serial.println("error opening names.txt");
  }
  // Enables SD card chip select pin
  digitalWrite(CS_SD, HIGH);
}

/*
 //shows all names and card ID's in memory to check if they are correct
void showCardIDsAndNames() {
  for (int i = 0; i < 5; i++) {
    Serial.println(allNames[i]);

  }
  for (int i = 0; i < 5; i++) {
    Serial.println(" ");
    Serial.print(cardIDs[i][0]);
    Serial.print("-");
    Serial.print(cardIDs[i][1]);
    Serial.print("-");
    Serial.print(cardIDs[i][2]);
    Serial.print("-");
    Serial.print(cardIDs[i][3]);
  }
}
*/


//displays tag owners name to lcd card
void displayName() {
  //lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Welcome ");
  lcd.print(allNames[personID]);
  delay(1500);
}

//displays clock in the lcd
void displayClock() {
  lcd.setCursor(0, 1);
  lcd.print(rtc.getDateStr());
  lcd.print(' ');
  lcd.print(rtc.getTimeStr(FORMAT_SHORT));
}

void initializeSDCard() {
  // SD Card Initialization
  // Enables SD card chip select pin
  digitalWrite(CS_SD, LOW);
  delay(200);
  if (SD.begin(CS_SD))
  {
    Serial.println("SD card is ready to use.");
  } else
  {
    Serial.println("SD card initialization failed");
    lcd.setCursor(0, 0);
    lcd.print("Sd card problem");
    return;
  }
  // Disables SD card chip select pin
  digitalWrite(CS_SD, HIGH);
}
