#include <string.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "astree.h"
#include "lyutils.h"
#include "symbol_table.h"

#define NO_SEQ 0xffffffff

enum class types;
const string attr_to_string (size_t attri);

//Defnies function types to be used while
//generating the symbol table
//Modeled after code provided by Wesley Mackey
types type_name_hash(const char* token){
   static const unordered_map<string, types> hash{
      {"'='",           types::ASSIGN   },
      {"'+'",           types::BINOP    },
      {"'-'",           types::BINOP    },
      {"'*'",           types::BINOP    },
      {"'/'",           types::BINOP    },
      {"'%'",           types::BINOP    },
      {"TOK_CALL",      types::CALL     },
      {"TOK_EQ",        types::COMPARE  },
      {"TOK_NQ",        types::COMPARE  },
      {"'<'",           types::COMPARE  },
      {"'>'",           types::COMPARE  },
      {"TOK_LE",        types::COMPARE  },
      {"TOK_GE",        types::COMPARE  },
      {"TOK_ARROW",     types::FIELD    },
      {"TOK_IDENT",     types::IDENT    },
      {"TOK_INDEX",     types::INDEX    },
      {"TOK_CHARCON",   types::INTCON   },
      {"TOK_INTCON",    types::INTCON   },
      {"TOK_NULLPTR_T", types::NULLPTR  },
      {"TOK_PTR",       types::PTR      },
      {"TOK_RETURN",    types::RETURN   },
      {"TOK_STRINGCON", types::STRCON   },
      {"TOK_TYPEID",    types::TYPEID   },
      {"TOK_POS",       types::UNOP     },
      {"TOK_NEG",       types::UNOP     },
      {"TOK_NOT",       types::UNOP     },
      {"TOK_VARDECL",   types::VARDECL  }
   };
   auto iter = hash.find(string(token));

   if(iter != hash.end())
      return iter->second;

   return types::NOMATTER;

   throw invalid_argument (string (__PRETTY_FUNCTION__)
                           + ":" + string (token));  
}

//Prints the symbol table to an output file
void dump_symbol_table(symbol_table* table, FILE* outfile) {
   vector<pair<string, symbol_node*>> map(table->begin(), table->end());

   sort(map.begin(), map.end(), 
        [](symbol_entry entry1, symbol_entry entry2){

      const location& left = (entry1.second)->lloc;
      const location& right = (entry2.second)->lloc;
      if(left.filenr != right.filenr)
         return left.filenr < right.filenr;

      if(left.linenr != right.linenr)
         return left.linenr < right.linenr;

      return left.offset < right.offset;
   });

   for(auto i: map){
      fprintf(outfile, "   ");
      i.second->print(&(i.first), outfile);
   }
}



//All set functions set the attributes of an
//astree or symbol node.
void set(astree* node, attr attribute){
   node->attributes.set(static_cast<size_t> (attribute));
}

void set(astree* node, const attr_bitset& attributes){
   node->attributes = attributes;
}

void set(symbol_node* node, attr attribute){
   node->attributes.set(static_cast<size_t> (attribute));
}


//Adds a string, symbol node pair to a symbol table
void table_insert(const string& s, symbol_node* node, 
                                   symbol_table* table){
   auto i = table->find(s);
   if(i == table->end()){
      table->insert({s, node});
      return;
   }

   errllocprintf(node->lloc, "duplicate variable %s\n", s.c_str()); 
}

//Prints the attributes of a bitset
void print_attributes(attr_bitset& attributes, const string& name){
   for(size_t i = 0; i < static_cast<size_t>(attr::BITSET_SIZE); ++i){
      if(attributes.test(i)) {
         const char* attr_string = attr_to_string(i).c_str();
         printf(" %s", attr_string);
         if(!strcmp(attr_string, "struct"))
             printf(" \"%s\"", name.c_str());
      }
   }

   printf("\n");
}

//Converts an attribute bitset to a string.
//Based on code provided by Wesley Mackey.
const string attrs_to_string(const attr_bitset& attributes, 
                             const string& name){
    string attr_string = "";
    for(size_t i = 0; i < static_cast<size_t>(attr::BITSET_SIZE); ++i){
        if(attributes.test(i)){
            attr_string += " " + attr_to_string(i);
            if(attr_to_string(i) == "struct")
                attr_string += " \"" + name + "\"";
        }
    }
    return attr_string;
}


