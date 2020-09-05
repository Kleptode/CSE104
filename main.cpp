// $Id: main.cpp,v 1.18 2017-10-19 16:02:14-07 - - $

#include <string>
#include <vector>
#include <iostream>
using namespace std;

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "astree.h"
#include "auxlib.h"
#include "emitter.h"
#include "lyutils.h"
#include "string_set.h"

const string cpp_name = "/usr/bin/cpp";
string cpp_command;
FILE* tok_file;
FILE* oil_file;

// Open a pipe from the C preprocessor.
// Exit failure if can't.
// Assigns opened pipe to FILE* yyin.
void cpp_popen (const char* filename) {
   cpp_command = cpp_name + " " + filename;
   yyin = popen (cpp_command.c_str(), "r");
   if (yyin == nullptr) {
      syserrprintf (cpp_command.c_str());
      exit (exec::exit_status);
   }else {
      if (yy_flex_debug) {
         fprintf (stderr, "-- popen (%s), fileno(yyin) = %d\n",
                  cpp_command.c_str(), fileno (yyin));
      }
      lexer::newfilename (cpp_command);
   }
}

void cpp_pclose() {
   int pclose_rc = pclose (yyin);
   eprint_status (cpp_command.c_str(), pclose_rc);
   if (pclose_rc != 0) exec::exit_status = EXIT_FAILURE;
}


void scan_opts (int argc, char** argv) {
   opterr = 0;
   yy_flex_debug = 0;
   yydebug = 0;
   lexer::interactive = isatty (fileno (stdin))
                    and isatty (fileno (stdout));
   for(;;) {
      int opt = getopt (argc, argv, "@:ly");
      if (opt == EOF) break;
      switch (opt) {
         case '@': set_debugflags (optarg);   break;
         case 'l': yy_flex_debug = 1;         break;
         case 'y': yydebug = 1;               break;
         default:  errprintf ("bad option (%c)\n", optopt); break;
      }
   }
   if (optind > argc) {
      errprintf ("Usage: %s [-ly] [filename]\n",
                 exec::execname.c_str());
      exit (exec::exit_status);
   }
   const char* filename = optind == argc ? "-" : argv[optind];
   cpp_popen (filename);
}

int main (int argc, char** argv) {
   exec::execname = basename (argv[0]);
   if (yydebug or yy_flex_debug) {
      fprintf (stderr, "Command:");
      for (char** arg = &argv[0]; arg < &argv[argc]; ++arg) {
            fprintf (stderr, " %s", *arg);
      }
      fprintf (stderr, "\n");
   }
   scan_opts (argc, argv);
   int parse_rc = yyparse();
   cpp_pclose();
   yylex_destroy();
   if (yydebug or yy_flex_debug) {
      fprintf (stderr, "Dumping parser::root:\n");
      if (parser::root != nullptr) parser::root->dump_tree (stderr);
      fprintf (stderr, "Dumping string_set:\n");
      string_set::dump (stderr);
   }
   if (parse_rc) {
      errprintf ("parse failed (%d)\n", parse_rc);
   }else {
      string fn = argv[argc-1]; 
      fn = fn.substr(0, fn.size() - 3);
      string str = fn + ".str";
      string tok = fn + ".tok";
      string ast = fn + ".ast";
      string sym = fn + ".sym";
      string oil = fn + ".oil";
      FILE* str_file = fopen(str.c_str(), "w");
      tok_file = fopen(tok.c_str(), "w");
      FILE* ast_file = fopen(ast.c_str(), "w");
      FILE* sym_file = fopen(sym.c_str(), "w");
      oil_file = fopen(oil.c_str(), "w");

      string_set::dump(str_file);
      fprintf(tok_file, "# \"%s\"\n", argv[argc-1]);
      astree::print (ast_file, parser::root); 
     /* symbol_generator* generator = new symbol_generator(sym_file);
      generator->traverse(parser::root);*/
      emit_sm_code(parser::root);
      delete parser::root;

      pclose(str_file);
      pclose(tok_file);
      pclose(ast_file);
      pclose(sym_file);
      pclose(oil_file);
   }
   return exec::exit_status;
}

