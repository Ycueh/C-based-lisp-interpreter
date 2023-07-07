#pragma once

#define LISPIMPL "Linked"

struct lisp {
   //Underlying array
   //Car contains the element
   //Crd points to next lisp
   lisp* car;
   atomtype i;
   lisp* cdr;
};
