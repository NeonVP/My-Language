#include <math.h>

#include "frontend/Parser.h"

TreeData_t MakeNumber( int number ) {
    TreeData_t value = {};
    value.type = NODE_NUMBER;
    value.data.number = number;

    return value;
}

TreeData_t MakeOperation( OperationType operation ) {
    TreeData_t value = {};

    value.type = NODE_OPERATION;
    value.data.operation = operation;

    return value;
}

TreeData_t MakeVariable( char* variable ) {
    TreeData_t value = {};

    value.type = NODE_VARIABLE;
    value.data.variable = variable;

    return value;
}

int CompareDoubleToDouble( double a, double b, double eps ) {
    if ( fabs( a - b ) < eps )
        return 0;
    if ( a - b > eps )
        return 1;
    if ( a - b < -eps )
        return -1;

    return 0;
}