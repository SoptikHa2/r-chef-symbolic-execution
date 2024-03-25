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

SEXP R_SymbolicString(const char * name, int length) {
    SEXP ans, charsxp;
    PROTECT(ans = allocVector(STRSXP, 1));

    // Create a temp string that we will use to generate enough space for the real string
    // It will be deleted later, but I didn't figure out yet how to do it better
    char * tmpString = (char *)malloc(length);
    memset(tmpString, 'A', length);

    PROTECT(charsxp = mkCharLen(tmpString, length)); // TODO: disable string cache
    SET_STRING_ELT(ans, 0, charsxp);
    R_GenerateSymbolicVar(name, STDVEC_DATAPTR(charsxp), length);

    free(tmpString);

    UNPROTECT(2);

    return ans;
}
