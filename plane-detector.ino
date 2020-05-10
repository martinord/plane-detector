#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

static const u1_t PROGMEM APPEUI[8] = {  };
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

static const u1_t PROGMEM DEVEUI[8] = {  };
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

static const u1_t PROGMEM APPKEY[16] = {  };
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}


// App variables
const int pinAdc = A5;
const int threshold = 15500;
unsigned long sendDelay = 60000; // 10 min
int counter = 0;
unsigned long t1, t2;



void do_send(osjob_t* j, int counter) {
     byte payload[2];
     
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
        
      payload[0] = highByte(counter);
      payload[1] =lowByte(counter);
     
      LMIC_setTxData2(1, payload, sizeof(payload), 0);
      Serial.println(F("Packet queued"));
       
    }
}


bool joined = false;
bool sleeping = false;

static osjob_t sendjob;
static osjob_t initjob;

static void initfunc (osjob_t*);

void setup() {
    delay(3000);
    Serial.begin(9600); 
    Serial.println(F("Welome to plane detector v.0.2..."));
    t1 = millis();

#ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
#endif
  delay(5000);

  os_init();
  os_setCallback(&initjob, initfunc);
  LMIC_reset();
}


void loop() {
  if (joined == false){
    os_runloop_once();
  }
  else{
   
    // -----------------------------------------------------------------//
    long sum = 0;
    bool plane = 0;
    
    // Main loop. Stops when detecting a plane for 10 sec
    while(!plane){
      os_runloop_once();
      for(int i=0; i<2000; i++)
      {
          sum += analogRead(pinAdc);
      }
  
      sum >>= 5;

      if (sum>threshold){
        Serial.println("Plane detected!");
        plane = 1;
        counter++;
      }
      else{
        delay(400);
      }

      t2 = millis();
      unsigned long dif_time = t2-t1;
      
      if (dif_time > sendDelay){
        
        // SEND
        do_send(&sendjob,counter);
        os_runloop_once();
        
        // Reset counter and time
        counter = 0;
        t1 = millis();
        
      }
    }
   // Plane detected, waiting... 
   delay(15000);

   // -----------------------------------------------------------------//
  }

}

static void initfunc (osjob_t* j) {
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
  LMIC_startJoining();
  // init done - onEvent() callback will be invoked...
}

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 8,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 4,
  .dio = {7, 1, LMIC_UNUSED_PIN},
};

void onEvent (ev_t ev) {
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      LMIC_setLinkCheckMode(0);
      joined = true;
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      os_setCallback(&initjob, initfunc);
      break;
    case EV_TXCOMPLETE:
      sleeping = true;
      if (LMIC.dataLen) {
        Serial.print(F("Data Received: "));
        Serial.println(LMIC.frame[LMIC.dataBeg], HEX);
      }
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      delay(50);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}
