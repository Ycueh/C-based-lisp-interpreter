#include "nuclei.h"

int main(int argc, char* argv[]) {
    Prog* p = calloc(1, sizeof(Prog));
    p->lispStack = stack_init();
    char* filePath = NULL;
    if (argc > 1) {
        filePath = argv[1];
    }
    readFile(filePath, p);
    PROG(p);
#ifdef INTERP
    //Create a lisp stack
    atom_free(p);
#else
    printf("Parsed OK");
#endif
    stack_free(p->lispStack);
    free(p);
    return 0;
}

void readFile(char* filePath, Prog* p) {
    FILE* fp = NULL;
    char row[LINESIZE];
    int charIndex = 0;
    char token[TOKENSIZE] = { 0 };
    fp = fopen(filePath, "r");
    if (fp == NULL) {
        fprintf(stderr, "The file could not open");
        exit(EXIT_FAILURE);
    }
    while (fgets(row, LINESIZE, fp) != NULL) {
        int strLength = strlen(row);
        if (strLength > 1) {
            row[strLength - 1] = '\0';
        }
        else if (row[strLength - 1] == '\n') {
            row[strLength - 1] = '\0';
        }
        splitLine(p, row, charIndex, token);
    }
    fclose(fp);
}

void atom_free(Prog* p) {
    int length = stack_size(p->lispStack);
    stack* lispStack = p->lispStack;
    while (length != 0) {
        stacktype varName;
        stack_pop(lispStack, &varName);
        int index = varName - 65;
        variable var = p->variableList[index];
        if (var.isLisp) {
#ifdef INTERP
            lisp_free(&var.atom);
            var.isLisp = false;
            var.atom = NIL;
#endif
            p->variableList[index].isLisp = NULL;
        }
        length = stack_size(p->lispStack);
    }
}

void splitLine(Prog* p, char* content, int charIndex, char* token) {
    if (content[charIndex] == '\0' || content[0] == '#') {
        if (token[0] == '\'' || token[0] == '\"') {
            strcpy(p->str[p->tokenIndex], token);
        }
        return;
    }
    if (token[0] == '\'' || token[0] == '\"') {
        strncat(token, &content[charIndex], 1);
        if (content[charIndex] == token[0]) {
            //If the current char is ' or "
            strcpy(p->str[p->tokenIndex], token);
            token[0] = '\0';
            p->tokenIndex++;
        }
    }
    else if (content[charIndex] == ' ' || content[charIndex] == ')' || content[charIndex] == '(') {
        if (token[0] != '\0') {
            strcpy(p->str[p->tokenIndex], token);
            token[0] = '\0';
            p->tokenIndex++;
        }
        if (content[charIndex] == ')' || content[charIndex] == '(') {
            //Save the ")"
            strncat(token, &content[charIndex], 1);
            strcpy(p->str[p->tokenIndex], token);
            token[0] = '\0';
            p->tokenIndex++;
        }
    }
    else {
        //Attach the char
        strncat(token, &content[charIndex], 1);
    }
    splitLine(p, content, charIndex + 1, token);
}

void PROG(Prog* p) {
    //"("<INSTRCTS>
    if (strcmp(p->str[p->count], "(") != 0) {
        ERROR("No Begin ( ?");
    }
    bracketCheck(p);
    INSTRCTS(p);
    if (p->str[p->count + 1][0] != '\0') {
        INSTRCTS(p);
    }
}

void INSTRCTS(Prog* p) {
    //<INSTRCT><INSTRCTS> | ")"
    p->count = p->count + 1;
    if (strcmp(p->str[p->count], ")") == 0) {
        return;
    }
    else {
        INSTRCT(p);
        INSTRCTS(p);
    }
}

void INSTRCT(Prog* p) {
    //"("<FUNC>")"
    if (strcmp(p->str[p->count], "(") == 0) {
        p->count = p->count + 1;
        Func(p);
        p->count = p->count + 1;
        if (strcmp(p->str[p->count], ")") == 0) {
            return;
        }
        else {
            ERROR("No right brackets ) ?");
        }
    }
    else {
        ERROR("No left brackets ( ?");
    }
}

