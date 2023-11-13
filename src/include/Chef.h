#ifndef CHEF_H_
#define CHEF_H_

#include "Defn.h"

enum S2E_CHEF_COMMANDS {
    START_CHEF,
    END_CHEF,
    TRACE_UPDATE
};

struct S2E_CHEF_COMMAND {
    enum S2E_CHEF_COMMANDS Command;
    union {
        struct {
        } start_chef;
        struct {
            uint8_t error_happened;
        } end_chef;
        struct {
            uint32_t op_code;
            uint32_t pc;
            uint32_t line;
            uint8_t function[61];
            uint8_t filename[61];
        } trace;
    } data;
};

void R_StartSymbolicExecution();

void R_EndSymbolicExecution(int errorHappened);

void R_UpdateHighLevelInstruction(u_int32_t opcode, uint32_t line, const char * filename, const char * funcname);

void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize);

void R_SendDebugMessage(const char * message);

void R_Assume(int assumption);

Rboolean R_SymbexEnabled();

#endif
