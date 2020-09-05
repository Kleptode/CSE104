%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

%}

%option 8bit
%option debug
%option nobackup
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn

LETTER          [A-Za-z_]
DIGIT           [0-9]
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
BAD_IDENT       ({DIGIT}+{IDENT})
NUMBER          {DIGIT}+
CHAR            ('([^\\'\n]|\\[\\'"0nt])')
BAD_CHAR        ('[\n\'\\]'|'.[^\'])
STRING          (\"([^\\"\n]|\\[\\'"0nt])*\")
BAD_STRING      (\"[^\n\"\\]*(.|\n))

%%

"+"             { return lexer::token ('+');       }
"-"             { return lexer::token ('-');       }
"*"             { return lexer::token ('*');       }
"/"             { return lexer::token ('/');       }
"%"             { return lexer::token ('%');       }
"="             { return lexer::token ('=');       }
"=="            { return lexer::token (TOK_EQ);    }
"!="            { return lexer::token (TOK_NE);    }
"<"             { return lexer::token (TOK_LT);    }
"<="            { return lexer::token (TOK_LE);    }
">"             { return lexer::token (TOK_GT);    }
">="            { return lexer::token (TOK_GE);    }
"->"            { return lexer::token (TOK_ARROW); }
","             { return lexer::token (',');       }
";"             { return lexer::token (';');       }
"("             { return lexer::token ('(');       }
")"             { return lexer::token (')');       }
"["             { return lexer::token ('[');       }
"]"             { return lexer::token (']');       }
"{"             { return lexer::token ('{');       }
"}"             { return lexer::token ('}');       }

"if"            { return lexer::token (TOK_IF);      }
"else"          { return lexer::token (TOK_ELSE);    }
"while"         { return lexer::token (TOK_WHILE);   }
"return"        { return lexer::token (TOK_RETURN);  }
"alloc"         { return lexer::token (TOK_ALLOC);   }
"nullptr"       { return lexer::token (TOK_NULLPTR); }
"not"           { return lexer::token (TOK_NOT);     }
"int"           { return lexer::token (TOK_INT);     }
"string"        { return lexer::token (TOK_STRING);  }
"struct"        { return lexer::token (TOK_STRUCT);  }
"array"         { return lexer::token (TOK_ARRAY);   }
"ptr"           { return lexer::token (TOK_PTR);     }
"void"          { return lexer::token (TOK_VOID);    }

{IDENT}         { return lexer::token (TOK_IDENT);     }
{NUMBER}        { return lexer::token (TOK_INTCON);    }
{CHAR}          { return lexer::token (TOK_CHARCON);   }
{STRING}        { return lexer::token (TOK_STRINGCON); }

"#".*           { lexer::include(); }
[ \t]+          {                   }
\n              { lexer::newline(); }

{BAD_IDENT}     { lexer::badtoken (TOK_IDENT);     }
{BAD_CHAR}      { lexer::badtoken (TOK_CHARCON);   }
{BAD_STRING}    { lexer::badtoken (TOK_STRINGCON); }
.               { lexer::badchar (*yytext);        }

%%