void Func(Prog* p) {
    //<RETFUNC>| <IOFUNC> | <IF> | <LOOP>
    char func[BIGSTR] = { 0 };
    strcpy(func, p->str[p->count]);
    if (strcmp(func, "IF") == 0) {//IF
        p->count = p->count + 1;
        If(p);
    }
    else if (strcmp(func, "WHILE") == 0) {//LOOP
        p->count = p->count + 1;
        Loop(p);
    }
    else if (strcmp(func, "SET") == 0 || strcmp(func, "PRINT") == 0) {//IOFUNC
        ioFunc(p);
    }
    else {
        retFunc(p);
    }
}

variable retFunc(Prog* p) {
    //INTFUNC
    variable return_var = { .atom = NIL,.isNIL = false,.isLisp = false,.isNumber = false,.name = '\0',
                            .number = 0,.isAssigned = false,.boolResult = false,.isBoolResult = false };
    char token[BIGSTR] = { 0 };
    strcpy(token, p->str[p->count]);
    if (strcmp(token, "PLUS") == 0 || strcmp(token, "LENGTH") == 0) {
#ifdef INTERP
        return_var.number = intFunc(p);
        return_var.isNumber = true;
#else
        intFunc(p);
#endif
    }
    //BOOLFUNC
    else if (strcmp(token, "LESS") == 0 || strcmp(token, "GREATER") == 0 ||
        strcmp(token, "EQUAL") == 0) {
#ifdef INTERP
        return_var.number = boolFunc(p);
        return_var.isBoolResult = true;
#else
        boolFunc(p);
#endif
    }
    //LISTFUNC
    else if (strcmp(token, "CAR") == 0 || strcmp(token, "CDR") == 0
        || strcmp(token, "CONS") == 0) {
#ifdef INTERP
        return_var.atom = listFunc(p);
        return_var.isLisp = true;
        if (return_var.atom != NIL) {
            return_var.number = return_var.atom->i;
        }
        else {
            return_var.isNIL = true;
        }
        return return_var;
#else
        listFunc(p);
#endif
    }
    else {
        ERROR("Expect a Function name");
    }
    return return_var;
}

bool boolFunc(Prog* p) {
#ifdef INTERP
    char token[BIGSTR];
    strcpy(token, p->str[p->count]);
    p->count = p->count + 1;
    variable var1 = List(p);
    p->count = p->count + 1;
    variable var2 = List(p);
    char lispString[BIGSTR];
    lisp_tostring(var1.atom, lispString);
    if (!var1.isAssigned) {
        lisp_free(&var1.atom);
    }
    if (!var2.isAssigned) {
        lisp_free(&var2.atom);
    }
    if (strcmp(token, "LESS") == 0) {
        if (var1.number < var2.number) {
            return true;
        }
    }
    else if (strcmp(token, "GREATER") == 0) {
        if (var1.number > var2.number) {
            return true;
        }
    }
    else if (strcmp(token, "EQUAL") == 0) {
        if (var1.number == var2.number) {
            return true;
        }
    }
#else
    p->count = p->count + 1;
    List(p);
    p->count = p->count + 1;
    List(p);
#endif
    return false;
}

lisp* listFunc(Prog* p) {
    char token[BIGSTR] = { 0 };
    strcpy(token, p->str[p->count]);
    if (strcmp(token, "CONS") == 0) {
#ifdef INTERP
        return CONS(p);
#else
        p->count = p->count + 1;
        List(p);
        p->count = p->count + 1;
        List(p);
#endif
    }
    else {
#ifdef INTERP
        p->count = p->count + 1;
        variable var = List(p);
        lisp* return_lisp = NIL;
        if (strcmp(token, "CAR") == 0) {
            return_lisp = lisp_copy(lisp_car(var.atom));
        }
        else {
            return_lisp = lisp_copy(lisp_cdr(var.atom));
        }
        if (!var.isAssigned) {
            lisp_free(&var.atom);
        }
        return return_lisp;

#else
        p->count = p->count + 1;
        List(p);
#endif
    }
    return NIL;
}