//Returns the basetype of a node in the astree.
//Used to retrofit the .ast file
attr get_basetype(const astree* root){
   const string& basetype = *(root->lexinfo);
   if(basetype == "int") 
      return attr::INT;

   else if(basetype == "string")
      return attr::STRING;

   else if(basetype == "array")
      return attr::ARRAY;

   else if(basetype == "void") 
      return attr::VOID;
 
   else
      return attr::STRUCT;
}

//All test functions check the attributes of an 
//astree or symbol node.
bool test(const astree* node, attr attribute){
   return node->attributes.test(static_cast<size_t> (attribute));
}

bool test(const attr_bitset& attributes, attr attribute){
   return attributes.test(static_cast<size_t> (attribute));
}

//All is_compatible function check if 2 bitsets or vectors of bitsets
//are of matching types. Used to iplement type checking.
bool is_compatible(const attr_bitset& cmp1, const attr_bitset& cmp2){
   bool status = false;
   static size_t shared = static_cast<size_t>(attr::BITSET_SIZE) -
                          static_cast<size_t>(attr::ARRAY);

   status |= ((cmp1<<shared) == (cmp2<<shared));
   status |= ((test(cmp1, attr::ARRAY)
             ||test(cmp1, attr::INT)
             ||test(cmp1, attr::STRING)
             ||test(cmp1, attr::STRUCT))
             &&test(cmp2, attr::NULLPTR_T));
 
   return status; 
}

bool is_compatible(const vector<symbol_node*>* cmp1, 
                   const vector<symbol_node*>* cmp2){

   if(cmp1->size() != cmp2->size())
      return false;

   int size = cmp1->size();
   for(int i = 0; i < size; ++i) {
      if(!is_compatible((*cmp1)[i]->attributes, (*cmp2)[i]->attributes))
         return false;
   }

   return true;
}

//Compare is used in the c++ sort function
bool compare(const symbol_entry& cmp1, 
             const symbol_entry& cmp2){

   const location& left = (cmp1.second)->lloc;
   const location& right = (cmp2.second)->lloc;
   return left.linenr < right.linenr;
}


//Checks if a function is in the symbol table.
symbol_node* check_function(const string& name, symbol_table* table){
   auto type = table->find(name);
   if(type != table->end())
      return type->second;

   return nullptr;
}

//Creates a symbol_generator object without an output file.
symbol_generator::symbol_generator(){
   structure = new symbol_table();
   global = new symbol_table();
   local = global;
   block_nr = 0;
   next_block = 0;
   func_node = nullptr;
   outfile = nullptr;
}

//Creates a symbol generattor object with a file.
symbol_generator::symbol_generator(FILE* file){
   structure = new symbol_table();
   global = new symbol_table();
   local = global;
   block_nr = 0;
   next_block = 0;
   func_node = nullptr;
   outfile = file;
}


