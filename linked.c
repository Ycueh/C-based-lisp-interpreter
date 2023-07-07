#include "lisp.h"
#include "specific.h"

#define LISTSTRLEN 1000
#define BIGSTR 10000
void test(void);
void tostring(const lisp* l, char* str);
lisp *read_lisp_str(const char* str);
void find_bracket(char *str, char *content);
void strn_cut(char *str, int num);
void remove_space(char *str);
int get_first_num(char *str,int *dst);
// Returns element 'a' - this is not a list, and
// by itelf would be printed as e.g. "3", and not "(3)"
lisp* lisp_atom(const atomtype a) {
    lisp* result_lisp = (lisp*)ncalloc(1, sizeof(lisp));
    result_lisp->i = a;
    result_lisp->car = NULL;
    result_lisp->cdr = NULL;
    return result_lisp;
}

// Returns a list containing the car as 'l1'
// and the cdr as 'l2'- data in 'l1' and 'l2' are reused,
// and not copied. Either 'l1' and/or 'l2' can be NULL
lisp* lisp_cons(const lisp* l1, const lisp* l2) {
    lisp* result_lisp = (lisp*)ncalloc(1, sizeof(lisp));
    result_lisp->car = (lisp*)l1;
    if (!l2) {
        result_lisp->cdr = NULL;
    }
    else {
        result_lisp->cdr = (lisp*)l2;
    }
    return result_lisp;
}

// Returns the car (1st) component of the list 'l'.
// Does not copy any data.
lisp* lisp_car(const lisp* l) {
    lisp* car;
    car = l->car;
    return car;
}

// Returns the cdr (all but the 1st) component of the list 'l'.
// Does not copy any data.
lisp* lisp_cdr(const lisp* l) {
    lisp* cdr;
    cdr = l->cdr;
    return cdr;
}

// Returns the data/value stored in the cons 'l'
atomtype lisp_getval(const lisp* l) {
    atomtype value = 0;
    if (lisp_isatomic(l)) {
        value = l->i;
    }
    else {
        value = l->car->i;
    }
    return value;
}

// Returns a boolean depending up whether l points tdo an atom (not a list)
bool lisp_isatomic(const lisp* l) {
    if (l) {
        if (!l->car && !l->cdr) {
            return true;
        }
    }
    return false;
}

// Returns a deep copy of the list 'l'
lisp* lisp_copy(const lisp* l) {
    lisp* copy_lisp;
    //Copy car
    if(!l){
        return NIL;
    }
    if (lisp_isatomic(l)) {
        copy_lisp = lisp_atom(l->i);
    }
    else {
        lisp* new_lisp = (lisp*)ncalloc(1, sizeof(lisp));
        new_lisp->car = lisp_copy(l->car);
        copy_lisp = new_lisp;
    }
    //Copy Cdr
    if (l->cdr) {
        copy_lisp->cdr = lisp_copy(l->cdr);
    }
    return copy_lisp;
}
// Returns number of components in the list.
int lisp_length(const lisp* l) {
    int length = 0;
    if (!l || lisp_isatomic(l)) {
        return 0;
    }
    while (l) {
        length++;
        l = l->cdr;
    }
    return length;
}

// Returns stringified version of list
void lisp_tostring(const lisp* l, char* str) {
    if (!l) {
        str[0] = '(';
        str[1] = ')';
        str[2] = '\0';
    }
    else {
        if (lisp_isatomic(l)) {
            str += sprintf(str, "%d", l->i);
        }
        else {
            str += sprintf(str, "(");
            tostring(l, str);
        }
    }

}

void tostring(const lisp* l, char* str) {
    if (lisp_isatomic(l->car)) {
        char number[BIGSTR];
        sprintf(number, "%d", l->car->i);
        strcat(str, number);
    }
    else {
        strcat(str, "(");
        tostring(l->car, str);
    }
    if (l->cdr) {
        strcat(str, " ");
        tostring(l->cdr, str);
    }
    else {
        strcat(str, ")");
    }
}

// Clears up all space used
// Double pointer allows function to set 'l' to NULL on success
void lisp_free(lisp** l) {
    if (*l) {
        lisp* car = (*l)->car;
        lisp* cdr = (*l)->cdr;
        free(*l);
        *l = NULL;
        lisp_free(&car);
        if (cdr) {
            lisp_free(&cdr);
        }
    }
}

/* ------------- Tougher Ones : Extensions ---------------*/
// Reference: DEXIN WANG email:ys22815@bristol.ac.uk
lisp* lisp_fromstring(const char*str){
    char str_copy[LISTSTRLEN];
    strcpy(str_copy, str);
    if (str_copy[0] == '('){
    strn_cut(str_copy, 1);
    }
    if (str_copy[strlen(str_copy)-1]== ')'){
        str_copy[strlen(str_copy)-1] = '\0';
    }
    return read_lisp_str(str_copy);
}

lisp *read_lisp_str(const char* str){
    char str_copy[LISTSTRLEN];
    strcpy(str_copy, str);
    if (str_copy[0] == ')' || strlen(str) == 0){
        return NULL;
    }
    lisp *car = NULL;
    if (str_copy[0] == '('){
        char content[LISTSTRLEN];
        find_bracket(str_copy, content);
        car = read_lisp_str(content);
    }
    else {
        atomtype a;
        if (get_first_num(str_copy, &a))
        {
            car = lisp_atom(a);
        }else{
            return lisp_cons(car,NIL);
        }
    }
    return lisp_cons(car, read_lisp_str(str_copy));
}

void find_bracket(char *str, char *content){
    char temp[BIGSTR];
    strcpy(temp, str);char *str1 = temp;
    strcpy(content, str);
    int left = 0;
    int right = 0;
    int i = 0;
    bool in_str = 0;
    while((left != right||left == 0)&& *str1 != '\0'){
        if(!in_str){
            switch(*str1){
                case '(':left++; break;
                case ')':right++; break;
                case '\"':in_str =1;
            }
        }else if(*str1 == '\"'){
            in_str = 0;
        }
        str1++;
        i++;
    }
    content[i - 1] = '\0';
    strn_cut(content,1);
    strn_cut(str,i);
    remove_space(str);
}

void strn_cut(char *str, int num){
    int str_len = strlen(str);
    if(str_len<=num){
        *str = '\0';
        return;
    }
    char temp[BIGSTR];
    strcpy(temp,str+num);
    strcpy(str,temp);
}
int get_first_num(char *str,int *dst){
    if(sscanf(str,"%d",dst)!=0){
        char temp[BIGSTR];
        sprintf(temp,"%d",*dst);
        remove_space(str);
        strn_cut(str,strlen(temp));
        remove_space(str);
        return 1;
    }
    return 0;
}
void remove_space(char *str){
    while(str[0]==' '){
        strn_cut(str,1);
    };
}
// Returns a new list from a set of existing lists.
// A variable number 'n' lists are used.
// Data in existing lists are reused, and not copied.
// You need to understand 'varargs' for this.
lisp* lisp_list(const int n, ...);

// Allow a user defined function 'func' to be applied to
// each component of the list 'l'.
// The user-defined 'func' is passed a pointer to a cons,
// and will maintain an accumulator of the result.
void lisp_reduce(void (*func)(lisp* l, atomtype* n), lisp* l, atomtype* acc);


