#ifndef CHEF_H_
#define CHEF_H_

#include "Defn.h"

typedef enum S2E_CONCOLICSESSION_COMMANDS {
    START_CONCOLIC_SESSION,
    END_CONCOLIC_SESSION
} S2E_CONCOLICSESSION_COMMANDS;

struct S2E_CONCOLICSESSION_COMMAND {
    S2E_CONCOLICSESSION_COMMANDS Command;
    uint32_t max_time;
    uint8_t is_error_path;
    uint32_t result_ptr;
    uint32_t result_size;
} __attribute__((packed));

enum S2E_INTERPRETERMONITOR_COMMANDS {
    TraceUpdate
};

struct S2E_INTERPRETERMONITOR_COMMAND {
    // There is just one command - TraceUpdate.
    // In order to not break compatibility with
    // existing Chef code, this has to be left out.
    //S2E_INTERPRETERMONITOR_COMMANDS Command;
    //union {
    uint32_t op_code;
    uint32_t frame_count;
    uint32_t frames[2];
    uint32_t line;
    uint8_t function[61];
    uint8_t filename[61];
    //};
} __attribute((packed));

void R_StartSymbolicExecution();

void R_EndSymbolicExecution(int errorHappened);

void R_UpdateHighLevelInstruction(u_int32_t opcode);

void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize);

void R_SendDebugMessage(const char * message);

#endif
