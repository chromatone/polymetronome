#ifndef MAINCOMMAND_H_
#define MAINCOMMAND_H_

#include <Arduino.h>
#include "CommandSerial.h"
#include "config.h"

class MainCommand : public CommandBase
{
private:
    std::vector<command_t> m_commands;
public:
    MainCommand() {  
        commandState = C_RUNNING;
    }

    ~MainCommand() {  }

    void addCallback(const String& name, const String& description, void (*callback)(void* arg) ) {
        command_t command;
        command.name = name;
        command.description = description;
        command.callback = callback;
        m_commands.push_back(command);
    }

    inline void runCallback(const String& fnName, void* arg) {
        for (auto& command : m_commands) {
            if(command.name == fnName) {
                command.callback(arg);
                return;
            }
        }
    } 

    virtual void help() {
        Serial.println("========= MainCommand help =========");
        for (auto& command : m_commands) {
            Serial.println(command.name + " -> " + command.description);
        }
    }

    virtual void parser(const std::vector<String>& cmd) {
        Serial.println("MainCommand parser");   
        if(cmd.size() > 0) {
            runCallback(cmd[0], (void*)&cmd);
        } 
    }

};

#endif /* MAINCOMMAND_H_ */