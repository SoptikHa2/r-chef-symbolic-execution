#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <Internal.h>
#include <Chef.h>
#include "s2e.h"

void R_StartSymbolicExecution() {}

void R_EndSymbolicExecution(Rboolean errorHappened) {}

void R_UpdateHighLevelInstruction(u_int32_t opcode) {}

void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize) {}

void R_SendDebugMessage(const char * message) {
    printf("Will print the following message to S2E debug: %s\n", message);
}

attribute_hidden SEXP do_chefDebugMessage(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP debug_msg = CAR(args);

    if (!isString(debug_msg) || LENGTH(debug_msg) != 1)
        error(_("character argument expected"));

    R_SendDebugMessage(translateCharFP(STRING_ELT(debug_msg, 0)));

    return debug_msg;
}