int intFunc(Prog* p) {
    int number = 0;
    char token[BIGSTR] = { 0 };
    strcpy(token, p->str[p->count]);
    if (strcmp(token, "PLUS") == 0) {
#ifdef INTERP
        number = PLUS(p);
#else
        p->count = p->count + 1;
        List(p);
        p->count = p->count + 1;
        List(p);
#endif
    }
    else {
#ifdef INTERP
        p->count = p->count + 1;
        variable var = List(p);
        if (var.isLisp || var.isNIL) {
            number = lisp_length(var.atom);
        }
        else {
            ERROR("It is not a valid lisp");
        }
        if (!var.isAssigned) {
            lisp_free(&var.atom);
        }
#else
        p->count = p->count + 1;
        List(p);
#endif
    }
    return number;
}

void ioFunc(Prog* p) {
    char token[BIGSTR] = { 0 };
    strcpy(token, p->str[p->count]);
    if (strcmp(token, "SET") == 0) {
        SET(p);
    }

    else if (strcmp(token, "PRINT") == 0) {
        PRINT(p);
    }
}

void SET(Prog* p) {
    p->count = p->count + 1;
    char token[BIGSTR] = { 0 };
    strcpy(token, p->str[p->count]);
    //VAR
    if (isupper(token[0])) {
        p->count = p->count + 1;
#ifdef INTERP
        int index = p->str[p->count - 1][0] - 'A';
        char Name = p->str[p->count - 1][0];
        //Move to list
        variable var = List(p);
        if (p->variableList[index].isAssigned && p->variableList[index].isLisp) {
            lisp_free(&p->variableList[index].atom);
        }
        p->variableList[index] = set_Variable(var);
        p->variableList[index].isAssigned = true;
        p->variableList[index].name = Name;
        if (p->variableList[index].isLisp) {
            stack_push(p->lispStack, Name);
        }
#else
        List(p);
#endif
    }
    else {
        ERROR("No valid Variable here?");
    }
}

void PRINT(Prog* p) {
    //STRING
    p->count = p->count + 1;
    if (p->str[p->count][0] == '\"') {
        if (isString(p)) {
#ifdef INTERP
            printf("%s\n", p->str[p->count]);
#endif 
            return;
        }
        else {
            ERROR("Not a valid String 152\n");
        }
    }
    else {
#ifdef INTERP
        variable var = List(p);
        printList(var);
#else
        List(p);
#endif
    }
}

void Loop(Prog* p) {
    if (strcmp(p->str[p->count], "(") == 0) {
        p->count = p->count + 1;
#ifdef INTERP
        int pointer = p->count;
        bool result = boolFunc(p);
#else
        boolFunc(p);
#endif
        p->count = p->count + 1;
        if (strcmp(p->str[p->count], ")") == 0) {
#ifdef INTERP
            p->count = p->count + 1; //"("
            int insCounter = p->count;
            if (result) {//If the boolFunc return true
                while (result) {
                    p->count = insCounter;
                    INSTRCTS(p);
                    p->count = pointer;
                    result = boolFunc(p);
                }
                p->count = insCounter;
                stack* bracketStack = stack_init();
                counterJump(p, bracketStack);
                stack_free(bracketStack);
            }
            else {
                //Skip the INSTRCTS
                stack* bracketStack = stack_init();
                counterJump(p, bracketStack);
                stack_free(bracketStack);
            }
#else
            p->count = p->count + 1;
            if (strcmp(p->str[p->count], "(") == 0) {
                INSTRCTS(p);
            }
            else {
                ERROR("No Left brackets");
            }
#endif
        }
        else {
            ERROR("No right brackets");
        }
    }
    else {
        ERROR("No left brackets")
    }
}

