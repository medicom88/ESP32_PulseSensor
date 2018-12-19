#include <WiFi.h>

#define HeartBeatLED  15
#define HeartBeatADC  36
#define ALPHA   0.6
#define SYSTEM_LOOP_10MS  10
#define BPM_60S_SCALE_VALUE   60000
#define AD_ROW_MIN  0
#define AD_ROW_MAX  4096
#define BPM_THRESHOLD 200

#define _LED_PWM_GPIO   26
#define _LED_GND_GPIO   25
#define DUTY_CYCLE    178       //256 * 0.7 -1

int freq = 1000;
int ledChannel = 0;
int resolution = 8;

WiFiClient client;

char ssid[30]     = "KeplerAP";
char password[30] = "hybus12345";
char hostIP[30]   = "192.168.4.1";
int hostPort = 8080;


int heartBeatAD_Data = 0;
volatile int Max_heartBeatAD_Data = AD_ROW_MIN;
volatile int Min_heartBeatAD_Data = AD_ROW_MAX;
volatile int Threshold_heartBeatAD_Data = 0;
volatile int HeartBeatBPM = 0;
volatile int His_HeartBeatBPM = 0;
volatile int New_HeartBeatBPM = 0;


char heartBeatCntStartFlag = 0;


hw_timer_t * timer = NULL;
volatile int system_time_1ms = 0;
volatile int heartBeatCnt_1ms = 0;
volatile int TimeDataCnt_10ms = 0;
void IRAM_ATTR onTimer(){
  system_time_1ms++;
  
  if(heartBeatCntStartFlag == 1){
    heartBeatCnt_1ms++;
  }

  if(Max_heartBeatAD_Data > AD_ROW_MIN){
    Max_heartBeatAD_Data--;
  }

  if(Min_heartBeatAD_Data < AD_ROW_MAX){
    Min_heartBeatAD_Data++;
  }
  
}



void PulseSensorFunc()
{
  static char onlyonce = 0;
  heartBeatAD_Data = analogRead(HeartBeatADC);

  //예외처리
  if(heartBeatAD_Data == 0 || Min_heartBeatAD_Data == 0 || Max_heartBeatAD_Data == 4096 || (Min_heartBeatAD_Data == Max_heartBeatAD_Data)/* || Reset_MinMax_1ms >= 5000*/){
    Max_heartBeatAD_Data = AD_ROW_MIN;
    Min_heartBeatAD_Data = AD_ROW_MAX;
    HeartBeatBPM = 0;
  }

  if(Max_heartBeatAD_Data < heartBeatAD_Data){
    Max_heartBeatAD_Data = heartBeatAD_Data;
  }

  if(Min_heartBeatAD_Data > heartBeatAD_Data){
    Min_heartBeatAD_Data = heartBeatAD_Data;
  }

  Threshold_heartBeatAD_Data = ((Max_heartBeatAD_Data - Min_heartBeatAD_Data) * 0.7) + Min_heartBeatAD_Data;

  if(heartBeatAD_Data >= Threshold_heartBeatAD_Data){
    if(onlyonce == 0){
      heartBeatCntStartFlag = 1;

      if(heartBeatCnt_1ms > 0){
        HeartBeatBPM = BPM_60S_SCALE_VALUE / heartBeatCnt_1ms;
        if(HeartBeatBPM > BPM_THRESHOLD){
          HeartBeatBPM = 0;
        }

        New_HeartBeatBPM = (His_HeartBeatBPM  * ALPHA) + ((1 - ALPHA) * HeartBeatBPM);
        His_HeartBeatBPM = New_HeartBeatBPM;
      }
      heartBeatCnt_1ms = 0;

      ledcWrite(ledChannel, DUTY_CYCLE);
      
      onlyonce = 1;
    }
    
  }
  else if(heartBeatAD_Data < Threshold_heartBeatAD_Data){
    onlyonce = 0;
    ledcWrite(ledChannel, 0);
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  timer = timerBegin(1, 80, true);      // 1 / (80MHz / N) => N:80 => 1 / timerFreq = 1 / 1MHz = 1us
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000, true);   // 1us * 1000 = 1ms => timer1 interrupt함수는 1ms 마다 동작  
  timerAlarmEnable(timer);

  pinMode(_LED_PWM_GPIO, OUTPUT);
  pinMode(_LED_GND_GPIO, OUTPUT);      
  digitalWrite(_LED_PWM_GPIO, LOW); 
  digitalWrite(_LED_GND_GPIO, LOW); 

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(_LED_PWM_GPIO, ledChannel);

  WiFi.disconnect(); 

  //wifi connecting 
  Serial.println("");
  Serial.println("WiFi Connecting to IP");
  Serial.print("ssid : ");
  Serial.print(ssid);
  Serial.print(", ");
  Serial.print("password : ");
  Serial.println(password);

  WiFi.begin(ssid, password);

  Serial.print("search");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  static char host_Response_OnlyOnce = 0;

  if(host_Response_OnlyOnce == 0){
    //wifi client host setting
    Serial.println("");
    Serial.println("WiFi Connecting to host");
    Serial.print("hostIP : ");
    Serial.print(hostIP);
    Serial.print(", ");
    Serial.print("hostPort : ");
    Serial.println(hostPort);
  
    
  
    Serial.print("search");
    while(!client.connect(hostIP, hostPort)) { 
      delay(500);
      Serial.print(".");
    }
    Serial.println("host Connected");

    host_Response_OnlyOnce = 1;
  }
  
  if(system_time_1ms >= 10){
    
    PulseSensorFunc();

    client.print(":");
    client.print(TimeDataCnt_10ms);
    client.print(",");
    client.print(heartBeatAD_Data);
    client.print(",");
    client.print(New_HeartBeatBPM);
    client.print("\n");

    TimeDataCnt_10ms = TimeDataCnt_10ms + 10;

    if(TimeDataCnt_10ms > 9999){
      TimeDataCnt_10ms = 0;
    }

//    Serial.print(heartBeatAD_Data);
//    Serial.print(", ");
//    Serial.print(Max_heartBeatAD_Data);
//    Serial.print(", ");
//    Serial.print(Min_heartBeatAD_Data);
//    Serial.print(", ");
//    Serial.print(heartBeatCnt_1ms);
//    Serial.print(", ");
//    Serial.print(Threshold_heartBeatAD_Data);
//    Serial.print(", ");
//    Serial.print(HeartBeatBPM);
//    Serial.print(", ");
//    Serial.print(New_HeartBeatBPM);
//    Serial.println("\n");

//    client.stop();

    system_time_1ms = 0;
  }
  



  
//  Serial.println(heartBeatAD_Data);


  
}
