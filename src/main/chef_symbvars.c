#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <Internal.h>
#include <Chef.h>
#include "s2e.h"


/// Mark given buffer with given size as symbolic. Register it with variable named, that will be used
/// for logging and marking variables for user.
attribute_hidden void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize) {
    s2e_make_symbolic(buffer, bufferSize, variableName);
}

/// R function, generate symbolic integer.
/// Expected one parmatere (variable name, a string).
SEXP do_chefSymbolicInt(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("character argument expected"));

    int32_t symbolicValue;
    R_GenerateSymbolicVar(translateCharFP(STRING_ELT(variable_name, 0)), (void *)&symbolicValue, sizeof(symbolicValue));

    return ScalarInteger(symbolicValue);
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
    bufferLength = lround(Rf_asReal(string_length)) + 1;
    else
    bufferLength = Rf_asInteger(string_length);

    if (bufferLength <= 0)
        error(_("second argument: expected positive value"));

    char * buf = malloc(bufferLength);

    buf[bufferLength-1] = 0;

    SEXP ans = allocVector(RAWSXP, bufferLength);
    R_GenerateSymbolicVar(translateCharFP(STRING_ELT(variable_name, 0)), (void *)RAW(ans), bufferLength);

    return ans;
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
    bufferLength = lround(Rf_asReal(string_length)) + 1;
    else
    bufferLength = Rf_asInteger(string_length);

    if (bufferLength <= 0)
        error(_("second argument: expected positive value"));

    char * buf = malloc(bufferLength);

    R_GenerateSymbolicVar(translateCharFP(STRING_ELT(variable_name, 0)), (void *)&buf, bufferLength-1);
    buf[bufferLength-1] = 0;

    return mkString(buf);
}


/// Generate symbolic numeric value (REALSXP).
SEXP do_chefSymbolicReal(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name = CAR(args);

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name) || LENGTH(variable_name) != 1)
        error(_("first argument: expected string (variable name)"));


    double symbolicValue;
    R_GenerateSymbolicVar(translateCharFP(STRING_ELT(variable_name, 0)), (void *)&symbolicValue, sizeof(symbolicValue));

    return ScalarReal(symbolicValue);
}
