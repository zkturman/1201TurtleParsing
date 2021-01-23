#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#define STARTNUM 30
#define ERRORBUFFER 100

enum bool {false, true};
typedef enum bool bool;

struct lexeme{
   char *word;
   int index;
   struct lexeme *next;
   struct lexeme *prev;
};
typedef struct lexeme lexeme;

struct sequence{
   lexeme *start;
   lexeme *current;
};
typedef struct sequence sequence;

struct program{
   sequence *code;
   int length;
   int capacity;
   bool valid;
   char *errMessage;
};
typedef struct program program;
void freeLexeme(lexeme *lex);
void freeSequence(sequence *s);
void freeProgram(program *p);
lexeme *createLexeme(char *word);
sequence *createSequence();
bool addLexeme(program *p, lexeme *word);
program *createProgram();
void *smartCalloc(int quantity, int size);
bool setProgError(program *p, char *message);
void errorQuit(char *message);
bool ruleMain(program *p);
bool ruleInstrctList(program *p);
bool ruleInstruction(program *p);
bool ruleTransform(program *p);
bool ruleDo(program *p);
bool ruleVar(program *p);
bool ruleVarnum(program *p);
bool ruleSet(program *p);
bool rulePolish(program *p);
bool ruleOp(program *p);
bool isOp(char c);
void test();

int main(int argc, char const *argv[]) {
   test();
   return 0;
}

