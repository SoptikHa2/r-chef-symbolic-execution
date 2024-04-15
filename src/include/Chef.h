#ifndef CHEF_H_
#define CHEF_H_

#include "Defn.h"

// For debugging purposes: emits line number of file of the location where is called and passes it to Chef.
// Useful for when symbolic execution gets stuck and we want to know where.
#ifndef chef_log
#define chef_log() do { if (R_SymbexEnabled()) { char buffer[128]; sprintf(buffer, "LOG: %s : %d\n", __FILE__, __LINE__); R_SendDebugMessage(buffer); } } while(0)
#endif

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

void R_UpdateHighLevelInstruction(u_int32_t opcode, u_int32_t pc, uint32_t line, const char * filename, const char * funcname);

void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize);

void R_SendDebugMessage(const char * message);

void R_Assume(int assumption);

Rboolean R_SymbexEnabled();

SEXP R_SymbolicInt(const char * name);
SEXP R_SymbolicRaw(const char * name, int length);
SEXP R_SymbolicReal(const char * name);
SEXP R_SymbolicVec(const char * name, int length);
SEXP R_SymbolicString(const char * name, int length);
SEXP R_SymbolicSymsxp(const char * name, int length);

#endif
