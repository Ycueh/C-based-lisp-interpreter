#pragma once

#include "general.h"
#include "lisp.h"
#include "specific.h"
#include "stack.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#define fileSize 2000
#define MAXNUMLINE 1000
#define LINESIZE 200
#define TOKENSIZE 50
#define BIGSTR 200
#define ALPHABET 26
#define NIL NULL
#define SKIPTRUE 1
#define SKIPFALSE 2
#define BRACKETCHECK 0
#define COUNTERJUMP 1
#define TIMESNUMBER 200
#define ERROR(PHRASE){fprintf(stderr,\
                      "%s",PHRASE);\
                      exit(EXIT_FAILURE);}


struct var{
    char name;
    bool isNIL;
    bool isLisp;
    bool isNumber;
    bool isAssigned;
    bool isBoolResult;
    bool boolResult;
    int  number;
    lisp* atom;
};
typedef struct var variable;
struct ifPosition{
    int trueEnd;
    int falseEnd;
};
typedef struct ifPosition ifCounter;
struct program{
    char str[MAXNUMLINE][LINESIZE];
    int count;
    int tokenIndex;
    
    variable variableList[ALPHABET];
    stack* lispStack;
};
typedef struct program Prog;


void readFile(char* filePath,Prog *p);

void splitLine(Prog *p,char* rowString,int charIndex,char* token);

void PROG(Prog *p);

int PLUS(Prog *p);

void INSTRCTS(Prog *p);

void INSTRCT(Prog *p);

void Func(Prog *p);

void ioFunc(Prog *p);

void SET(Prog *p);

void PRINT(Prog *p);

variable retFunc(Prog *p);

bool boolFunc(Prog *p);

lisp* listFunc(Prog *p);

int intFunc(Prog *p);

void If(Prog *p);

void Loop(Prog *p);

variable List(Prog *p);

variable isLiteral(Prog *p);

bool isString(Prog *p);

void atom_free(Prog *p);

lisp* CONS(Prog *p);

lisp* variable_lisp(variable var);

void counterJump(Prog *p,stack *bracketStack);

void bracketCheck(Prog *p);

void printList(variable var);

variable set_Variable(variable var);
