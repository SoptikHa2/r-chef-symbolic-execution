#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <Chef.h>
#include "s2e.h"


/// Mark given buffer with given size as symbolic. Register it with variable named, that will be used
/// for logging and marking variables for user.
attribute_hidden void R_GenerateSymbolicVar(const char * variableName, void * buffer, size_t bufferSize) {
    s2e_make_symbolic(buffer, bufferSize, variableName);
}

SEXP R_SymbolicInt(const char * varName) {
    char name[180];
    snprintf(name, 180, "int__%s", varName);
    name[179] = 0;

    int32_t symbolicValue;
    R_GenerateSymbolicVar(name, (void *)&symbolicValue, sizeof(symbolicValue));

    R_Assume(symbolicValue != -2147483648); // Out of bounds for R, outputs NA, which is not desired for this function

    return ScalarInteger(symbolicValue);
}

SEXP R_SymbolicRaw(const char * varName, int length) {
    char name[180];
    snprintf(name, 180, "raw__%s", varName);
    name[179] = 0;

    SEXP ans = allocVector(RAWSXP, length);
    R_GenerateSymbolicVar(name, (void *)RAW(ans), length);

    return ans;
}

SEXP R_SymbolicReal(const char * varName) {
    char name[180];
    snprintf(name, 180, "real__%s", varName);
    name[179] = 0;

    double symbolicValue;
    R_GenerateSymbolicVar(name, (void *)&symbolicValue, sizeof(symbolicValue));

    return ScalarReal(symbolicValue);
}

SEXP R_SymbolicVec(const char * varName, int length) {
    char vecTypeName[180];
    snprintf(vecTypeName, 180, "vec_type__%s", varName);
    vecTypeName[179] = 0;

    char vecDataName[180];
    snprintf(vecDataName, 180, "vec_data__%s", varName);
    vecDataName[179] = 0;

    SEXPTYPE vecType;
    R_GenerateSymbolicVar(vecTypeName, (void *)&vecType, sizeof(vecType));
    // STRSXP, VECSXP and EXPRSXP are not supported
    R_Assume(vecType == LGLSXP || vecType == INTSXP || vecType == REALSXP || vecType == CPLXSXP || vecType == RAWSXP);

    SEXP vector = PROTECT(allocVector(vecType, length));

    if (vecType == LGLSXP) {
        R_GenerateSymbolicVar(vecDataName, (void *)LOGICAL(vector), length * sizeof(Rboolean));
    } else if (vecType == INTSXP) {
        R_GenerateSymbolicVar(vecDataName, (void *)INTEGER(vector), length * sizeof(int));
    } else if (vecType == REALSXP) {
        R_GenerateSymbolicVar(vecDataName, (void *)REAL(vector), length * sizeof(double));
    } else if (vecType == CPLXSXP) {
        R_GenerateSymbolicVar(vecDataName, (void *)COMPLEX(vector), length * sizeof(Rcomplex));
    } else if (vecType == STRSXP) {
        // ...
    } else if (vecType == VECSXP) {
        // ...
    } else if (vecType == RAWSXP) {
        R_GenerateSymbolicVar(vecDataName, (void *)RAW(vector), length * sizeof(Rbyte));
    } else if (vecType == EXPRSXP) {
        // ...
    }

    UNPROTECT(1);
    return vector;
}

SEXP R_SymbolicString(const char * varName, int length) {
    char name[180];
    snprintf(name, 180, "str__%s", varName);
    name[179] = 0;

    SEXP ans, charsxp;
    PROTECT(ans = allocVector(STRSXP, 1));

    // Create a temp string that we will use to generate enough space for the real string
    // It will be deleted later, but I didn't figure out yet how to do it better
    char * tmpString = (char *)malloc(length+1);
    tmpString[length] = 0;

    R_GenerateSymbolicVar(name, tmpString, length);
    for(int i = 0; i < length; i++) {
        R_Assume(tmpString[i] != 0);
    }

    PROTECT(charsxp = mkCharLen(tmpString, length));
    SET_STRING_ELT(ans, 0, charsxp);

    free(tmpString);

    UNPROTECT(2);

    return ans;
}

SEXP R_SymbolicSymsxp(const char * varName, int length) {
    char name[180];
    snprintf(name, 180, "sym__%s", varName);
    name[179] = 0;

    SEXP symstr = PROTECT(R_SymbolicString(name, length));

    SEXP symsxp = mkSYMSXP(STRING_ELT(symstr, 0), R_UnboundValue);

    UNPROTECT(1);
    return symsxp;
}