void test(){
   lexeme *lex0, *lex1, *lex2, *lex3, *lex4, *lex5, *lex6, *lex7, *lex8;
   sequence *seq1;
   program *prog1;

   /*struct create testing*/
   lex1 = createLexeme("test");
   seq1 = createSequence();
   prog1 = createProgram();
   assert(lex1 != NULL);
   assert(strcmp(lex1->word, "test") == 0);
   assert(lex1->index == 0);
   assert(lex1->next ==NULL);
   assert(lex1->prev == NULL);
   assert(seq1->start == NULL);
   assert(seq1->current == NULL);
   assert(prog1->code->start == NULL);
   assert(prog1->code->current == NULL);
   assert(prog1->length == 0);
   assert(prog1->valid == true);
   assert(prog1->errMessage == NULL);
   assert(prog1->capacity == 0);

   /*test adding lexemes*/
   assert(addLexeme(NULL,lex1) == false);
   assert(addLexeme(prog1,NULL) == false);
   assert(addLexeme(prog1,lex1) == true);
   assert(strcmp(prog1->code->start->word,lex1->word) == 0);
   assert(strcmp(prog1->code->current->word,lex1->word) == 0);
   assert(prog1->length == 1);
   assert(lex1->index == 1);
   assert(prog1->code->start->index == prog1->length);
   lex2 = createLexeme("test2");
   assert(strcmp(prog1->code->start->word, lex1->word) == 0);
   assert(addLexeme(prog1,lex2) == true);
   assert(strcmp(prog1->code->current->word, lex2->word) == 0);
   assert(strcmp(prog1->code->start->next->word, lex2->word) == 0);
   assert(strcmp(prog1->code->current->prev->word, lex1->word) == 0);
   assert(prog1->length == 2);
   assert(lex2->index == 2);
   assert(prog1->code->current->index == prog1->length);

   prog1->code->current = prog1->code->start;
   assert(setProgError(prog1,"test error") == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage, "test error Issue encountered at word 1: "
      "test.\n") == 0);
   prog1->valid = true;
   free(prog1->errMessage);

   /*Test '{' and '}' start and end conditions for parser*/
   assert(ruleMain(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage,
         "Error: Program did not start with {. Issue encountered at word 1: "
         "test.\n") == 0);
   freeProgram(prog1);
   prog1 = createProgram();
   lex3 = createLexeme("{");
   addLexeme(prog1, lex3);
   assert(ruleMain(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage,
         "Error: Program did not end with }. Issue encountered at word 1: "
         "{.\n") == 0);
   prog1->valid = true;
   free(prog1->errMessage);
   lex5 = createLexeme("}");
   addLexeme(prog1,lex5);
   prog1->code->current = lex5->prev;
   assert(ruleInstrctList(prog1) == true);
   assert(ruleMain(prog1) == true);

   freeProgram(prog1);
   prog1 = createProgram();
   lex1 = createLexeme("-1.3");
   lex2 = createLexeme("1.3");
   lex3 = createLexeme("-1");
   lex4 = createLexeme("23");
   lex5 = createLexeme("1");
   lex6 = createLexeme("A");
   lex7 = createLexeme("TEST");
   lex8 = createLexeme("}");
   /*Test var and varnum*/
   addLexeme(prog1,lex1);
   assert(ruleVarnum(prog1) == true);
   addLexeme(prog1,lex2);
   assert(ruleVarnum(prog1) == true);
   addLexeme(prog1,lex3);
   assert(ruleVarnum(prog1) == true);
   addLexeme(prog1, lex4);
   assert(ruleVarnum(prog1) == true);
   addLexeme(prog1, lex5);
   assert(ruleVarnum(prog1) == true);
   addLexeme(prog1, lex6);
   assert(ruleVar(prog1) == true);
   assert(ruleVarnum(prog1) == true);
   addLexeme(prog1, lex7);
   assert(ruleVar(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage, "Error: VAR is too many characters. "
      "Issue encountered at word 7: TEST.\n") == 0);
   prog1->valid = true;
   free(prog1->errMessage);
   assert(ruleVarnum(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage, "Error: VAR is too many characters. "
      "Issue encountered at word 7: TEST.\n") == 0);
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, lex8);
   assert(ruleVar(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage, "Error: VAR is an unexpected character. "
      "Issue encountered at word 8: }.\n") == 0);
   prog1->valid = true;
   free(prog1->errMessage);
   assert(ruleVarnum(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage, "Error: VAR is an unexpected character. "
      "Issue encountered at word 8: }.\n") == 0);
   prog1->valid = true;
   free(prog1->errMessage);

   freeProgram(prog1);
   prog1 = createProgram();
   lex0 = createLexeme("{");
   lex1 = createLexeme("-1.3");
   lex2 = createLexeme("1.3");
   lex3 = createLexeme("-1");
   lex4 = createLexeme("23");
   lex5 = createLexeme("1");
   lex6 = createLexeme("A");
   lex7 = createLexeme("TEST");
   lex8 = createLexeme("}");
   /*Test rule transform and FD, RT, LT*/
   addLexeme(prog1,lex0);
   addLexeme(prog1,lex1);
   prog1->code->current = lex0;
   assert(ruleTransform(prog1) == true);
   addLexeme(prog1,lex2);
   prog1->code->current = lex1;
   assert(ruleTransform(prog1) == true);
   addLexeme(prog1,lex3);
   prog1->code->current = lex2;
   assert(ruleTransform(prog1) == true);
   addLexeme(prog1,lex4);
   prog1->code->current = lex3;
   assert(ruleTransform(prog1) == true);
   addLexeme(prog1,lex5);
   prog1->code->current = lex4;
   assert(ruleTransform(prog1) == true);
   addLexeme(prog1,lex6);
   prog1->code->current = lex5;
   assert(ruleTransform(prog1) == true);
   addLexeme(prog1,lex7);
   prog1->code->current = lex6;
   assert(ruleTransform(prog1) == false);
   assert(prog1->valid == false);
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1,lex8);
   prog1->code->current = lex7;
   assert(ruleTransform(prog1) == false);
   assert(prog1->valid == false);
   freeProgram(prog1);

   /*Test FD, RT, LT works*/
   prog1 = createProgram();
   lex0 = createLexeme("{");
   addLexeme(prog1, lex0);
   lex1 = createLexeme("FD");
   lex2 = createLexeme("1.3");
   addLexeme(prog1,lex1);
   addLexeme(prog1,lex2);
   prog1->code->current = lex2->prev;
   assert(ruleInstruction(prog1) == true);
   lex3 = createLexeme("RT");
   lex4 = createLexeme("23");
   addLexeme(prog1,lex3);
   addLexeme(prog1,lex4);
   prog1->code->current = lex4->prev;
   assert(ruleInstruction(prog1) == true);
   lex5 = createLexeme("LT");
   lex6 = createLexeme("A");
   addLexeme(prog1,lex5);
   addLexeme(prog1,lex6);
   prog1->code->current = lex6->prev;
   assert(ruleInstruction(prog1) == true);
   /*check junk intructions don't work*/
   lex7 = createLexeme("FD");
   lex8 = createLexeme("ABC");
   addLexeme(prog1, lex7);
   addLexeme(prog1, lex8);
   assert(ruleInstruction(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage, "Error: No proper instruction found. "
      "Issue encountered at word 9: ABC.\n") == 0);
   prog1->valid = true;
   free(prog1->errMessage);
   prog1->code->current = lex8->prev;
   assert(ruleInstruction(prog1) == false);
   assert(prog1->valid == false);
   assert(strcmp(prog1->errMessage, "Error: VAR is too many characters. "
      "Issue encountered at word 9: ABC.\n") == 0);

   /*Test Polish and OP*/
   assert(isOp('a') == false);
   assert(isOp('+') == true);
   assert(isOp('-') == true);
   assert(isOp('/') == true);
   assert(isOp('*') == true);



   /*Test Set*/

   /*Test Do*/

   free(seq1);
   freeProgram(prog1);
}

