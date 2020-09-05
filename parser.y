%{

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%destructor { destroy ($$); } <>
%printer { astree::dump (yyoutput, $$); } <>

%initial-action {
   parser::root = new astree (TOK_ROOT, {0, 0, 0}, "");
}

%token TOK_VOID TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULLPTR TOK_ARRAY TOK_ARROW TOK_ALLOC TOK_PTR
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE TOK_NOT
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON
%token TOK_ROOT TOK_BLOCK TOK_CALL

%token TOK_FUNCTION TOK_PARAM TOK_PROTOTYPE TOK_TYPEID
%token TOK_VARDECL TOK_FIELD TOK_INDEX

%token '(' ')' '[' ']' '{' '}' ';'
%token '=' '+' '-' '*' '/' '%' '!'

%right TOK_IF TOK_ELSE
%right '='
%left  TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left  '+' '-'
%left  '*' '/' '%'
%right TOK_POS TOK_NEG TOK_NOT
%left  '[' TOK_ARROW TOK_FUNCTION TOK_ALLOC

%start  start

%%

start   : program       { $$ = $1 = nullptr; }
        ;

program : program structdef { $$ = $1->adopt($2); }
        | program function  { $$ = $1->adopt($2); }
        | program statement { $$ = $1->adopt($2); }  
        |                   { $$ = parser::root;  }
        | program error '}' 
          {
            destroy($3); 
            $$ = $1;   
          }  
        | program error ';'
          { 
            destroy($3);
            $$ = $1;
          }  
        ;

structdef : TOK_STRUCT TOK_IDENT structdecl '}' ';'
            {
              destroy($4, $5); 
              $2->zap_sym(TOK_TYPEID);
              $$ = $1->adopt($2, $3);
            }
          | TOK_STRUCT TOK_IDENT '{' '}' ';'
            {
                destroy($3, $4);
                destroy($5);
                $2->zap_sym(TOK_TYPEID);
                $$ = $1->adopt($2); 
            }
          ;

structdecl : '{' identdecl ';'
              {
                destroy($3); 
                $$ = $1->adopt($2); 
              }
            | structdecl identdecl';'
              {
                destroy($3); 
                $$ = $1->adopt($2);
              }
            ;

identdecl : type TOK_IDENT 
            { 
              $$ = $1->adopt($2); 
            }
          ;

type : plaintype { $$ = $1; }
     | newarray  { $$ = $1; }
     ; 

newarray : TOK_ARRAY TOK_LT plaintype TOK_GT
           {
             destroy($2, $4);
             $$ = $1->adopt($3);
           }
         ;

plaintype : TOK_VOID   { $$ = $1; }
          | TOK_INT    { $$ = $1; }
          | TOK_STRING { $$ = $1; }
          | TOK_PTR TOK_LT TOK_STRUCT TOK_IDENT TOK_GT 
            {
              destroy($2, $3);
              destroy($5);
              $$ = $1->adopt($4);
            }
          ;

function : identdecl func_rec ')' block
           {
             destroy($3);
             $$ = new astree(TOK_FUNCTION, $1->lloc, "");
             $$ = $$->adopt($1, $2);
             $$ = $$->adopt($4);
           }
         | identdecl '(' ')' block
           {
             destroy($3);
             $2->zap_sym(TOK_PARAM);
             $$ = new astree(TOK_FUNCTION, $1->lloc, "");
             $$ = $$->adopt($1, $2);
             $$ = $$->adopt($4);
           }
         | identdecl func_rec ')' ';'
           {
             destroy($3, $4);
             $$ = new astree(TOK_PROTOTYPE, $1->lloc, "");
             $$ = $$->adopt($1, $2);
           }
         | identdecl '(' ')' ';'
           {
             destroy($3, $4);
             $2->zap_sym(TOK_PARAM);
             $$ = new astree(TOK_PROTOTYPE, $1->lloc, "");
             $$ = $$->adopt($1, $2);
           }
         
 
         ;

func_rec : '(' identdecl 
           { 
             $1->zap_sym(TOK_PARAM);
             $$ = $1->adopt($2);
           }
         | func_rec ',' identdecl 
           {
             destroy($2);
             $$ = $1->adopt($3);
           }
         ;

block : block_rec '}'
        {
          destroy($2);
          $$ = $1;
        }
      | '{' '}'
        {
          destroy($2);
          $1->zap_sym(TOK_BLOCK);
          $$ = $1;
        }
      ;

block_rec : '{' statement
            {
              $1->zap_sym(TOK_BLOCK);
              $$ = $1->adopt($2);
            }
          | block_rec statement { $$ = $1->adopt($2); }
          ;

statement : vardecl     { $$ = $1; }
          | block       { $$ = $1; }
          | while       { $$ = $1; }
          | ifelse      { $$ = $1; }
          | return      { $$ = $1; }
          | ';'         { $$ = $1; }
          | expr ';'    
            {
              destroy($2);
              $$ = $1; 
            }
          ;

vardecl : identdecl '=' expr ';' 
          {
            destroy($4);
            $2->zap_sym(TOK_VARDECL);
            $$ = $2->adopt($1, $3);
          }
        | identdecl ';'
          {
            $2->zap_sym(TOK_VARDECL);
            $$ = $1->adopt($2);
          }
        ;    

while : TOK_WHILE '(' expr ')' statement
        {
          destroy($2, $4);
          $$ = $1->adopt($3, $5);
        }
      ;

ifelse : TOK_IF '(' expr ')' statement TOK_ELSE statement
         {
           destroy($2, $4);
           destroy($6);
           $$ = $1->adopt($3, $5);
           $$ = $1->adopt($7);
         }
       | TOK_IF '(' expr ')' statement %prec TOK_IF
         {
           destroy($2, $4);
           $$ = $1->adopt($3, $5);
         }
       ;

return : TOK_RETURN expr ';'
         {
           destroy($3);
           $$ = $1->adopt($2);
         }
       | TOK_RETURN ';'
         {
           destroy($2);
           $$ = $1;
         }
       ;

expr : binop        { $$ = $1;                  }
     | unop         { $$ = $1;                  }
     | allocator    { $$ = $1;                  }
     | call         { $$ = $1;                  }
     | '(' expr ')' { destroy($1, $3); $$ = $2; }
     | variable     { $$ = $1;                  }
     | constant     { $$ = $1;                  }
     ;

binop : expr TOK_EQ expr       { $$ = $2->adopt($1, $3); }
      | expr TOK_NE expr       { $$ = $2->adopt($1, $3); }
      | expr TOK_LT expr       { $$ = $2->adopt($1, $3); }
      | expr TOK_LE expr       { $$ = $2->adopt($1, $3); }
      | expr TOK_GT expr       { $$ = $2->adopt($1, $3); }
      | expr TOK_GE expr       { $$ = $2->adopt($1, $3); }
      | expr '=' expr          { $$ = $2->adopt($1, $3); }
      | expr '+' expr          { $$ = $2->adopt($1, $3); }
      | expr '-' expr          { $$ = $2->adopt($1, $3); }
      | expr '*' expr          { $$ = $2->adopt($1, $3); }
      | expr '/' expr          { $$ = $2->adopt($1, $3); }
      | expr '%' expr          { $$ = $2->adopt($1, $3); }
      ;

unop : '+' expr %prec TOK_POS { $$ = $1->adopt_sym($2, TOK_POS); }
     | '-' expr %prec TOK_NEG { $$ = $1->adopt_sym($2, TOK_NEG); }
     | TOK_NOT expr           { $$ = $1->adopt($2);              }
     ;

allocator : TOK_ALLOC TOK_LT TOK_STRING TOK_GT '(' expr ')'
            {
              destroy($2, $4);
              destroy($5, $7);
              $$ = $1->adopt($3, $6);
            }
          | TOK_ALLOC TOK_LT TOK_STRUCT TOK_IDENT TOK_GT '(' ')'
            {
              destroy($2, $3);
              destroy($5, $6);
              destroy($7);
              $$ = $1->adopt($4);
            }
          | TOK_ALLOC TOK_LT newarray TOK_GT '(' expr ')'
            {
              destroy($2, $4);
              destroy($5, $7);
              $$ = $1->adopt($3, $6);
            }
          ;

call : call_rec ')'
       {
         destroy($2);
         $$ = $1;
       }
     | TOK_IDENT '(' ')'
       {
         destroy($3);
         $2->zap_sym(TOK_CALL);
         $$ = $2->adopt($1);
       }
     ;

call_rec : TOK_IDENT '(' expr
           {
             $2->zap_sym(TOK_CALL);
             $$ = $2->adopt($1, $3);
           }
         | call_rec ',' expr
           {
             destroy($2);
             $$ = $1->adopt($3);
           }
         ;

variable : TOK_IDENT { $$ = $1; } 
         | expr '[' expr ']' %prec TOK_INDEX
           {
             destroy($4);
             $2->zap_sym(TOK_INDEX);
             $$ = $2->adopt($1, $3);
           }
         | expr TOK_ARROW TOK_IDENT %prec TOK_ARROW
           {
             $3->zap_sym(TOK_FIELD);
             $$ = $2->adopt($1, $3);
           }
         ;

constant : TOK_INTCON    { $$ = $1; }
         | TOK_CHARCON   { $$ = $1; }
         | TOK_STRINGCON { $$ = $1; }
         | TOK_NULLPTR   { $$ = $1; }
         ;

%%

const char* parser::get_tname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}
