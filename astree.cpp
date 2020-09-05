// $Id: astree.cpp,v 1.17 2019-03-15 14:32:40-07 - - $

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "string_set.h"
#include "lyutils.h"

extern FILE* tok_file;

astree::astree (int symbol_, const location& lloc_, const char* info) {
   symbol = symbol_;
   lloc = lloc_;
   lexinfo = string_set::intern (info);
   // vector defaults to empty -- no children
   block_nr = 0;
   attributes = *(new attr_bitset());
   symbol_item = nullptr;
}

astree::~astree() {
   while (not children.empty()) {
      astree* child = children.back();
      children.pop_back();
      delete child;
   }
   if (yydebug) {
      fprintf (stderr, "Deleting astree (");
      astree::dump (stderr, this);
      fprintf (stderr, ")\n");
   }
}

astree* astree::adopt (astree* child1, astree* child2) {
   if (child1 != nullptr) children.push_back (child1);
   if (child2 != nullptr) children.push_back (child2);
   return this;
}

astree* astree::adopt_sym (astree* child, int symbol_) {
   symbol = symbol_;
   return adopt (child);
}

void astree::zap_sym (int symbol_) {
   symbol = symbol_;
}

void astree::dump_node (FILE* outfile) {
   fprintf (outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
            static_cast<const void*> (this),
            parser::get_tname (symbol),
            lloc.filenr, lloc.linenr, lloc.offset,
            lexinfo->c_str());
   for (size_t child = 0; child < children.size(); ++child) {
      fprintf (outfile, " %p",
               static_cast<const void*> (children.at(child)));
   }
}

void astree::dump_tree (FILE* outfile, int depth) {
   fprintf (outfile, "%*s", depth * 3, "");
   dump_node (outfile);
   fprintf (outfile, "\n");
   for (astree* child: children) child->dump_tree (outfile, depth + 1);
   fflush (nullptr);
}

void astree::dump (FILE* outfile, astree* tree) {
   if (tree == nullptr) fprintf (outfile, "nullptr");
                   else tree->dump_node (outfile);
}

const string attr_to_string (size_t attri) {
    attr attribute = static_cast<attr> (attri);
    static const unordered_map<attr, string> hash {
            {attr::VOID       , "void"       },
            {attr::INT        , "int"        },
            {attr::NULLPTR_T  , "null"       },
            {attr::STRING     , "string"     },
            {attr::STRUCT     , "struct"     },
            {attr::ARRAY      , "array"      },
            {attr::FUNCTION   , "function"   },
            {attr::VARIABLE   , "variable"   },
            {attr::FIELD      , "field"      },
            {attr::TYPEID     , "typeid"     },
            {attr::PARAM      , "param"      },
            {attr::LOCAL      , "local"      },
            {attr::LVAL       , "lval"       },
            {attr::CONST      , "const"      },
            {attr::VREG       , "vreg"       },
            {attr::VADDR      , "vaddr"      },
            {attr::BITSET_SIZE, "bitset_size"},
    };
    auto str = hash.find (attribute);
    if (str == hash.end()) {
        throw invalid_argument (string (__PRETTY_FUNCTION__) + ": "
                + to_string (unsigned (attribute)));
    }
    return str->second;
}

void astree::print (FILE* outfile, astree* tree, int depth) {
   for(int i = 0; i < depth; ++i) {
      fprintf (outfile, "|  ");
   }
  
   const char *tname = parser::get_tname(tree->symbol);
   if(strstr (tname, "TOK_") == tname) tname += 4;

    if(strcmp("ROOT", tname) != 0){
       fprintf(tok_file, " %4ld   %-6.3f  %-3d  %-10s  %-s\n",
           tree->lloc.filenr, tree->lloc.linenr + 
           tree->lloc.offset/1.0000, tree->symbol, 
           tname, tree->lexinfo->c_str());
   }


   fprintf (outfile, "%s \"%s\" (%zd.%zd.%zd)\n",
            tname, tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset);


   for(size_t i = 0; i < static_cast<size_t>(attr::BITSET_SIZE); ++i) {
       if(tree->attributes.test(i)){
           const char* s = attr_to_string(i).c_str();
           if(!strcmp(s, "typeid"))
               continue;

           fprintf(outfile, " %s", attr_to_string(i).c_str());
           
           if(!strcmp(s, "struct")) {
               if(tree->symbol_item != nullptr)
               fprintf(outfile, " \"%s\"", 
                       tree->symbol_item->type_name.c_str());
           }
       }
   }

   if(tree->symbol_item != nullptr) {
       fprintf(outfile, " (%zd.%zd.%zd)", 
               tree->symbol_item->lloc.filenr,
               tree->symbol_item->lloc.linenr,
               tree->symbol_item->lloc.offset);
   }

   for (astree* child: tree->children) {
      astree::print (outfile, child, depth + 1);
   }
}
  
void destroy (astree* tree1, astree* tree2) {
   if (tree1 != nullptr) delete tree1;
   if (tree2 != nullptr) delete tree2;
}

void errllocprintf (const location& lloc, const char* format,
                    const char* arg) {
   static char buffer[0x1000];
   assert (sizeof buffer > strlen (format) + strlen (arg));
   snprintf (buffer, sizeof buffer, format, arg);
   const string* filename = lexer::filename (lloc.filenr);
   errprintf ("%s:%zd.%zd: %s", (*filename).c_str(), lloc.linenr, 
           lloc.offset, buffer);
}
