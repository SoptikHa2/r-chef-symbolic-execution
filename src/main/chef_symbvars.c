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

SEXP R_SymbolicInt(const char * name) {
    int32_t symbolicValue;
    R_GenerateSymbolicVar(name, (void *)&symbolicValue, sizeof(symbolicValue));

    R_Assume(symbolicValue != -2147483648); // Out of bounds for R, outputs NA, which is not desired for this function

    return ScalarInteger(symbolicValue);
}

SEXP R_SymbolicRaw(const char * name, int length) {
    SEXP ans = allocVector(RAWSXP, length);
    R_GenerateSymbolicVar(name, (void *)RAW(ans), length);

    return ans;
}

SEXP R_SymbolicReal(const char * name) {
    double symbolicValue;
    R_GenerateSymbolicVar(name, (void *)&symbolicValue, sizeof(symbolicValue));

    return ScalarReal(symbolicValue);
}

SEXP R_SymbolicVec(const char * name, int length) {
    char vecTypeVarName[180];
    snprintf(vecTypeVarName, 180, "%s_type", name);
    vecTypeVarName[179] = 0;

    SEXPTYPE vecType;
    R_GenerateSymbolicVar(vecTypeVarName, (void *)&vecType, sizeof(vecType));
    // STRSXP, VECSXP and EXPRSXP are not supported
    R_Assume(vecType == LGLSXP || vecType == INTSXP || vecType == REALSXP || vecType == CPLXSXP || vecType == RAWSXP);

    SEXP vector = PROTECT(allocVector(vecType, length));

    if (vecType == LGLSXP) {
        R_GenerateSymbolicVar(name, (void *)LOGICAL(vector), length * sizeof(Rboolean));
    } else if (vecType == INTSXP) {
        R_GenerateSymbolicVar(name, (void *)INTEGER(vector), length * sizeof(int));
    } else if (vecType == REALSXP) {
        R_GenerateSymbolicVar(name, (void *)REAL(vector), length * sizeof(double));
    } else if (vecType == CPLXSXP) {
        R_GenerateSymbolicVar(name, (void *)COMPLEX(vector), length * sizeof(Rcomplex));
    } else if (vecType == STRSXP) {
        // ...
    } else if (vecType == VECSXP) {
        // ...
    } else if (vecType == RAWSXP) {
        R_GenerateSymbolicVar(name, (void *)RAW(vector), length * sizeof(Rbyte));
    } else if (vecType == EXPRSXP) {
        // ...
    }

    UNPROTECT(1);
    return vector;
}

/// R function, generate symbolic string.
/// Expected two parameters: variable name (a string) and length (an integer or real (will be rounded)).
/// The actual generated string might be shorter than the requested length if a nullbyte occurs somewhere
/// in the symbolically generated args. The length provided is the maximum size of the string + 1 (because of a nullbyte).
SEXP do_chefSymbolicString(SEXP call, SEXP op, SEXP args, SEXP env) {
    printf("Symbstring\n");
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

    printf("0\n");

    SEXP ans, charsxp;
    PROTECT(ans = allocVector(STRSXP, 1));
    printf("2\n");
    // Create a temp string that we will use to generate enough space for the real string
    // It will be deleted later, but I didn't figure out yet how to do it better, eg with mkCharLen.
    char * tmpString = (char *)malloc(bufferLength);
    for (int i = 0; i < bufferLength; i++) {
        tmpString[i] = 'A';
    }
    tmpString[bufferLength-1] = 0;
    PROTECT(charsxp = mkCharLen("", bufferLength)); // TODO: disable cache
    printf("3\n");
    ((char *)STDVEC_DATAPTR(charsxp))[bufferLength-1] = 0; // terminating nullbyte
    SET_STRING_ELT(ans, 0, charsxp);
    printf("A\n");
    R_GenerateSymbolicVar(translateCharFP(STRING_ELT(variable_name, 0)), (void *)CHAR(charsxp), bufferLength-1);
    printf("B\n");

    // assume that there are no imm nullbytes
    for(int i = 0; i < bufferLength; i++) {
        printf("C\n");
        R_Assume(CHAR(charsxp)[i] != 0);
        printf("D\n");
    }
    printf("E\n");

    free(tmpString);

    UNPROTECT(2);

    return ans;
}
