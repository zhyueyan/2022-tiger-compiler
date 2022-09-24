%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

 /*
  * Comments-handling: Change the states between 'INITIAL' and 'COMMENT' by recognizing '/ *' and '* /'.
  * Initiate a variable 'pairs' in scanner.h to calculate the pair of '/ *' and '* /' so as to ensure the ending of comments.
  *
  * String-handling: Transform the states between 'INITIAL' and STR' by recognizing '"'. Change the escape string into the real string value.
  * Transform the state into 'IGNORE' when recognizing '/' with one or more formatting characters.
  *
  * Error-handling: Give error message 'illegal token' when illegal char is recognized.
  *                 Skip white space chars.space, tabs and LF.
  *                 Give error message 'unpaired '"'' when " is not paired in the string.Give error message 'unpaired '"'' when " is not paired in the string.
  * End-of-file: Free the pointer when reading <<EOF>>.
  */

%x COMMENT STR IGNORE

%%

<INITIAL>{
    \"              {
                        initStr();
                        adjust();
                        begin(StartCondition__::STR);
                    }
    "/*"           {
                        adjust();
                        initPair();
                        begin(StartCondition__::COMMENT);
                    }  
}
<COMMENT>{
    "*/"        {
                    adjust();
                    if(!decreasePair()) {
                        
                        begin(StartCondition__::INITIAL);
                    }
                }
    "/*"        {
                    adjust();
                    addPair();
                }
    \n          {  
                    adjust(); errormsg_->Newline();
                }
    \\.|.       {
                    adjust();
                }
}
<IGNORE>{
    \\ {
                    adjustStr();
                    begin(StartCondition__::STR);
                 }

    .           adjustStr();
}
<STR>{
    \"          {
                    adjustStr();
                    begin(StartCondition__::INITIAL);
                    setMatched(getStr());
                    return Parser::STRING;
                }

    \\t         {
                    adjustStr();
                    addStr("\t");
                }
    
    \\n         {
                    adjustStr();
                    addStr("\n");
                }


    \\[0-9]{3}  {
                    adjustStr();
                    atos(matched());
                }

    \\^[a-zA-Z]  {
                    atoc(matched());
                    adjustStr();
                 }

    \\[\t\n\0\f] {
                    adjustStr();
                    begin(StartCondition__::IGNORE);
                 }

    \\\"        {
                    adjustStr();
                    addStr("\"");
                }

    \\.|.       {
                    adjustStr();
                    addStr(matched());
                }
    
}

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   //Parser::ID
  *   Parser::STRING
  *   //Parser::INT
  *   //Parser::COMMA
  *   //Parser::COLON
  *   //Parser::SEMICOLON
  *   //Parser::LPAREN
  *   //Parser::RPAREN
  *   //Parser::LBRACK
  *   //Parser::RBRACK
  *   //Parser::LBRACE
  *   //Parser::RBRACE
  *   //Parser::DOT
  *   //Parser::PLUS
  *   //arser::MINUS
  *   //Parser::TIMES
  *   //Parser::DIVIDE
  *   //Parser::EQ
  *   //Parser::NEQ
  *   //Parser::LT
  *   //Parser::LE
  *   //Parser::GT
  *   //Parser::GE
  *   //Parser::AND
  *   //Parser::OR
  *   //Parser::ASSIGN
  *   //Parser::ARRAY
  *   //Parser::IF
  *   //Parser::THEN
  *   //Parser::ELSE
  *   //Parser::WHILE
  *   //Parser::FOR
  *   //Parser::TO
  *   //Parser::DO
  *   //Parser::LET
  *   //Parser::IN
  *   //Parser::END
  *   //Parser::OF
  *   //Parser::BREAK
  *   //Parser::NIL
  *   //Parser::FUNCTION
  *   //Parser::VAR
  *   //Parser::TYPE
  */

 /* reserved words */
 "array" {adjust(); return Parser::ARRAY;}
 /* TODO: Put your lab2 code here */
 "let" {adjust();return Parser::LET;}
 "type" {adjust(); return Parser::TYPE;}
 "var" {adjust(); return Parser::VAR;}
 "function" {adjust(); return Parser::FUNCTION;}
 "if" {adjust(); return Parser::IF;}
 "then" {adjust(); return Parser::THEN;}
 "else" {adjust(); return Parser::ELSE;}
 "while" {adjust(); return Parser::WHILE;}
 "for" {adjust(); return Parser::FOR;}
 "to" {adjust(); return Parser::TO;}
 "do" {adjust(); return Parser::DO;}
 "in" {adjust(); return Parser::IN;}
 "end" {adjust(); return Parser::END;}
 "of" {adjust(); return Parser::OF;}
 "break" {adjust(); return Parser::BREAK;}
 "nil" {adjust(); return Parser::NIL;}

 "=" {adjust(); return Parser::EQ;}
 "<>" {adjust(); return Parser::NEQ;}
 "<" {adjust(); return Parser::LT;}
 "<=" {adjust(); return Parser::LE;}
 ">" {adjust(); return Parser::GT;}
 ">=" {adjust(); return Parser::GE;}
 "{" {adjust(); return Parser::LBRACE;}
 "}" {adjust(); return Parser::RBRACE;}
 "(" {adjust(); return Parser::LPAREN;}
 ")" {adjust(); return Parser::RPAREN;}
 "[" {adjust(); return Parser::LBRACK;}
 "]" {adjust(); return Parser::RBRACK;}
 ":=" {adjust(); return Parser::ASSIGN;}
 "," {adjust(); return Parser::COMMA;}
 "." {adjust(); return Parser::DOT;}
 "+" {adjust(); return Parser::PLUS;}
 "-" {adjust(); return Parser::MINUS;}
 "*" {adjust(); return Parser::TIMES;}
 "/" {adjust(); return Parser::DIVIDE;}
 "&" {adjust(); return Parser::AND;}
 "|" {adjust(); return Parser::OR;}
 ":" {adjust(); return Parser::COLON;}
 ";" {adjust(); return Parser::SEMICOLON;}

 

 /* regular */
 [a-zA-Z][A-Za-z0-9_]* {adjust(); return Parser::ID;}
 [0-9]+ {adjust(); return Parser::INT;}




 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