void *smartCalloc(int quantity, int size){
   void *v;
   v = calloc(quantity, size);
   if (v == NULL){
      errorQuit("Could not allocate memory...exiting\n");
   }
   return v;
}

void errorQuit(char *message){
   fprintf(stderr,"%s",message);
   exit(EXIT_FAILURE);
}

bool setProgError(program *p, char *message){
   char fullError[ERRORBUFFER];
   p->valid = false;
   sprintf(fullError,"%s Issue encountered at word %d: %s.\n",
      message, p->code->current->index, p->code->current->word);
   p->errMessage = (char *)smartCalloc(strlen(fullError) + 1,sizeof(char));
   strcpy(p->errMessage,fullError);
   return false;
}

bool ruleMain(program *p){
   p->code->current = p->code->start;
   if (strcmp(p->code->current->word,"{") != 0){
      return setProgError(p, "Error: Program did not start with {.");
   }
   return ruleInstrctList(p);
}

bool ruleInstrctList(program *p){
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Program did not end with }.");
   }
   p->code->current = p->code->current->next;
   if (strcmp(p->code->current->word,"}") == 0){
      return p->valid;
   }
   if (ruleInstruction(p) == false){
      return p->valid;
   }
   p->code->current = p->code->current->next;
   return ruleInstrctList(p);
}

bool ruleInstruction(program *p){
   if (strcmp(p->code->current->word,"FD") == 0){
      return ruleTransform(p);
   }
   else if (strcmp(p->code->current->word,"RT") == 0){
      return ruleTransform(p);
   }
   else if (strcmp(p->code->current->word,"LT") == 0){
      return ruleTransform(p);
   }
   else if (strcmp(p->code->current->word, "DO") == 0){
      return ruleDo(p);
   }
   else if (strcmp(p->code->current->word, "SET") == 0){
      return ruleSet(p);
   }
   return setProgError(p, "Error: No proper instruction found.");
}

bool ruleTransform(program *p){
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: No VARNUM found.");
   }
   p->code->current = p->code->current->next;
   return ruleVarnum(p);
}

