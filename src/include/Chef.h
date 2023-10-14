#ifndef CHEF_H_
#define CHEF_H_

#include "Defn.h"

void R_StartSymbolicExecution();

void R_EndSymbolicExecution(Rboolean errorHappened);

void R_UpdateHighLevelInstruction(u_int32_t opcode);

void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize);

void R_SendDebugMessage(const char * message);

#endif