//Performs type checking on the abstract syntax tree
void symbol_generator::type_check(astree* root) {
   const char* token = parser::get_tname (root->symbol);
   types type = type_name_hash(token);

   if(strcmp(token, "TOK_ARROW") && root->children.size()) {
      for(auto child : root->children)
         type_check(child);
   }

    root->block_nr = block_nr;
    astree* left = nullptr;
    astree* right = nullptr;
    if(root->children.size())
        left = root->children[0];
    if(root->children.size() > 1)
        right = root->children[1];

    size_t shr{};
    switch(type) {
        
   case types::ASSIGN:
      if(is_compatible(left->attributes, right->attributes)
        && test(left, attr::LVAL)){

         set(root, left->attributes);
         set(root, attr::VREG);
      }

      break;

   case types::BINOP:
      if(test(left, attr::INT)
         && test(right, attr::INT)){

         set(root, attr::INT);
         set(root, attr::VREG);
      }

      break;

   case types::CALL:{
      if(left->symbol_item == nullptr)
         break;

      vector<symbol_node*>* params = left->symbol_item->parameters;
      if(params->size() == root->children.size() - 1){
         auto i = root->children.begin() + 1;
         auto j = params->begin();
         for(; i < root->children.end(); ++i, ++j){
            if(is_compatible((*i)->attributes, (*j)->attributes))
               continue;

            else{
               astree* l = *i;
               symbol_node* r = *j;
               errllocprintf(root->lloc, 
                             "incompatible parameter \n\t%s\n",
                             (attrs_to_string(l->attributes, 
                              l->symbol_item ? 
                              l->symbol_item->type_name : "") + "\n\t"
                              + attrs_to_string(r->attributes, 
                              r->type_name)).c_str());

               break;
            }
         }

         set(root, left->symbol_item->attributes);
         set(root, attr::VREG);
      }

      else
         errllocprintf(left->lloc, "incompatible parameter %s\n",
                       left->lexinfo->c_str());

      break;
   }

   case types::COMPARE:
      if(is_compatible(left->attributes, right->attributes)){
         set(root, attr::INT);
         set(root, attr::VREG);
      }

      break;

   case types::FIELD:{
      type_check(left);
      if(left->symbol_item == nullptr)
         break;

      if(left->symbol_item->fields == nullptr)
         break;

      auto i = left->symbol_item->fields->find(*(right->lexinfo)); 
      if(i != left->symbol_item->fields->end()){
         set(root, attr::VADDR);
         set(root, attr::LVAL);
         set(root, i->second->attributes);
         break;
      }

      const astree* l = left;
      symbol_node* r = i->second;
      errllocprintf(root->lloc, "undefined field \n\t%s\n", 
                   (attrs_to_string(l->attributes, 
                   l->symbol_item ? l->symbol_item->type_name : "") 
                   + "\n\t" + attrs_to_string(r->attributes, 
                   r->type_name)).c_str());

      break;
   }

   case types::IDENT: 
      check_var(root);
      break;

   case types::INDEX:{
      shr = static_cast<size_t>(attr::BITSET_SIZE) -
            static_cast<size_t>(attr::STRUCT);

      if((test(left, attr::INT) 
        || test(left, attr::STRING) 
        || test(left, attr::STRUCT) 
        || test(left, attr::VOID)) 
        && test(left, attr::ARRAY)
        && test(right, attr::INT)){

         set(root, (left->attributes<<shr)>>shr);
         set(root, attr::VADDR);
         set(root, attr::LVAL);
      }

      else if(test(left, attr::STRING)
             && test(right, attr::INT)){

         set(root, attr::INT);
         set(root, attr::VADDR);
         set(root, attr::LVAL);
      }
      
      else
         errllocprintf(root->lloc, "incompatible index \n\t%s\n\t%s",
                      (attrs_to_string(left->attributes, 
                       left->symbol_item ?
                       left->symbol_item->type_name : "")
                       + attrs_to_string(right->attributes, 
                       right->symbol_item ? 
                       right->symbol_item->type_name : "")).c_str());
      break;
   }

   case types::INTCON: 
      set(root, attr::INT);
      set(root, attr::CONST);
      break;

   case types::NULLPTR:
      set(root, attr::NULLPTR_T);
      set(root, attr::CONST);
      break;

   case types::PTR:
      set(root, attr::STRUCT);
      break;

   case types::RETURN:{
      if((root->children.size() == 0
        && test(func_node->attributes, attr::VOID))
        || is_compatible(func_node->attributes, 
                         root->children[0]->attributes))
         break;

      astree* l = root->children[0];
      symbol_node* r = func_node;
      errllocprintf(root->lloc, "incompatible return type \n\t%s\n",
                   (attrs_to_string(l->attributes, 
                   l->symbol_item ? l->symbol_item->type_name : "") 
                   + "\n\t" + attrs_to_string(r->attributes, 
                   r->type_name)).c_str());
      break;
   }

   case types::STRCON:
      set(root, attr::STRING);
      set(root, attr::CONST);
      break;

   case types::TYPEID: 
      check_struct(root->children[0]);
      break;

   case types::UNOP:
      if(test(left, attr::INT)) {
         set(root, attr::INT);
         set(root, attr::VREG);    
      }

      break;

   default:
      break;
   }
}

//Checks if a struct is in the symbol table
symbol_node* symbol_generator::check_struct(astree* root){
   auto type = structure->find(*(root->lexinfo));
   
   if(type != structure->end()){
      root->symbol_item = type->second;
      root->attributes = type->second->attributes;
      return type->second;
   }
    
   errllocprintf(root->lloc, "undefined type: %s\n",
                 root->lexinfo->c_str());

   return nullptr;    
}