void If(Prog* p) {
    //Skip false statement is not finished
    //"IF" "(" <BOOLFUNC> ")" "(" <INSTRCTS> "(" <INSTRCTS>
    if (strcmp(p->str[p->count], "(") == 0) {
        p->count = p->count + 1;
#ifdef INTERP
        bool result = boolFunc(p);
#else
        boolFunc(p);
#endif
        p->count = p->count + 1;
        if (strcmp(p->str[p->count], ")") == 0) {
            p->count = p->count + 1;
            //INSTRCTS
            if (strcmp(p->str[p->count], "(") == 0) {
#ifdef INTERP
                stack* bracketStack = stack_init();
                if (result) {
                    INSTRCTS(p);
                    //Skip the false statement
                    p->count = p->count + 1;
                    counterJump(p, bracketStack);
                    stack_free(bracketStack);
                    return;
                }
                else {
                    //Skip the true statement
                    counterJump(p, bracketStack);
                    stack_free(bracketStack);
                }
#else
                INSTRCTS(p);
#endif
                p->count = p->count + 1;
                if (strcmp(p->str[p->count], "(") == 0) {
#ifdef INTERP
                    INSTRCTS(p);
#else
                    INSTRCTS(p);
#endif
                }
                else {
                    ERROR("No false statement")
                }
            }
            else {
                ERROR("No true statement")
            }
        }
        else {
            ERROR("No right brackets ) ?");
        }
    }
    else {
        ERROR("No left brackets ( ?");
    }
}

variable List(Prog* p) {
    //LIST
    variable return_var = { .atom = NIL,.isLisp = false,.isNIL = false,.isNumber = false,.name = '\0',
                            .number = 0,.isAssigned = false,.boolResult = false };
    //<VAR>
    if (isupper(p->str[p->count][0]) && p->str[p->count][1] == '\0') {
        //Check if the variable is initialized
#ifdef INTERP
        int index = p->str[p->count][0] - 'A';
        if (p->variableList[index].isAssigned == false) {
            ERROR("The Variable is not assigned");
        }
        else {
            return_var = p->variableList[index];
        }
#endif
    }
    //NIL
    else if (strcmp(p->str[p->count], "NIL") == 0) {
        return_var.isNIL = true;
    }
    //RETFUNC
    else if (strcmp(p->str[p->count], "(") == 0) {
        p->count = p->count + 1;
        return_var = retFunc(p);
        p->count = p->count + 1;
#ifdef INTERP
#else
        if (strcmp(p->str[p->count], ")") != 0) {
            ERROR("No right brackets )?");
        }
#endif
    }
    //LITERAL
    else if (p->str[p->count][0] == '\'') {
#ifdef INTERP
        return_var = isLiteral(p);
#else
        int length = strlen(p->str[p->count]);
        if (p->str[p->count][length - 1] != '\'') {
            ERROR("Not a valid Literal");
        }
#endif
    }
    else {
        ERROR("No Valid list");
    }
    return return_var;
}

variable isLiteral(Prog* p) {
    variable return_var = { .atom = NULL,.isLisp = false,.isNIL = false,.isNumber = false,.name = '\0',
                            .number = 0 };
    int length = strlen(p->str[p->count]);
    //If the literal is ''
    if (length == 2 || p->str[p->count][length - 1] != '\'') {
        ERROR("Not a valid literal");
    }
    //Remove the signel quote
    char str[BIGSTR] = { 0 };
    for (int i = 1; i < length - 1; i++) {
        str[i - 1] = p->str[p->count][i];
    }
    //Check if it is a number
    if (str[0] != '(') {
        int number = 0;
        int length = strlen(str);
        for (int i = 0; i <= length - 1; i++) {
            if (!isdigit(str[i])) {
                if (str[i] != '-') {
                    ERROR("There should be only number 0-9");
                }
            }
        }
        sscanf(str, "%d", &number);
        return_var.isNumber = true;
        return_var.number = number;
    }
    //check if it is a vaild literal
    else {
#ifdef INTERP
        return_var.atom = lisp_fromstring(str);
        char lispString[BIGSTR];
        lisp_tostring(return_var.atom, lispString);
        if (strcmp(lispString, str) != 0) {
            ERROR("Not a valid Literal");
        }
        return_var.isLisp = true;
#endif
    }
    return return_var;
}

bool isString(Prog* p) {
    int length = strlen(p->str[p->count]);
    if (p->str[p->count][0] == '\"' && p->str[p->count][length - 1] == '\"') {
        return true;
    }
    return false;
}

lisp* CONS(Prog* p) {
#ifdef INTERP
    p->count = p->count + 1;
    variable var1 = List(p);
    p->count = p->count + 1;
    variable var2 = List(p);
    lisp* lispOne = variable_lisp(var1);
    lisp* lispTwo;
    if (var2.isNumber) {
        ERROR("It should be a Lisp,Not a atom");
    }
    else {
        lispTwo = variable_lisp(var2);
    }
    return lisp_cons(lispOne, lispTwo);;
#else
    p->tokenIndex++;
    return NIL;
#endif
}