bool ruleDo(program *p){
   if (p->code->current->next == NULL){
      setProgError(p, "Error: Null DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVar(p) == false){
      return p->valid;
   }
   if (p->code->current->next == NULL){
      setProgError(p, "Error: Expected FROM in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (strcmp(p->code->current->word, "FROM") != 0){
      setProgError(p, "Error: Expected FROM in DO instruction.");
   }
   if (p->code->current->next == NULL){
      setProgError(p, "Error: Expected VARNUM in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVarnum(p) == false){
      return p->valid;
   }
   if (p->code->current->next == NULL){
      setProgError(p, "Error: Expected TO in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (strcmp(p->code->current->word, "TO") != 0){
      setProgError(p, "Error: Expected TO in DO instruction.");
   }
   if (p->code->current->next == NULL){
      setProgError(p, "Error: Expected VARNUM in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVarnum(p) == false){
      return p->valid;
   }
   if (p->code->current->next == NULL){
      setProgError(p, "Error: Expected { in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (strcmp(p->code->current->word,"{") != 0){
      return setProgError(p, "Error: Expected { in DO instruction.");
   }
   return ruleInstrctList(p);
}

bool ruleVar(program *p){
   char check;
   if (strlen(p->code->current->word) > 1){
      return setProgError(p,"Error: VAR is too many characters.");
   }
   check = p->code->current->word[0];
   if (isupper(check) == false || isalpha(check) == false){
      return setProgError(p,"Error: VAR is an unexpected character.");
   }
   return p->valid;
}

bool ruleVarnum(program *p){
   if (strspn(p->code->current->word,"0123456789") == strlen(p->code->current->word)){
      return p->valid;
   }
   if (strspn(p->code->current->word,"-.0123456789") == strlen(p->code->current->word)){
      if (strspn(p->code->current->word,"-") > 0){
         if (strspn(p->code->current->word,"-") > 1 || p->code->current->word[0] != '-'){
            return setProgError(p, "Error: VARNUM contains invalid characters.");
         }
      }
      if (strspn(p->code->current->word,".") > 0){
         printf("failed here 2\n");
         return setProgError(p, "Error: VARNUM contains invalid characters.");
      }
      return p->valid;
   }
   return ruleVar(p);
}

bool ruleSet(program *p){
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Null SET instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVar(p) == false){
      return p->valid;
   }
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Expected := in SET instruction.");
   }
   p->code->current = p->code->current->next;
   if (strcmp(p->code->current->word, ":=") == 0){
      return setProgError(p, "Error: Expected := in SET instruction.");
   }
   if (rulePolish(p) == false){
      return p->valid;
   }
   return p->valid;
}

bool rulePolish(program *p){
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Null POLISH instruction.");
   }
   p->code->current = p->code->current->next;
   if (strcmp(p->code->current->word, ";") == 0){
      return p->valid;
   }
   if (strlen(p->code->current->word) == 1 && isOp(p->code->current->word[0])){
      if (ruleOp(p) == false){
         return p->valid;
      }
   }
   else if (ruleVarnum(p) == false){
      return p->valid;
   }
   return rulePolish(p);
}

bool ruleOp(program *p){
   if (strlen(p->code->current->word) > 1){
      return setProgError(p, "Error: OP is more than one character.");
   }
   if (isOp(p->code->current->word[0]) == false){
      return setProgError(p, "Error: Invalid OP found.");
   }
   return p->valid;
}

bool isOp(char c){
   switch(c){
      case '+':
         return true;
      case '-':
         return true;
      case '*':
         return true;
      case '/':
         return true;
      default:
         return false;
   }
}

void freeLexeme(lexeme *lex){
   free(lex->word);
   free(lex);
}
void freeSequence(sequence *s){
   lexeme *lex;
   lex = s->start;
   while(lex->next != NULL){
      lex = lex->next;
      freeLexeme(lex->prev);
   }
   freeLexeme(lex);
   free(s);
}
void freeProgram(program *p){
   if (p->valid == false){
      free(p->errMessage);
   }
   freeSequence(p->code);
   free(p);
}
lexeme *createLexeme(char *word){
   lexeme *lex;
   lex = (lexeme *)smartCalloc(1,sizeof(lexeme));
   lex->word = (char *)smartCalloc(strlen(word) + 1, sizeof(char));
   lex->word = strcpy(lex->word,word);
   return lex;
}
sequence *createSequence(){
   sequence *s;
   s = (sequence *)smartCalloc(1, sizeof(sequence));
   return s;
}
program *createProgram(){
   program *p;
   sequence *seq;
   p = (program *)smartCalloc(1,sizeof(program));
   seq = createSequence();
   p->code = seq;
   p->length = 0;
   p->valid = true;
   return p;
}
bool addLexeme(program *p, lexeme *word){
   if (p == NULL || word == NULL){
      return false;
   }
   if (p->code->start == NULL){
      p->code->start = word;
      p->code->current = word;
   }
   else {
      p->code->current->next = word;
      word->prev = p->code->current;
      p->code->current = word;

   }
   p->length++;
   word->index = p->length;
   return true;;
}
