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

    SEXP ans = PROTECT(allocVector(RAWSXP, bufferLength));
    R_GenerateSymbolicVar(translateCharFP(STRING_ELT(variable_name, 0)), (void *)RAW(ans), bufferLength);

    UNPROTECT(1);
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

    SEXP ans = mkString(buf);
    free(buf);
    return ans;
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


SEXP do_chefSymbolicVec(SEXP call, SEXP op, SEXP args, SEXP env) {
    checkArity(op, args);
    SEXP variable_name_arg = CAR(args);
    SEXP vec_length_arg = CAR(CDR(args));

    if(!R_SymbexEnabled())
        error(_("Symbolic execution is not enabled. Use envvar R_SYMBEX=1."));

    if (!isString(variable_name_arg) || LENGTH(variable_name_arg) != 1)
        error(_("first argument: expected string (variable name)"));

    if (!isReal(vec_length_arg) && !isInteger(vec_length_arg))
        error(_("second argument: expected int (result length)"));

    int vec_length;
    if isReal(vec_length_arg)
        vec_length = lround(Rf_asReal(vec_length_arg)) + 1;
    else
        vec_length = Rf_asInteger(vec_length_arg);

    const char * variable_name = translateCharFP(STRING_ELT(variable_name_arg, 0));

    char vecTypeVarName[180];
    snprintf(vecTypeVarName, 180, "%s_type", variable_name);
    vecTypeVarName[179] = 0;

    SEXPTYPE vecType;
    R_GenerateSymbolicVar(vecTypeVarName, (void *)&vecType, sizeof(vecType));
    // STRSXP, VECSXP and EXPRSXP are not supported
    R_Assume(vecType == LGLSXP || vecType == INTSXP || vecType == REALSXP || vecType == CPLXSXP || vecType == RAWSXP);

    SEXP vector = PROTECT(allocVector(vecType, vec_length));

    if (vecType == LGLSXP) {
        R_GenerateSymbolicVar(variable_name, (void *)LOGICAL(vector), vec_length * sizeof(Rboolean));
    } else if (vecType == INTSXP) {
        R_GenerateSymbolicVar(variable_name, (void *)INTEGER(vector), vec_length * sizeof(int));
    } else if (vecType == REALSXP) {
        R_GenerateSymbolicVar(variable_name, (void *)REAL(vector), vec_length * sizeof(double));
    } else if (vecType == CPLXSXP) {
        R_GenerateSymbolicVar(variable_name, (void *)COMPLEX(vector), vec_length * sizeof(Rcomplex));
    } else if (vecType == STRSXP) {
        // ...
    } else if (vecType == VECSXP) {
        // ...
    } else if (vecType == RAWSXP) {
        R_GenerateSymbolicVar(variable_name, (void *)RAW(vector), vec_length * sizeof(Rbyte));
    } else if (vecType == EXPRSXP) {
        // ...
    }

    UNPROTECT(1);
    return vector;
}
