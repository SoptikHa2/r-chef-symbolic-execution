#include "R_ext/Boolean.h"
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <Internal.h>
#include <Chef.h>
#include "s2e.h"

// Start symbolic execution. It can be stopped and subsequently started multiple times.
// Executing this when symbolic execution is already running will result in a recoverable error.
// So it won't do anything, but there will be a nasty message in the log.
void R_StartSymbolicExecution() {
    struct S2E_CHEF_COMMAND cmd;
    cmd.Command = START_CHEF;

    s2e_invoke_plugin("Chef", &cmd, sizeof(cmd));
}

/// End symbolic execution. It can be later resumed.
/// If the first parameter is FALSE, nothing special will happen.
/// If the first parameter is TRUE, an error case will be logged and assignment of input variables will be generated to reach this state.
void R_EndSymbolicExecution(int errorHappened) {
    struct S2E_CHEF_COMMAND cmd;
    cmd.Command = END_CHEF;
    cmd.data.end_chef.error_happened = errorHappened;

    s2e_invoke_plugin("Chef", &cmd, sizeof(cmd));
}

/// Record execution of instruction with given option at given location.
/// The opcode is for performance purposes - Chef will prioritize states with diverse opcodes. Setting it to zero
/// does not break anything.
/// All other parameters are for debugging/log purposes only, they can be left out. Only the first 60 chars
/// of filename/funcname will be transfered.
attribute_hidden void R_UpdateHighLevelInstruction(u_int32_t opcode, u_int32_t pc, uint32_t line, const char * filename, const char * funcname) {
    struct S2E_CHEF_COMMAND cmd;
    cmd.Command = TRACE_UPDATE;
    cmd.data.trace.op_code = opcode;
    strncpy((char *)cmd.data.trace.filename, filename ? filename : "<no support>", 60);
    strncpy((char *)cmd.data.trace.function, funcname ? funcname : "<no support>", 60);
    cmd.data.trace.line = line;
    cmd.data.trace.pc = pc;

    s2e_invoke_plugin("Chef", &cmd, sizeof(cmd));
}


void R_SendDebugMessage(const char * message) {
    s2e_message(message);
}

/// If first parameter is false, kill the state (without marking it as error, thus without generating any testcases).
/// This can be used to implement arbitrary conditions for symbolic variables, in line of:
/// 1. generate symbolic int
/// 2. R_Assume(symbInt > 14 && symbInt & 0xFF == 0xAB);
void R_Assume(int assumption) {
    s2e_assume(assumption);
}

/// Symbolic execution has to be enabled by setting the envvar R_SYMBEX to "1". If
/// this is not the case, this function returns false.
Rboolean R_SymbexEnabled() {
    const char * symbex = getenv("R_SYMBEX");
    if (symbex && strcmp(symbex, "1") == 0) return TRUE;
    return FALSE;
}

/// R function: send debug message.
/// Exepcted one parameter, a string.
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

/// R function, start symbolic execution.
/// Expected zero parameters.
attribute_hidden SEXP do_chefStartSymbex(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    R_StartSymbolicExecution();

    return R_NilValue;
}

/// R function, end symbolic execution.
/// Expected one parmaeter, see R_EndSymbolicExecution.
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

/// R function, assume a condition.
/// Expected one parameter, a logical.
/// See R_Assume for details.
attribute_hidden SEXP do_chefAssume(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP assumption;
    PROTECT(assumption = eval(CAR(args), env));

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));


    R_Assume(R_AsLogicalNoNA(assumption, call, env));

    UNPROTECT(1);

    return R_NilValue;
}

/// R function, assert a condition.
/// Expected one parameter, a logical.
/// If the condition is false, symbolic execution will be stopped and a testcase will be generated.
attribute_hidden SEXP do_chefAssert(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP assertion;
    PROTECT(assertion = eval(CAR(args), env));

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));


    if (!R_AsLogicalNoNA(assertion, call, env)) {
        R_SendDebugMessage("Assertion failed");
        R_EndSymbolicExecution(1);
    }

    UNPROTECT(1);

    return R_NilValue;
}

/// R function, generate symbolic integer.
/// Expected one parmatere (variable name, a string).
SEXP do_chefSymbolicInt(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);

    if (!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("character argument expected"));

    return R_SymbolicInt(translateCharFP(STRING_ELT(variable_name, 0)));
}

