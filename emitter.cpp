#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "astree.h"
#include "emitter.h"
#include "auxlib.h"
#include "lyutils.h"
extern FILE* oil_file;

using namespace std;

void emit (astree* root);

int sn = 0;
int tn = 0;
int whn = 0;
int ifn = 0;
int loc_flag = 0;
string header = "";

//returns a node's lexinfo as a string
string get_str(astree* tree) {
   return tree->lexinfo->c_str();
}

//returns an identifier string
string get_ident(astree* tree) {
   if(tree->symbol == TOK_ARROW)
      return get_str(tree->children.at(0)) + "->" + 
             get_str(tree->children.at(1));

   else
      return get_str(tree);
}

//Prints all code in the correct place
void emit_insn (const char* opcode, const char* operand) {
   fprintf (oil_file, "%-10s%s %s\n", header.c_str(), opcode, operand);
   header = "";
}

//Prints the start of emitter code
void emit_start (const char* opcode){
   fprintf(oil_file, "%s:", opcode);
}

//Posorder search algorithm provided by Wesley Mackey
void postorder (astree* tree) {
   assert (tree != nullptr);
   for (size_t child = 0; child < tree->children.size(); ++child) {
      emit (tree->children.at(child));
   }
}

//default stmnt parser
void postorder_emit_stmts (astree* tree) {
   postorder (tree);
}

//Default block parser
void postorder_emit_block (astree* tree) {
   postorder(tree); 
}

//Handles function calls
void postorder_emit_call (astree* tree) {
   assert (tree != nullptr);
   string func_name = get_str(tree->children.at(0));
   string start =  "call " + func_name + " (";
   for(size_t child = 1; child < tree->children.size(); ++child) {
      string arg = get_ident(tree->children.at(child));
      if(child == tree->children.size() - 1)
         start.append(arg + ")");
      else
         start.append(arg + ", ");
   }

   emit_insn(start.c_str(), "");
}

//Handles all comparison fucntions
void postorder_emit_compare(astree* tree) {
   if(tree->symbol == TOK_EQ || tree->symbol == TOK_NE)
      return;
   else if(tree->symbol == TOK_NOT)
      return;
   string temp_reg = "$t" + to_string(tn) + ":i"; 
   string cmp =
   get_str(tree->children.at(0)) + " " +
   get_str(tree) + " " + get_str(tree->children.at(1));
   string line = temp_reg + " = " + cmp;
   emit_insn(line.c_str(), "");
}

//Handles functions
void postorder_emit_func (astree* tree) {
   assert (tree != nullptr);
   loc_flag = 1;
   string func_ident = get_str(tree->children.at(0)->children.at(0));
   string func_type = get_str(tree->children.at(0));
   if(strcmp(func_type.c_str(), "void") == 0)
      func_type = "";

   func_ident.append(":");
   header = func_ident;
   emit_insn(".function", func_type.c_str());
   for(size_t child = 1; child < tree->children.size(); ++child) {
      emit (tree->children.at(child));
   }
 
   emit_insn("return", "");
   emit_insn(".end", "");
   loc_flag = 0;
}

//Handles if statments
void postorder_emit_if(astree* tree){
   string num = to_string(ifn);
   header = ".if" + num + ":";
   emit(tree->children.at(0));
   string go;
   if(tree->children.size() == 2){
      go = "goto .fi" + num + " if ";
      if(tree->children.at(0)->symbol == TOK_EQ)
         go += get_ident(tree->children.at(0)->children.at(0)) + 
              " != " + get_ident(tree->children.at(0)->children.at(1));

      else if(tree->children.at(0)->symbol == TOK_NE)
         go += get_ident(tree->children.at(0)->children.at(0)) + 
              " == " + get_ident(tree->children.at(0)->children.at(1));

      else if(tree->children.at(0)->symbol == TOK_NOT)
         go += get_str(tree->children.at(0)->children.at(0));

      else{
         go += "not $t" + to_string(tn) +":i";
         tn ++;
      }

      emit_insn(go.c_str(), "");

      header = ".th" + num + ":";
      emit(tree->children.at(1));
      header = ".fi" + num + ":";
   }
   else{
      go = "goto .el" + num + " if ";
      if(tree->children.at(0)->symbol == TOK_EQ)
         go += get_ident(tree->children.at(0)->children.at(0)) + 
              " != " + get_ident(tree->children.at(0)->children.at(1));

      else if(tree->children.at(0)->symbol == TOK_NE)
         go += get_ident(tree->children.at(0)->children.at(0)) + 
              " == " + get_ident(tree->children.at(0)->children.at(1));

      else if(tree->children.at(0)->symbol == TOK_NOT)
         go += get_str(tree->children.at(0)->children.at(0));

      else{
         go += "not $t" + to_string(tn) +":i";
         tn ++;
      }
      header = ".th" + num + ":";
      emit(tree->children.at(1));
      emit_insn(("goto .fi" + num).c_str(), "");
      header = ".el" + num + ":";
      emit(tree->children.at(2));
   }
   ++ifn;
}

