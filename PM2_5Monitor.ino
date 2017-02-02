#include <AltSoftSerial.h>
//#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
//#include <TimeLib.h>
//#include <uartWIFI.h>



//软串口，Uno开发板：Tx-D9、Rx-D8。Rx接传感器的Tx。
AltSoftSerial altSerial; 
//SoftwareSerial altSerial;

//YWrobot I2C 1602液晶屏
LiquidCrystal_I2C lcd(0x20, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//攀藤G5的数据格式
struct _panteng {
        unsigned char len[2];
        unsigned char pm1_cf1[2];
        unsigned char pm2_5_cf1[2];
        unsigned char pm10_0_cf1[2];
        unsigned char pm1_0[2];
        unsigned char pm2_5[2];
        unsigned char pm10_0[2];
        unsigned char pm0_3_count[2];
        unsigned char pm0_5_count[2];
        unsigned char pm1_0_count[2];
        unsigned char pm2_5_count[2];
        unsigned char pm5_count[2];
        unsigned char pm10_count[2];
        unsigned char hcho[2];
        unsigned char checksum[2];
} panteng;


// for yeelink api
#define APIKEY         "72f18469a82f9396526051ab7ab3e08c" // replace your yeelink api key here

//replace the device ID and sensor ID for temperature sensor.
#define DEVICEID0       354276 // replace your device ID
#define SENSORID0       400070 // replace your sensor ID

//replace the device ID and sensor ID for humidity sensor.
#define DEVICEID1       354276 // replace your device ID
#define SENSORID1       400071 // replace your sensor ID

#define RESET_PIN 13

const char server[] = "api.yeelink.net";   // name address for yeelink API

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
//boolean lastConnected = false;                 // state of the connection last time through the main loop
//const unsigned long postingInterval = 5*1000; // delay between 2 datapoints, 5s
//String returnValue = ""; 
//boolean ResponseBegin = false;

//String bar = "###########################################################################################################";

void setup()
{

        pinMode(RESET_PIN, OUTPUT);
        digitalWrite(RESET_PIN, HIGH); //seems the digitial pins are default to LOW, need to initialize to HIGH to avoid constant reset.
        
        lcd.init();                      // initialize the lcd 
        //lcd.backlight();
        lcd.noBacklight();

        Serial.begin(115200);        //USB串口向PC发送数据
        altSerial.begin(9600);        //软串口连接传感器

        lcd.setCursor(0, 0);
        lcd.print("ready...");

        randomSeed(analogRead(0));
}

  int frame_count=0;     // number of data frames received from G5S Sensor
  int upload_count=0;    // number of uploads to the server
  int upload_success_count=0; // number of successful uploads
  float pm2_5_avg=0;
  float hcho_avg=0;
  int data_count=0;
  int pm1_0, pm2_5, pm10_0, hcho, frame_length, frame_checksum;        //PM1.0、PM2.5、PM10
  int checksum, error_count=0;

        
void loop()
{
        unsigned char c;


        static int state = 0;
        static int count = 0;
        static int time=0;

       // int i;
//Serial.print(".");
        if (altSerial.available()) {
                //Serial.println(".");
                c = altSerial.read();
                switch (state) {
                case 0:
                        if (0x42 == c)
                                state = 1;
                                count=0;
                                //Serial.println("state1");
                        break;
                case 1:
                        if (0x4d == c) {
                                state = 2;
                                count = 0;
                                //Serial.println("state2");
                        }
                        break;
                case 2:
                        ((unsigned char *) &panteng)[count++] = c;
                        //sprintf(str, "%02X ", c);
                        //Serial.print(str);
                        if (count > 30) {
                                state = 0;
                                pm1_0 = panteng.pm1_0[0] * 256 + panteng.pm1_0[1];
                                pm2_5 = panteng.pm2_5[0] * 256 + panteng.pm2_5[1];
                                pm10_0 = panteng.pm10_0[0] * 256 + panteng.pm10_0[1];
                                hcho = panteng.hcho[0] *256 + panteng.hcho[1];
                                frame_length = panteng.len[0]*256+panteng.len[1];
                                frame_checksum = panteng.checksum[0]*256+panteng.checksum[1];

                                
                                



                                //sprintf(str, "%d\t%d\t%d\t%d", time++,pm1_0, pm2_5, pm10_0);
                                //Serial.println(str);
                                char pm2_5_str[20];
                                snprintf(pm2_5_str,16, "PM2.5/10=%d/%d   ", pm2_5,pm10_0);
                                char pm1_0_str[20];
                                snprintf(pm1_0_str,16, "PM1.0/10=%d/%d   ", pm1_0, pm10_0);
                                char hcho_f[10];
                                char hcho_str[20];
                                sprintf(hcho_str, "HCHO:%s  ", dtostrf(hcho*0.001, 1,3, hcho_f));
                                



                                frame_count++;
                                
                                if(data_count==0) {
                                  pm2_5_avg=pm2_5;
                                }
                                else {
                                  pm2_5_avg=pm2_5_avg*(data_count*1.0/(data_count+1)) + pm2_5*1.0/(data_count+1);
                      
                                }
                                //keep count max at 100, a spike will be flatten out sooner
                                if(data_count<100) {
                                  data_count++;
                                }
                                
                                checksum = calculateChecksum();
                                
                                debug();

//                                lcd.clear();
//                                if (error_count==-1) {
//                                  lcd.print(pm2_5_str);
//                                  lcd.setCursor(0, 1);
//                                  lcd.print(hcho_str);          
//                                }
//                                else {
//                                  lcd.print("Frame: "+String(frame_count));
//                                  lcd.setCursor(0, 1);
//                                  lcd.print("Error: "+String(error_count));
//                                }

                                if (checksum == frame_checksum ) {

                                  
                                  sendData(DEVICEID0, SENSORID0,pm2_5);
                                  
                                  delay(5000);
                                  
                                  sendData(DEVICEID0, SENSORID1,hcho*0.001);
  
                                  delay(5000);                                 
                                }
                                                            


                                
                                /*
                                for (i = 0; i < strlen(str); i++) {
                                        lcd.setCursor(i, 0);
                                        lcd.print(str);
                                }
                                */
                        }
                        break;
                default:
                        break;
                }
        }
        else {
               //lcd.setCursor(0, 1);
               //lcd.print("waiting...");
        }
}

int calculateChecksum() {
  int checksum=143; //0x42+0x4d
  for(int i=0; i<28; i++) {
    checksum += ((unsigned char *) &panteng)[i];
  }
  return checksum;
}

void debug () {

    //Serial.print(pm2_5_avg);
    //Serial.print(" / ");
    //Serial.println(data_count);

    if (frame_count%100==0) {
      //Serial.println(bar);
      //Serial.println("\n>>>>>>>>>>>>>> Frame Count: "+String(frame_count) + "Error Count: "+String(error_count) + "<<<<<<<<<<<<<<<<");
      //Serial.println(bar);
    }
    else {
      //Serial.print(".");
    }

    if (checksum != frame_checksum ) {
      error_count++;
//      Serial.println(bar);
      Serial.println("\n>>>>>> OH NO !!!  "+String(error_count) +"<<<<<<<<<<<");

      Serial.print("PM2.5: ");
      Serial.println(pm2_5);
      Serial.print("HCHO: ");
      Serial.println(hcho*0.001);
      
      Serial.println("Frame Length: "+String(frame_length));
      Serial.println("Frame Checksum: "+String(frame_checksum)+" Calculated Checksum: "+String(checksum));
  
//      Serial.println(bar);
    }
//    else if (pm2_5>1000 || hcho>2000 ) {
//      error_count++;
//      Serial.println(bar);
//      Serial.println("\n#### HOLY SHIT!!! "+String(error_count) +"####");
//      
//      Serial.print("PM2.5: ");
//      Serial.println(pm2_5);
//      Serial.print("HCHO: ");
//      Serial.println(hcho*0.001);
//      
//      Serial.print("Frame Length: ");
//      Serial.println(frame_length);
//      
//      Serial.print("Checksum: ");
//      Serial.print(frame_checksum);
// 
//
//      Serial.print(" / ");
//      Serial.println(checksum);   
//      Serial.println(bar);
//      }
}

// this method makes a HTTP connection to the server:
void sendData(long device_id,long sensor_id,float thisData) {


  //reset wifi board every 100 connections made
  if(upload_count !=0 && upload_count % 100 ==0) {
    resetWifi();
  }

  upload_count++;
        
  //Serial.end();
  Serial.begin(115200);
  
  while (!Serial) {
  ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  String json = "{";
//        json +="\"timestamp\":\"";
//        json += String(year());
//        json +="-";
//        json +=String(month());
//        json +="-";
//        json +=String(day());
//        json +="T";
//        json +=String(hour());
//        json +=":";
//        json +=String(minute());
//        json +=":";
//        json +=String(second());
//        json +="\",\r\n";
        json += "\"value\":"+ String(thisData);
        json += "}";
    
    String cmd;
        cmd = "POST /v1.0/device/";
        cmd += String(device_id);
        cmd += "/sensor/";
        cmd += String(sensor_id);
        cmd += "/datapoints";
        cmd += " HTTP/1.1\r\n";
        cmd += "Host: api.yeelink.net\r\n";
//        cmd += "Accept: */*\r\n";
        cmd += "U-ApiKey: ";
        cmd += APIKEY;
        cmd += "\r\n";
        cmd += "Connection: close\r\n";
//        cmd += "Content-Type: application/x-www-form-urlencoded\r\n";
//        cmd += "\r\n";    
        cmd += "Content-Length: "+String(json.length());  
        cmd += "\r\n\r\n";
        cmd += json;
        cmd += "\r\n";
        cmd += "\r\n";

        Serial.print(cmd);
        Serial.flush();


        delay(2000);
        readResponse();
        
        Serial.end();
        

        

//        delay(3000);
        
    //wifi.Send(cmd);
    // note the time that the connection was made:
    lastConnectionTime = millis();
}

void resetWifi() {
      lcd.setCursor(0, 1);
      lcd.print("Resetting Wifi...");
      digitalWrite(RESET_PIN, LOW);   // Reset ESP8266
      delay(50);                       // wait for a short period
      digitalWrite(RESET_PIN, HIGH);
      delay(10000);                    // wait for wifi reconnect
      lcd.setCursor(0, 1);
      lcd.print("Wifi reconnected...");
}

void readResponse() {
          String s;
        while (Serial.available()) {
          char inChar = (char)Serial.read(); 
          // add it to the inputString unless end of line is reached.
          
          if (inChar == '\n' || inChar == '\r') {
            break;
          } 
          
          s += inChar;

        }
        
        //lcd.clear();
        
        if (s=="HTTP/1.1 200 OK") {

          upload_success_count++;
          lcd.setCursor(0, 0);
          lcd.print("Upload: "+String(upload_success_count)+"/"+String(upload_count));

        } else {
          lcd.setCursor(0, 0);
          lcd.print("Upload: "+String(upload_success_count)+"/"+String(upload_count));
          lcd.setCursor(0, 1);
          lcd.print(s);
          
          delay(5000);
        }



        Serial.flush();
        while (Serial.available()) {
          Serial.read(); 
        }
        
}