/// R function, generate symbolic byte vector.
/// Expected two parameters: variable name (a string) and length (an integer or real (will be rounded)).
SEXP do_chefSymbolicBytes(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);
    SEXP string_length = CAR(CDR(args));

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("first argument: expected string (variable name)"));

    if (!isReal(string_length) && !isInteger(string_length))
        error(_("second argument: expected int (result length)"));

    int bufferLength;
    if isReal(string_length)
    bufferLength = lround(Rf_asReal(string_length));
    else
    bufferLength = Rf_asInteger(string_length);

    if (bufferLength <= 0)
        error(_("second argument: expected at least 1"));

    return R_SymbolicRaw(translateCharFP(STRING_ELT(variable_name, 0)), bufferLength);
}


/// Generate symbolic numeric value (REALSXP).
SEXP do_chefSymbolicReal(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("first argument: expected string (variable name)"));


    return R_SymbolicReal(translateCharFP(STRING_ELT(variable_name, 0)));
}


SEXP do_chefSymbolicVec(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);
    SEXP string_length = CAR(CDR(args));

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("first argument: expected string (variable name)"));

    if (!isReal(string_length) && !isInteger(string_length))
        error(_("second argument: expected int (result length)"));

    int bufferLength;
    if isReal(string_length)
    bufferLength = lround(Rf_asReal(string_length));
    else
    bufferLength = Rf_asInteger(string_length);

    if (bufferLength <= 0)
        error(_("second argument: expected at least 1"));

    return R_SymbolicVec(translateCharFP(STRING_ELT(variable_name, 0)), bufferLength);
}


/// R function, generate symbolic string.
/// Expected two parameters: variable name (a string) and length (an integer or real (will be rounded)).
/// The actual generated string might be shorter than the requested length if a nullbyte occurs somewhere
/// in the symbolically generated args. The length provided is the maximum size of the string + 1 (because of a nullbyte).
SEXP do_chefSymbolicString(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);
    SEXP string_length = CAR(CDR(args));

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("first argument: expected string (variable name)"));

    if (!isReal(string_length) && !isInteger(string_length))
        error(_("second argument: expected int (result length)"));

    int bufferLength;
    if isReal(string_length)
    bufferLength = lround(Rf_asReal(string_length));
    else
    bufferLength = Rf_asInteger(string_length);

    if (bufferLength <= 0)
        error(_("second argument: expected at least 1"));

    return R_SymbolicString(translateCharFP(STRING_ELT(variable_name, 0)), bufferLength);
}


SEXP do_chefSymbolicAny(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name_arg = CAR(args);
    SEXP string_length = CAR(CDR(args));

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name_arg) || LENGTH(variable_name_arg) != 1)
        error(_("first argument: expected string (variable name)"));

    if (!isReal(string_length) && !isInteger(string_length))
        error(_("second argument: expected int (result length)"));

    int bufferLength;
    if isReal(string_length)
    bufferLength = lround(Rf_asReal(string_length));
    else
    bufferLength = Rf_asInteger(string_length);

    if (bufferLength <= 0)
        error(_("second argument: expected at least 1"));

    const char * variable_name = translateCharFP(STRING_ELT(variable_name_arg, 0));

    char symbValTypeName[180];
    snprintf(symbValTypeName, 180, "%s_type", variable_name);
    symbValTypeName[179] = 0;

    unsigned symbValType;
    R_GenerateSymbolicVar(symbValTypeName, &symbValType, sizeof(symbValType));

    switch(symbValType) {
        case 0:
            return R_SymbolicInt(variable_name);
        case 1:
            return R_SymbolicRaw(variable_name, bufferLength);
        case 2:
            return R_SymbolicReal(variable_name);
        case 3:
            return R_SymbolicVec(variable_name, bufferLength);
        case 4:
            return R_NilValue;
        case 5:
            return R_SymbolicString(variable_name, bufferLength);
        case 6:
            return R_SymbolicSymsxp(variable_name, bufferLength);
        case 7:
            return R_SymbolicListsxp(variable_name, bufferLength);
        default:
            R_Assume(0); // not valid, will kill this state
            return NULL;
    }
}