//Checks if a variable is in the symbol tables
symbol_node* symbol_generator::check_var(astree* root){
   auto type = local->find(*(root->lexinfo));
   if(type == local->end())
      type = global->find(*(root->lexinfo));

   if(type != global->end()){
      root->symbol_item = type->second;
      root->attributes = type->second->attributes;
      return type->second;
   }
    
   errllocprintf(root->lloc, "undefined variable: %s\n",
                 root->lexinfo->c_str());

   return nullptr;
}

//Handles identifiers
symbol_node* symbol_generator::ident_decl(astree* root,
                                          symbol_table* table, 
                                          const string& decl_type, 
                                          size_t seq) {
   astree* left = nullptr;
   astree* right = nullptr;
   astree* var = nullptr;
   if(root->children.size())
      left = root->children[0];

   if(root->children.size() > 1)
      right = root->children[1];

   symbol_node* symbol = new symbol_node(root->lloc, root->block_nr);

   attr basetype = get_basetype(root);
   set(symbol, basetype);

   if(decl_type == "ident"){
      set(symbol, attr::VARIABLE);
      set(symbol, attr::LVAL);
   }

   else if(decl_type == "func"){
      set(symbol, attr::FUNCTION);       
   }

   else if(decl_type == "field"){
      set(symbol, attr::FIELD);
   }

   else if(decl_type == "param"){
      symbol->block_nr = root->block_nr = block_nr; 
      set(symbol, attr::VARIABLE);
      set(symbol, attr::PARAM);
      set(symbol, attr::LVAL);
   }

   else if(decl_type == "local"){
      symbol->block_nr = root->block_nr = block_nr;
      set(symbol, attr::VARIABLE);
      set(symbol, attr::LOCAL);
      set(symbol, attr::LVAL);
   }

   if(basetype == attr::STRUCT){
      symbol_node* type = check_struct(root->children[0]);
      if(type == nullptr)
         return nullptr;

      symbol->lloc = left->lloc;
      symbol->fields = type->fields;
      symbol->type_name = type->type_name;
      var = left;
   }

   else if(basetype == attr::ARRAY){
      attr left_base = get_basetype(left);
      if(left_base == attr::STRUCT){
         symbol_node* type = check_struct(left);
         if(type == nullptr)
            return nullptr;
            
         symbol->fields = type->fields;
         symbol->type_name = type->type_name;
      }
        
      set(symbol, left_base);
      symbol->lloc = right->lloc;
      var = right;
   }

   else{
      symbol->lloc = left->lloc;
      var = left;
   }

   symbol->sequence = seq;   
   var->block_nr = root->block_nr;
   var->symbol_item = symbol;
   var->attributes = symbol->attributes;

   table_insert(*(var->lexinfo), symbol, table);

   return symbol;
}

//Creates a basic symbol node object
symbol_node::symbol_node(location l, size_t nr){
    attributes = *(new attr_bitset());
    sequence = 0;
    fields = nullptr;
    lloc = l;
    block_nr = nr;
    parameters = nullptr;
    type_name = "";
}

//Prints a symbol node
void symbol_node::print(const string* name, FILE* outfile) {
    fprintf(outfile, "%s (%zd.%zd.%zd) {%zd} %s",
            name->c_str(), lloc.filenr, lloc.linenr, lloc.offset,
            block_nr, attrs_to_string(attributes, type_name).c_str());

    if(sequence != NO_SEQ)
        fprintf(outfile, " %zd", sequence);

    fprintf(outfile, "\n");
}

//Handles functions during traversal
void symbol_generator::func_stmt(astree* root, symbol_table* table){
   int size = root->children.size();
   for(int i = 0; i < size; ++i) {
      astree* child = root->children[i];
      astree* left = nullptr;
      astree* right = nullptr;
      if(child->children.size())
         left = child->children[0];

      if(child->children.size() > 1)
         right = child->children[1];

      child->block_nr = block_nr;

      const char* token = parser::get_tname (child->symbol);

      if(!strcmp(token, "TOK_VARDECL")){
         type_check(right);
         symbol_node* var = ident_decl(left, table, "local", i);     
         if(var != nullptr && !is_compatible(var->attributes, 
                                             right->attributes)){
            errllocprintf(root->lloc, "incompatible types for %s\n", 
                          child->lexinfo->c_str());

            print_attributes(var->attributes, var->type_name);
            string temp = "";
            if(right->symbol_item != nullptr)
               temp = right->symbol_item->type_name;

            print_attributes(right->attributes, temp);
         }
      }

      else
         type_check(child);
   }
}