int PLUS(Prog* p) {
#ifdef INTERP
    p->count = p->count + 1;
    variable var1 = List(p);
    int number1 = var1.number;
    p->count = p->count + 1;
    variable var2 = List(p);
    int number2 = var2.number;
    int result = number1 + number2;
    if (var1.isAssigned) {
        //PLUS A '1': A = A + 1
        int index = var1.name - 65;
        p->variableList[index].number = result;
    }
    else if (var1.isLisp) {
        lisp_free(&var1.atom);
    }
    if (!var2.isAssigned && var2.isLisp) {
        lisp_free(&var2.atom);
    }
    return result;
#else
    return p->tokenIndex;
#endif
}

lisp* variable_lisp(variable var) {
    if (var.isNumber) {
#ifdef INTERP
        return lisp_atom(var.number);
#endif
    }
    else if (var.isLisp) {
#ifdef INTERP
        if (var.isAssigned) {
            return lisp_copy(var.atom);
        }
        else {
            return var.atom;
        }
#endif
    }
    return NIL;
}

void counterJump(Prog* p, stack* bracketStack) {
#ifdef INTERP
    int index = p->count;
    stack_push(bracketStack, p->str[p->count][0]);
    index = index + 1;
    while (stack_size(bracketStack) != 0) {
        stacktype atom = p->str[index][0];
        if (atom == '(') {

            stack_push(bracketStack, atom);
        }
        else if (atom == ')') {
            stack_peek(bracketStack, &atom);
            if (atom == '(') {
                stack_pop(bracketStack, &atom);
            }
            else {
                ERROR("Expected a bracket");
            }
        }
        if (stack_size(bracketStack) != 0) {
            index++;
        }
    }
    p->count = index;
#else
    p->tokenIndex = 0;
    bracketStack = NULL;
#endif
}

void bracketCheck(Prog* p) {
    stack* bracketStack = stack_init();
    int index = p->count;
    stack_push(bracketStack, p->str[index][0]);
    index++;
    while (p->str[index][0] != '\0') {
        stacktype bracket = p->str[index][0];
        if (p->str[index][0] != '\'') {
            if (bracket == '(') {
                stack_push(bracketStack, bracket);
            }
            else if (bracket == ')') {
                stack_peek(bracketStack, &bracket);
                if (bracket == '(') {
                    stack_pop(bracketStack, &bracket);
                }
                else {
                    ERROR("Expected a brackets")
                }
            }
        }
        index++;
    }
    int size = stack_size(bracketStack);
    if (size != 0) {
        printf("Size:%d", size);
        ERROR("No enough brackets");
    }
    stack_free(bracketStack);
}

void printList(variable var) {
#ifdef INTERP
    if (var.isNumber) {
        char str[BIGSTR];
        sprintf(str, "%d", var.number);
        puts(str);
    }
    else if (var.isLisp) {
        char str[BIGSTR];
        lisp_tostring(var.atom, str);
        printf("%s\n", str);
        var.isLisp = false;
        if (!var.isAssigned) {
            lisp_free(&var.atom);
        }
    }
    else if (var.isNIL) {
        printf(NIL);
    }
    else if (var.isBoolResult) {
        printf("%d\n", var.number);
    }
#else
    var.number = 0;
#endif
}

variable set_Variable(variable var) {
    variable return_var = { .name = '\0',.isNIL = false,.isLisp = false,.isNumber = false,.number = 0,
                            .isAssigned = false,.isBoolResult = false,.boolResult = false,.atom = NIL };
    if (var.isLisp) {
#ifdef INTERP
        if (var.isAssigned) {
            return_var.atom = lisp_copy(var.atom);
        }
        else {
            return_var.atom = var.atom;
        }
#endif
        return_var.isLisp = var.isLisp;
        return_var.number = var.number;
        return_var.isNumber = var.isNumber;
    }
    else {
        return_var = var;
    }

    return return_var;
}
