#include <Arduino.h> 
#include "CommandSerial.h"
#include "MainCommand.h"

#define FET_PIN 16
#define PIEZO_PIN 17

#define ON_STATE_DURATION_MS 7 // 7ms
 

volatile uint32_t _last_hall = 0;
uint32_t _time_offset = 0;
 
uint32_t f_bpm = 100; 

CommandSystem	_commandSystem;
MainCommand 	_cmdMain;

void setup() { 
  Serial.begin(115200); 
  pinMode(FET_PIN, OUTPUT);

  pinMode(PIEZO_PIN, INPUT_PULLUP); 

  _cmdMain.addCallback("bpm", "Set bpm", [](void* arg){
      std::vector<String> cmd = *(std::vector<String>*)arg;
      if(cmd.size() < 2) return; 
      f_bpm = cmd[1].toInt();
  });

  _commandSystem.registerClass(&_cmdMain);


  attachInterrupt(digitalPinToInterrupt(PIEZO_PIN), [](){
    if(millis() - _last_hall>100) { 
      _last_hall = millis();
    }
  }, FALLING);

}

void loop() { 
  static uint32_t last_t =0;
  static uint32_t last_hall_detected = 0;


  _commandSystem.parser(); 
  uint32_t t_now = millis();
  uint32_t t_split = 1000*60/(f_bpm);
  static bool last_state = false;
  bool state = (millis()%t_split) < ON_STATE_DURATION_MS;
  digitalWrite(FET_PIN, state);
  if(last_state!=state) {
    last_state = state;
    Serial.printf("%d hall %d vs last trig %d\n", state, _last_hall, millis());
  }
  if(_last_hall != last_hall_detected) {
    last_hall_detected = _last_hall;
    Serial.printf("hall detected %d\n", last_hall_detected);
  }
}
 