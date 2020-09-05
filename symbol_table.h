#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

#include <bitset>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "auxlib.h"

struct astree;

enum class attr {
   VOID, INT, NULLPTR_T, STRING, STRUCT, ARRAY, FUNCTION, VARIABLE,
   FIELD, TYPEID, PARAM, LOCAL, LVAL, CONST, VREG, VADDR, BITSET_SIZE,
};
using attr_bitset = bitset<unsigned(attr::BITSET_SIZE)>;

struct symbol_node;

using symbol_table = unordered_map<string, symbol_node*>;
using symbol_entry = symbol_table::value_type;

struct symbol_node {
   attr_bitset attributes;
   size_t sequence;
   symbol_table* fields;
   location lloc;
   size_t block_nr;
   vector<symbol_node*>* parameters;
   string type_name;

   symbol_node(location lloc, size_t nr);
   symbol_node();
   void print(const string* name, FILE* file);
};

enum class types {
   ASSIGN, BINOP, CALL, COMPARE, FIELD, IDENT, INDEX, 
   INTCON, NULLPTR, PTR, RETURN, STRCON, TYPEID, UNOP,
   VARDECL, NOMATTER
};

struct symbol_generator {
   symbol_table* structure;
   symbol_table* global;
   symbol_table* local;
   symbol_node* func_node;
   size_t block_nr;
   size_t next_block;
   FILE* outfile;

   symbol_generator(FILE* file);
   symbol_generator();
   void traverse(astree* root);
   void type_check(astree* root);
   void func_stmt(astree* root, symbol_table* table);
   symbol_node* check_struct(astree* root);
   symbol_node* check_var(astree* root);
   symbol_node* ident_decl(astree* root, symbol_table* table,
                            const string& decl_type, size_t seq = 0); 
};

void dump_symbol_table(symbol_table* table);
void type_check(const astree* root, types type);
void set(astree* root, attr attri);
void set(astree* root, const attr_bitset& attris);
void set(symbol_node* node, attr attri);
bool is_compatible(const attr_bitset& a, const attr_bitset& b);
bool test(const astree* root, attr attri);
bool test(const attr_bitset& attrs, attr attri);
attr get_basetype(const astree* root);
const string attrs_to_string(const attr_bitset& attrs,
                             const string& name);

#endif
