#include <SoftwareSerial.h>

#define BLINKDELAY 1000
#define BLUEPIN 5
#define YELLOWPIN 4
SoftwareSerial GSMserial(7, 8);

char end_c[2];
int8_t answer = 0, SMSsuccess = 0;
char phone_number[10] = "0123456789";
uint32_t lastblink;
char response[100];
char timec[20];

void setup() {
  pinMode(BLUEPIN, OUTPUT);
  pinMode(YELLOWPIN, OUTPUT);
  digitalWrite(YELLOWPIN, LOW);
  digitalWrite(BLUEPIN, LOW);
  //Serial.begin(9600);
  Serial.println("serial start");
  // ctrlZ String definition
  end_c[0] = 0x1a;
  end_c[1] = '\0';
  GSMstart();
  gettime();
  sendSMS();
  while (SMSsuccess == 0)
  {
    Serial.print("delay start");
    // repeat the SMS sends sequence
    delay(60000);
    sendSMS();
  }
  delay(1000);
  GSMserial.println("AT+CPOWD=1");
  digitalWrite(YELLOWPIN, HIGH);
  GSMserial.end();
}

void loop() {
  /* // uncomment to communicate with GSM module over serial
    if (Serial.available())
    {
      GSMserial.write(Serial.read());
    }
    if (GSMserial.available())
    {
      Serial.write(GSMserial.read());
    }
  */
  if (SMSsuccess == 1 && millis() > lastblink + BLINKDELAY)
  {
    digitalWrite(BLUEPIN, !digitalRead(BLUEPIN));
    digitalWrite(YELLOWPIN, !digitalRead(YELLOWPIN));
    lastblink = millis();
  }
}

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout)
{
  uint8_t x = 0;
  answer = 0;
  //char response[100];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialice the string
  delay(100);
  while (GSMserial.available() > 0) GSMserial.read();   // Clean the input buffer
  //Serial.println(ATcommand);
  GSMserial.println(ATcommand);    // Send the AT command
  //x = 0;
  previous = millis();

  // this loop waits for the answer
  do {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (GSMserial.available() != 0) {
      response[x] = GSMserial.read();
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer) != NULL)
      {
        answer = 1;
      }
    }
    // Waits for the answer with time out and reads the GPS data
  } while ((answer == 0) && ((millis() - previous) < timeout));
  Serial.println(response);
  //Serial.print("answer: "); Serial.println(answer);
  return answer;
}

void GSMstart() // init of GSM module
{
  digitalWrite(YELLOWPIN, HIGH);
  delay(2000);
  GSMserial.begin(9600);
  answer = 0;
  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0)
  {
    // waits for an answer from the module
    while (answer == 0)
    { // send AT every two seconds and wait for the answer
      answer = sendATcommand("AT", "OK", 2000);
    }
  }
  answer = 0;
  delay(2000);
  answer = sendATcommand("AT+CREG?", ",1", 2000);

  // waits for an answer from the module
  while (answer == 0)
  { // checks whether module is connected to network
    answer = sendATcommand("AT+CREG?", "+CREG: 1,1", 2000);  // registrered to home network
  }
  delay(5000);
  while (GSMserial.available() > 0) GSMserial.read();   // Clean the input buffer
  //sendATcommand("ATE0", "OK", 1000);
  digitalWrite(YELLOWPIN, LOW);
}

void gettime(void)
{
  // The module has to receive one time the command: AT+CLTS=1 before connecting to the GSM network
  // then next times it gets the time from the network after connecting via the following command
  sendATcommand("AT+CCLK?", "OK", 2000); //Get the time and store it in global variable 'response'
  
  // the following function retieves the time from the response
  boolean found = false;

  int i = 0;
  while (i < 100 && found == false)
  {
    //Serial.print("response[i]: ");
    //Serial.println(response[i]);
    if (response[i] == '"') // search for " in response, after that the time is printed
    {
      Serial.print("first \" at i: ");
      Serial.println(i);
      found = true;
    }
    i++;
  }
  Serial.print("time: ");
  for (int j = 0; j < 17; j++)
  {
    timec[j] = response[i + j]; // store the time in the timec variable
    Serial.print(timec[j]);
  }
  Serial.println("");
}


int sendSMS(void)
{
  digitalWrite(BLUEPIN, HIGH);
  sendATcommand("AT+CMGF = 1", "OK", 2000); //Because we want to send the SMS in text mode
  Serial.println("AT+CMGS = \"");
  delay(1000);
  sendATcommand("AT", "OK", 2000);
  delay(1000);
  GSMserial.print("AT+CMGS = \"");    // include "+" in number

  for (int i = 0; i < 10; i++)
  {
    // Print phone number:
    GSMserial.print(phone_number[i]);
  }
  GSMserial.println("\"");
  sendATcommand("", ">", 10000);
  GSMserial.print("Your message to be send here. The battery voltag now is ");
  float voltage;
  for (int i = 0; i < 10; i++)
    voltage += map(analogRead(A0), 0, 1023, 0, 500); // take 10 measurements, map to integer 500
  voltage = voltage / (float)1000;      // divide by 100 for converting to float, divide by 10 for averaging measurements
  /*
    Serial.print("Voltage is: ");
    Serial.println(voltage, 2);
  */
  GSMserial.print(voltage, 2);
  GSMserial.print(" V. ");

  GSMserial.print("SMS verstuurd om ");
  for (int j = 0; j < 17; j++)
  {
    GSMserial.print(timec[j]);
  }
  GSMserial.print(".");
  GSMserial.println(end_c);//the ASCII code of the ctrl+z is 26
  GSMserial.println();
  Serial.println("all data sent to GSM\n");
  delay(1000); // without this delay the sequence fails
  SMSsuccess = sendATcommand("", "OK", 10000);
  Serial.print("SMSsuccess: "); Serial.println(SMSsuccess);
  sendATcommand("AT", "OK", 1000);
  digitalWrite(BLUEPIN, LOW);
  return SMSsuccess;
}
