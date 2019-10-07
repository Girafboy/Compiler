%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */

%x COMMENT STRING

%%

 /*
  * TODO: Put your codes here (lab2).
  *
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  */

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
"\n" {adjust(); errormsg.Newline();}

 /* fixed symbol */
"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}
"." {adjust(); return Parser::DOT;}
"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"do" {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
"nil" {adjust(); return Parser::NIL;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}

 /* Identifier and Integer */
[[:alpha:]_][[:alnum:]_]* {adjust(); stringBuf_=matched(); return Parser::ID;}
[[:digit:]]+ {adjust(); stringBuf_=matched(); return Parser::INT;}

/* String and Comment */
\" {adjust(); begin(StartCondition__::STRING); stringBuf_.clear();}
"/*" {{adjust(); commentLevel_=0; begin(StartCondition__::COMMENT);}}

<STRING>{
  \" {adjustStr(); begin(StartCondition__::INITIAL); setMatched(stringBuf_); return Parser::STRING;}
  \\\" {adjustStr(); stringBuf_ += '\"';}
  \\\\ {adjustStr(); stringBuf_ += '\\';}
  \\n {adjustStr(); stringBuf_ += '\n';}
  \\t {adjustStr(); stringBuf_ += '\t';}
  \\[[:digit:]]{3} {adjustStr(); stringBuf_ += (char)atoi(matched().c_str() + 1);}
  \\[ \n\t\f]+\\ {adjustStr();}
  \\\^[A-Z] {adjustStr(); stringBuf_ += matched()[2] - 'A' + 1;}
  . {adjustStr(); stringBuf_ += matched();}
}

<COMMENT>{
  "*/" {adjustStr(); if(commentLevel_) commentLevel_--; else begin(StartCondition__::INITIAL);}
  .|\n {adjustStr();}
  "/*" {adjustStr(); commentLevel_++;}
}

/* remain error */
. {adjust(); errormsg.Error(errormsg.tokPos, "illegal token");}