//Performs a post order traversal of the syntax tree and
//preforms type checking. Main function of the symbol_table file
void symbol_generator::traverse(astree* root){
   astree* left = nullptr;
   astree* right = nullptr;
   if(root->children.size())
      left = root->children[0];

   if(root->children.size() > 1)
      right = root->children[1];  

   const char* token = parser::get_tname(root->symbol); 

   if (!strcmp(token, "TOK_FUNCTION")){
      vector<symbol_node*>* parameters = new vector<symbol_node*>();
      symbol_table* table = new symbol_table();
      local = table;

      block_nr = next_block++;
      int j = 0;
      for(auto i = right->children.begin();
               i != right->children.end(); ++i, ++j){

         symbol_node* param = ident_decl(*i, table, "param", j);
         if(param != nullptr)
            parameters->push_back(param);
      }

      astree* function = *(left->children.end() - 1);
      symbol_node* prototype = check_function(*(function->lexinfo),
      global);
      if(prototype == nullptr){
         symbol_node* func = ident_decl(left, global, "func", NO_SEQ);
         if(func != nullptr) {
            func->parameters = parameters;
            prototype = func;
         }
      }

      else if(!is_compatible(parameters, prototype->parameters)){
         errllocprintf(root->lloc, 
                       "incompatible function prototypetype %s\n", 
                       function->lexinfo->c_str());
         return;
      }

      func_node = prototype;
      func_node->print(function->lexinfo, outfile);
      func_stmt(root->children[2], table);

      dump_symbol_table(local, outfile);
      local = global;
   }

   else if(!strcmp(token, "TOK_PROTOTYPE")){
      symbol_node* func = ident_decl(left, global, "func", NO_SEQ);
      if(func != nullptr){
         astree* function = *(left->children.end() - 1);
         func->print(function->lexinfo, outfile);
         func->parameters = new vector<symbol_node*>();
         symbol_table* table = new symbol_table();
         local = table;

         block_nr = next_block++;
         int j = 0;
         for(auto i = right->children.begin();
                  i != right->children.end(); ++i, ++j){

            symbol_node* param = ident_decl(*i, table, "param", j);
            if(param != nullptr)  
               func->parameters->push_back(param);
         }

         dump_symbol_table(local, outfile);
         local = global;
      }

   }
   
   else if (!strcmp(token, "TOK_STRUCT")){
      block_nr = 0;
      symbol_table* table = new symbol_table();
      symbol_node* node = new symbol_node(left->lloc, 0);
      set(node, attr::STRUCT);
      set(node, attr::TYPEID);
      node->fields = table;
      node->type_name = *(left->lexinfo);
      node->sequence = NO_SEQ;
      table_insert(*(left->lexinfo), node, structure);
      node->print(left->lexinfo, outfile);
      left->symbol_item = node;
      left->attributes = node->attributes;

      int j = 0;
      for(auto i = right->children.begin();
               i != right->children.end(); ++i, ++j)

         ident_decl(*i, table, "field", j);

      dump_symbol_table(table, outfile); 
   }

   else if(!strcmp(token, "TOK_VARDECL")){
      type_check(right);
      symbol_node* var = ident_decl(left, global, "ident", NO_SEQ);
      if(var == nullptr){
         ;; //do nothing
      }

      else if(is_compatible(var->attributes, right->attributes)){
         astree* decl_name = *(left->children.end() - 1);
         var->print(decl_name->lexinfo, outfile);
      }

      else{
         errllocprintf(root->lloc, "incompatible types for %s\n", 
                       root->lexinfo->c_str());

         print_attributes(var->attributes, var->type_name);
         string temp = "";
         if(right->symbol_item != nullptr)
            temp = right->symbol_item->type_name;
         print_attributes(right->attributes, temp);
      }
   }

   else if(!strcmp(token, "TOK_ROOT")){
      for(auto i = root->children.begin();
               i != root->children.end(); ++i){
         traverse(*i);
      }
   }

   else
      errllocprintf(root->lloc, "symbol_generate: invalid: %s\n", 
                    root->lexinfo->c_str());
}
