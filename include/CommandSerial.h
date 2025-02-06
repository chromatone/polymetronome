#ifndef COMMANDSERIAL_H_
#define COMMANDSERIAL_H_

#include <Arduino.h>
#include <HardwareSerial.h>
#include <queue>

/**
 * @brief Design base on concept that some class may not always run
 * So it's good to add and remove at runtime to reduce memory usage
 */

enum commandState_en
{
    C_STOPPED,
    C_RUNNING,
};


typedef struct {
    String name;
    String description;
    void (*callback)(void* arg);
} command_t;

class CommandBase
{
public:
    commandState_en commandState;
    int id;
    CommandBase *next;
 

    virtual ~CommandBase() {}

    void remove(CommandBase **prevNext)
    {
        *prevNext = next;
        delete this;
    }

    void insert(CommandBase **prevNext)
    {
        next = *prevNext;
        *prevNext = this;
    }
    
    // virtual void update() = 0;
    virtual void parser(const std::vector<String>& cmd) = 0;
    virtual void help() = 0;
}; 


class CommandSystem
{
public:
    CommandBase *command_s; 
    int currentId; 
     

    CommandSystem()
    {
        command_s = 0; 
        Serial.println("CommandSystem initialized");
    }

    /// plays a command, and returns an idividual id
    int registerClass(CommandBase *command)
    {
        command->id = currentId;
        command->insert(&command_s);
        return currentId++;
    }
 
    std::vector<String> splitString(const String& input) {
        std::vector<String> result;
        int start = 0;
        int end = input.indexOf(' ');

        while (end != -1) {
            result.push_back(input.substring(start, end));
            start = end + 1;
            end = input.indexOf(' ', start);
        }

        // Add the last substring
        result.push_back(input.substring(start));

        return result;
    }


    inline void parser()
    { 
        if(!Serial.available()) return;
        
        String cmd = Serial.readStringUntil('\n');
        std::vector<String> cmdParts = splitString(cmd);
        Serial.println("Recv: " + cmd); 
        if(cmdParts.size() == 0) return;
        int mode = 0;
        if(cmdParts[0] == "help") mode = 1;

        CommandBase **command = &command_s;
        while (*command)
        {
            if ((*command)->commandState == C_STOPPED){ 
                (*command)->remove(command);
            }
            else{
                if(mode == 1) 
                    (*command)->help();
                else if(mode == 0)
                    (*command)->parser(cmdParts); 
                command = &(*command)->next;
            } 
        }
        return;
    }  

    /// stops commandState a command with an individual id
    void stop(int id)
    {
        CommandBase **effectp = &command_s;
        while (*effectp)
        {
            if ((*effectp)->id == id)
            {
                (*effectp)->remove(effectp);
                return;
            }
            effectp = &(*effectp)->next;
        }
    }

    void stop(CommandBase *command)
    {
        stop(command->id);
    }
};







 


    // inline void update() {
    //     if(Serial.available()) {
    //         String cmd = Serial.readStringUntil('\n');
    //         Serial.println(cmd);
    //         int noteInt = cmd.toInt();
    //         if(noteInt > 0 && noteInt < 128) {
    //             _audio.requestMIDIPlay(noteInt, 1);
    //         }
    //         if(cmd.startsWith("set")) {
    //             String set = cmd.substring(4);
    //             String param = set.substring(0, set.indexOf(' '));
    //             String value = set.substring(set.indexOf(' ')+1);
    //             Serial.println(set);
    //             Serial.println(param);
    //             Serial.println(value);
    //             if(param == "brightness") {
    //                 int valuePCT = constrain(value.toInt(), 0, 100); 
    //                 _sabreConfig.leds.setBrightnessPCT(valuePCT);
    //                 SaberConfigSave();
    //                 Serial.printf("Set brightness to %d \%\n", valuePCT);
    //             }
    //         }
    //         if(cmd=="on") {
    //             turnOn();
    //         }
    //         if(cmd=="off") {
    //             turnOff();
    //         }
    //         if(cmd=="play") {
    //             _audio.requestSDPlay("/iam_atomic.wav", 1, 1);
    //         }
    //         if(cmd=="wifi1") {
    //             _audio.requestSDPlay("/wificonnecting.wav", 1, 1);
    //         }
    //         if(cmd=="wifi2") {
    //             _audio.requestSDPlay("/wificonnected.wav", 1, 1);
    //         }
    //     }
    // }

#endif /* COMMANDSERIAL_H_ */