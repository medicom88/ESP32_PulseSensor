#include <WiFi.h>

WiFiServer server(8080);

char ssid[30]     = "KeplerAP";
char password[30] = "hybus12345";
char hostIP[30]   = "192.168.4.1";
int hostPort = 8080;


hw_timer_t * timer = NULL;
volatile int system_time_1ms = 0;
volatile int WiFi_Client_State_Tick_1ms = 0;
void IRAM_ATTR onTimer(){
  system_time_1ms++;
  WiFi_Client_State_Tick_1ms++;
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  timer = timerBegin(1, 80, true);      // 1 / (80MHz / N) => N:80 => 1 / timerFreq = 1 / 1MHz = 1us
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000, true);   // 1us * 1000 = 1ms => timer1 interrupt함수는 1ms 마다 동작  
  timerAlarmEnable(timer);

  //wifi AP connecting 
  Serial.println("");
  Serial.println("WiFi AP Connecting");
  Serial.print("ssid : ");
  Serial.print(ssid);
  Serial.print(", ");
  Serial.print("password : ");
  Serial.println(password);
  
  WiFi.softAP(ssid, password);

  Serial.println("");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  //wifi Server connecting 
  Serial.println("");
  Serial.println("WiFi Server Connecting");
  server.begin();
  Serial.println("Sever Ready");
  
}

void loop() {
  // put your main code here, to run repeatedly:

  if(system_time_1ms >= 10){
    WiFiClient client = server.available();   // listen for incoming clients
    if (client) {                             // if you get a client,
      Serial.println("new client");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
            char c = client.read();             // read a byte, then
            Serial.write(c);                    // print it out the serial monitor
            WiFi_Client_State_Tick_1ms = 0;
        }
        if(WiFi_Client_State_Tick_1ms > 5000){
          client.stop();
          Serial.println("client disconnect");
        }
      }
      
    }
    system_time_1ms = 0;
  }
}
















