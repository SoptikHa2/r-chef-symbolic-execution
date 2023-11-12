#include "R_ext/Boolean.h"
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <Internal.h>
#include <Chef.h>
#include "s2e.h"
#include "Chef.h"

void R_StartSymbolicExecution() {
    struct S2E_CHEF_COMMAND cmd;
    cmd.Command = START_CHEF;

    s2e_invoke_plugin("Chef", &cmd, sizeof(cmd));
}

void R_EndSymbolicExecution(int errorHappened) {
    struct S2E_CHEF_COMMAND cmd;
    cmd.Command = END_CHEF;
    cmd.data.end_chef.error_happened = errorHappened;

    s2e_invoke_plugin("Chef", &cmd, sizeof(cmd));
}

void R_UpdateHighLevelInstruction(u_int32_t opcode, uint32_t line, const char * filename, const char * funcname) {
    struct S2E_CHEF_COMMAND cmd;
    cmd.Command = TRACE_UPDATE;
    cmd.data.trace.op_code = opcode;
    strncpy((char *)cmd.data.trace.filename, filename ? filename : "<no support>", 60);
    strncpy((char *)cmd.data.trace.function, funcname ? funcname : "<no support>", 60);
    cmd.data.trace.line = line;
    cmd.data.trace.pc = 0;

    s2e_invoke_plugin("Chef", &cmd, sizeof(cmd));
}

void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize) {
    s2e_make_symbolic(buffer, bufferSize, variableName);
}

void R_SendDebugMessage(const char * message) {
    s2e_message(message);
}

Rboolean R_SymbexEnabled() {
    const char * symbex = getenv("R_SYMBEX");
    if (symbex && strcmp(symbex, "1") == 0) return TRUE;
    return FALSE;
}

attribute_hidden SEXP do_chefDebugMessage(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP debug_msg = CAR(args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(debug_msg) || LENGTH(debug_msg) != 1)
        error(_("character argument expected"));

    R_SendDebugMessage(translateCharFP(STRING_ELT(debug_msg, 0)));

    return R_NilValue;
}

attribute_hidden SEXP do_chefStartSymbex(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    R_StartSymbolicExecution();

    return R_NilValue;
}

attribute_hidden SEXP do_chefEndSymbex(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP error_happened = CAR(args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!IS_SCALAR(error_happened, LGLSXP) || SCALAR_LVAL(error_happened) == NA_LOGICAL)
        error(_("expected one true/false value"));

    R_EndSymbolicExecution(SCALAR_LVAL(error_happened));

    return R_NilValue;
}

attribute_hidden SEXP do_chefSymbolicInt(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("character argument expected"));

    int symbolicValue;
    R_GenerateSymbolicVar(translateCharFP(STRING_ELT(variable_name, 0)), (void *)&symbolicValue, sizeof(symbolicValue));

    return ScalarInteger(symbolicValue);
}