//Handles and binary and unary operations
string postorder_emit_oper(astree* tree) {
   assert(tree->children.size() == 2);
   string lstr;
   string rstr;
   int r_check = 0;
   int l_check = 0;
   int rtn = 0;
   int ltn = 0;
   string temp_reg;
   string op = get_str(tree);
   astree* left = tree->children.at(0);
   astree* right = tree->children.at(1);
   if(left->children.size() == 2 && left->symbol != TOK_ARROW){
      ++l_check;
      lstr = postorder_emit_oper(left);
      ltn = tn;
   }
   else
      lstr = get_str(left);

   if(right->children.size() == 2 && right->symbol != TOK_ARROW){
      ++r_check;
      rstr = postorder_emit_oper(right);
      rtn = tn;
   }
   else
      rstr = get_ident(right);
   
   ++tn;
   if(r_check == 0 && l_check == 0){
      temp_reg = 
      "$t" + to_string(tn-1) + ":i = " + lstr + " " + op + " " + rstr;
      emit_insn(temp_reg.c_str(), "");
      return "$t" + to_string(tn-1) + ":i";
   }
   else if(r_check == 1 && l_check == 0){
      temp_reg = "$t" + to_string(tn-1) + ":i = " + lstr +
                 " " + op + " $t" + to_string(rtn-1) + ":i"; 
      
      emit_insn(temp_reg.c_str(), "");
      return "$t" + to_string(tn-1) + ":i";
   }

   else if(r_check == 0 && l_check == 1){
      temp_reg = "$t" + to_string(tn-1) + ":i = $t" +
                 to_string(ltn-1) + ":i " + op + " " + rstr;

      emit_insn(temp_reg.c_str(), "");
      return " $t" + to_string(tn-1) + ":i";
   } 

   temp_reg = "$t" + to_string(tn-1) + ":i = $t" + to_string(ltn-1) +
               ":i " + op + " $t" + to_string(rtn-1) + ":i"; 

   emit_insn(temp_reg.c_str(), "");
   return " $t" + to_string(tn-1) + ":i";
}

//Handles parameters
void postorder_emit_param (astree* tree) {
   assert (tree != nullptr);
   for (size_t child = 0; child < tree->children.size(); ++child) {
      string param_type = get_str(tree->children.at(child));
      string param_ident = 
      get_str(tree->children.at(child)->children.at(0));
      string temp = param_type + " " + param_ident;
      emit_insn(".param", temp.c_str());
   }
}

//Handles ptr
void postorder_emit_ptr (astree* tree) {
   assert(tree != nullptr);
   string ident = "ptr" + get_str(tree->children.at(1));
   if (loc_flag == 1)
      emit_insn(".local", ident.c_str());
   else
      emit_insn(".global", ident.c_str());
}

//Handles returns
void postorder_emit_return(astree* tree) {
   string ret_val = get_str(tree->children.at(0));
   emit_insn("return", ret_val.c_str());
}

//Handles extra semicolons
void postorder_emit_semi (astree* tree) {
   postorder (tree);
   return;
}

//Handles structs
void postorder_emit_struct(astree* tree) {
   emit_insn(".struct", tree->children.at(0)->lexinfo->c_str());
   if(tree->children.size() == 2){
      astree* block = tree->children.at(1);
      for (size_t child = 0; child < block->children.size(); ++child) {
         string field_type = get_str(block->children.at(child));
         string field_ident =
         get_str(block->children.at(child)->children.at(0));
      
         string field = field_type + " " + field_ident;
         emit_insn(".field", field.c_str());
      }
   }
   emit_insn(".end", "");
}

//Default statement for variables
void postorder_emit_var (astree* tree, const char* opcode) {
   postorder(tree);
   emit_insn (opcode, "");
}

//Handles while statements
void postorder_emit_while (astree* tree) {
   string num = to_string(whn);
   string while_head = ".wh" + num + ":";
   header = while_head;
   emit(tree->children.at(0));
   string go = "goto .od" + num + " if ";
   if(tree->children.at(0)->symbol == TOK_EQ)
      go += get_ident(tree->children.at(0)->children.at(0)) + 
            " != " + get_ident(tree->children.at(0)->children.at(1));

   else if(tree->children.at(0)->symbol == TOK_NE)
       go += get_ident(tree->children.at(0)->children.at(0)) + 
            " == " + get_ident(tree->children.at(0)->children.at(1));

   else if(tree->children.at(0)->symbol == TOK_NOT)
       go += get_str(tree->children.at(0)->children.at(0));

   else{
      go += "not $t" + to_string(tn) +":i";
      tn ++;
   }

   emit_insn(go.c_str(), "");
   header = ".do" + num +":";
   emit(tree->children.at(1));
   emit_insn(("goto .wh" + num).c_str(), "");
   header = ".od" + num + ":";
   ++whn;
}

//Default statement for accepted, but not handled tokens
void emit_push (astree* tree, const char* opcode) {
   emit_insn (opcode, tree->lexinfo->c_str());
}

//Handles variable initialization
void emit_assign (astree* tree) {
   assert (tree->children.size() == 2);
   astree* left = tree->children.at(0);
   astree* right = tree->children.at(1);
   if (left->symbol != TOK_IDENT && left->symbol != TOK_ARROW) {
      ;;//do nothing
   }
   else {
      string ident = get_str(left);
      if(left->children.size() != 0){
         string var = get_str(left->children.at(0));
         string field = get_str(left->children.at(1));
         ident = var + ident + field;  
      }
      ident.append(" =");
      if(right->children.size() != 0){
         string expr = postorder_emit_oper(right);
         emit_insn(ident.c_str(), expr.c_str());
      }
      else {
         string val = get_str(right);
         emit_insn(ident.c_str(), val.c_str());
      }
   }
}

//Handles variable declatation
void emit_vardecl (astree* tree) {
   assert(tree->children.size() == 2);
   astree* left = tree->children.at(0);
   astree* right = tree->children.at(1);
   string type = get_str(left);
   string ident = get_str(left->children.at(0));
   if(strcmp(type.c_str(), "ptr") == 0)
      ident = get_str(left->children.at(1));
   string decl = type + " " + ident;
   if(loc_flag == 1)
      emit_insn(".local", decl.c_str());
   else{
      if(left->symbol == TOK_STRING){
         header = ".s" + to_string(sn) + ":";
         emit_insn(get_str(right).c_str(), "");
         ++sn;
      }
      else
         header = ident + ":";
   }
      
   if(strcmp(type.c_str(), "ptr") == 0) {
      string struct_type = get_str(right->children.at(0));
      string expr = "malloc " + struct_type;
      ident.append(" =");
      emit_insn(ident.c_str(), expr.c_str());
   }
   else if(right->children.size() != 0) {
      ident.append(" =");
      string expr = postorder_emit_oper(right);
      emit_insn(ident.c_str(), expr.c_str());
   }
   else {
      string val = get_str(right);
      if(loc_flag == 0){
         header = ident + ":";
         string end = type + " " + val;
         emit_insn(".global", end.c_str());
      }
      else{
         ident.append(" =");
         emit_insn(ident.c_str(), val.c_str());
      }
   }
}

//Formatted switch statement
void emit (astree* tree) {
   switch (tree->symbol) {
      case TOK_ROOT      : postorder(tree);                    break;
      case TOK_FUNCTION  : postorder_emit_func(tree);          break;
      case TOK_PROTOTYPE :                                     break;
      case TOK_STRUCT    : postorder_emit_struct(tree);        break;
      case ';'           : postorder_emit_semi (tree);         break;
      case '='           : emit_assign (tree);                 break;
      case '+'           : postorder_emit_oper (tree);         break;
      case '-'           : postorder_emit_oper (tree);         break;
      case '*'           : postorder_emit_oper (tree);         break;
      case '/'           : postorder_emit_oper (tree);         break;
      case '%'           : postorder_emit_oper (tree);         break;
      case TOK_POS       : postorder_emit_oper (tree);         break;
      case TOK_NEG       : postorder_emit_oper (tree);         break;
      case TOK_INT       : postorder_emit_var(tree, "int");    break;
      case TOK_STRING    : postorder_emit_var(tree, "string"); break;
      case TOK_VOID      : postorder_emit_var(tree, "void");   break;
      case TOK_PARAM     : postorder_emit_param(tree);         break;
      case TOK_BLOCK     : postorder(tree);                    break;
      case TOK_CALL      : postorder_emit_call(tree);          break;
      case TOK_VARDECL   : emit_vardecl(tree);                 break;
      case TOK_CHARCON   : emit_push (tree, "char");           break;
      case TOK_IDENT     : emit_push (tree, "ident");          break;
      case TOK_INTCON    : emit_push (tree, "num");            break;
      case TOK_STRINGCON : emit_push(tree, "str");             break;
      case TOK_RETURN    : postorder_emit_return(tree);        break;
      case TOK_TYPEID    : postorder(tree);                    break;
      case TOK_PTR       : postorder_emit_ptr(tree);           break;
      case TOK_ARROW     : postorder(tree);                    break;
      case TOK_FIELD     : postorder(tree);                    break;
      case '{'           : postorder(tree);                    break;
      case TOK_ALLOC     : postorder(tree);                    break;
      case TOK_ARRAY     : postorder(tree);                    break;
      case TOK_WHILE     : postorder_emit_while(tree);         break;
      case TOK_IF        : postorder_emit_if(tree);            break;
      case TOK_ELSE      : postorder(tree);                    break;
      case TOK_NOT       : postorder_emit_compare(tree);       break;
      case TOK_LE        : postorder_emit_compare(tree);       break;
      case TOK_LT        : postorder_emit_compare(tree);       break;
      case TOK_GE        : postorder_emit_compare(tree);       break;
      case TOK_GT        : postorder_emit_compare(tree);       break;
      case TOK_EQ        : postorder_emit_compare(tree);       break;
      case TOK_NE        : postorder(tree);                    break;
      case TOK_NULLPTR   : postorder (tree);                   break;
      case TOK_INDEX     : postorder (tree);                   break;
      default            : assert (false);                     break;
   }
}

//Emits the tree to the oil file
void emit_sm_code (astree* tree) {
   printf ("\n");
   if (tree) emit (tree);
}
