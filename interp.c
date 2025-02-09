#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "neillsdl2.h"
#include "Stack/stack.h"

#define COMMANDARGS 2
#define FILEINDEX 1
#define COLOURMAX 256
#define FILEBUFFER 50
#define STARTNUM 30
#define ERRORBUFFER 100
#define RECTSIZE 20
#define MILLISECONDDELAY 20
#define ALPHANUM 26
#define WHITESPACE "\n\f\r\t "
#define OPCHARS "+-/*"
#define DIGITS "0123456789"
#define NUMCHARS "-.0123456789"
#define FACENORTH 90
#define DEGTORAD (M_PI / 180)
#define STREQ(A, B) (strcmp(A, B) == 0)

struct loop{
   double to;
   double from;
   int varIndex;
};
typedef struct loop loop;

struct turtle{
   double xcoord;
   double ycoord;
   double angle;
   double vars[26];
};
typedef struct turtle turtle;

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
   bool valid;
   char *errMessage;
   turtle squirt;
   double vars[26];
   stack *polish;
   SDL_Simplewin *sw;
};
typedef struct program program;

/*Reads a file and generates a sequence of words by delimiting the file at
whitespace characters. These words are added to the returned program struct.*/
program *readProgramFile(char *filename);

/*Returns true if a program follows the rule for the <MAIN> grammar*/
bool ruleMain(program *p);

/*A recursive function that returns true if all instructions in a program
follows the rules defined by <INSTRCTLST> and <INSTRUCTION> grammar*/
bool ruleInstrctList(program *p);

/*Returns true if an instruction follows <INSTRUCTION> grammar*/
bool ruleInstruction(program *p);

/*Returns true if the subsequent word for FD, RT, and LT instructions
is a valid variable or number. Uses SDL to draw lines for FD. RT and LT update
the angle of the turtle struct.*/
bool ruleTransform(program *p);

/*Draws a line in SDL and updates the screen based on a given distance and the
current coordinates of the turtle. Uses functions from neillsd2.*/
void drawline(program *p, double distance);

/*Gets the new x coordinate for the turtle based on the given distance.*/
double getNewX(double distance, turtle t);

/*Gets the new y coordinate for the turtle based on the given distance.*/
double getNewY(double distance, turtle t);

/*Updates the angle of the turtle in radiant based on a rotation in degrees.
Rotates right if right is true, otherwise left.*/
double getNewAngle(double oldAng, double rotation, bool right);

/*Returns true if the DO instruction follows the correct grammar*/
bool ruleDo(program *p);

/*Returns true if the DO instruction has correct FROM and TO grammar. Updates
the to and from variables of the loop stuct.*/
bool ruleDoInfo(program *p, loop *doLoop);

/*Returns true if the DO instruction has the correct FROM grammar. Updates the
for variable of the loop struct.*/
bool ruleDoFrom(program *p, loop *doLoop);

/*Returns true if the DO instruction has the correct TO grammar. Updates the
to variable of the loop struct.*/
bool ruleDoTo(program *p, loop *doLoop);

/*Returns true if the grammar of the DO loop is correct. Performs the actions
in the loop a certain number of times. The specified variable is set to the FROM
value and incremented by one. The loop repeats until the TO value is exceeded.*/
bool ruleDoLoop(program *p, loop doLoop);

/*Returns true if the SET instruction has the correct grammar. Updates the value
of a given variable. All variables are initialised to 0 if not set here.*/
bool ruleSet(program *p);

/*A recursive function that returns true if a POLISH expression has the
correct grammar. Adds numbers to a stack and operates on the top two stack
doubles. The stack ADT by Neill Campbell is used for this, but uses doubles
instead of ints.*/
bool rulePolish(program *p);

/*Returns true if the current word is a valid operator. Performs +, -, *, and /
operations on the stack by popping the top two values and pushing the result.*/
bool ruleOp(program *p);

/*Returns true if the current word meets the criteria for <VARNUM> grammar*/
bool ruleVarnum(program *p);

/*Returns the number of times a character c appears in a string*/
int charFrequency(char *str, char c);

/*Returns true if the current word meets the criteria for the <VAR> grammar*/
bool ruleVar(program *p);

/*Converts the current word from a string to a double.*/
double getValue(program *p);

/*Only returns false. Stops file reading and sets the error message in a
program struct when grammar rules aren't met.*/
bool setProgError(program *p, char *message);

/*Returns the index for the program vars array that corresponds to a given VAR*/
int getAlphaIndex(char c);

/*Returns an initialised program struct that contains a sequence of words*/
program *createProgram();

/*Returns a sequence struct that will hold a doubly linked list of words*/
sequence *createSequence();

/*Returns a lexeme struct that that will hold a word from a file*/
lexeme *createLexeme(char *word);

/*Returns true when the word is added to the program. This increases the
program's length variable and updates the index of the current word.*/
bool addLexeme(program *p, lexeme *word);

/*Used to calloc space and check for failed memory allocation. If allocation
fails, the program quits.*/
void *smartCalloc(int quantity, int size);

/*Quits the program and prints the specified message to stderr*/
void errorQuit(char *message);

/*Frees memory allocated for a program structure*/
void freeProgram(program *p);

/*Frees memory allocated for a sequence structure*/
void freeSequence(sequence *s);

/*Frees memory allocated for a lexeme structure*/
void freeLexeme(lexeme *lex);

/*Used to simulate program structures for realistic testing scenarios. Focuses
on functionality of the parser.*/
void testParse();

/*Used to simulate program structures for realistic testing scenarios*/
bool testProgram(char *progText, char *errorMessage);

/*Tests new functions added for the interpreter*/
void testInterp();

int main(int argc, char **argv) {
   program *p;
   char *filename;
   SDL_Simplewin sw;
   testParse();
   testInterp();
   if (argc != COMMANDARGS){
      errorQuit("Wrong number of arguments...exiting.\n");
   }
   Neill_SDL_Init(&sw);
   filename = argv[FILEINDEX];
   p = readProgramFile(filename);
   p->sw = &sw;
   Neill_SDL_SetDrawColour(&sw, COLOURMAX - 1, COLOURMAX - 1, COLOURMAX - 1);
   ruleMain(p);
   do{
      Neill_SDL_Events(&sw);
   } while (!sw.finished);
   if (p->valid == false){
      printf("%s", p->errMessage);
   }
   SDL_Quit();
   atexit(SDL_Quit);
   freeProgram(p);
   return 0;
}

program *readProgramFile(char *filename){
   char buffer[FILEBUFFER];
   char *token;
   FILE *fp;
   program *p;
   p = createProgram();

   if ((fp = fopen(filename, "r")) == NULL){
      printf("Could not open file...exiting\n");
      exit(EXIT_FAILURE);
   }
   while (fgets(buffer, FILEBUFFER, fp) != NULL){
      token = strtok(buffer, WHITESPACE);
      while (token != NULL){
         if (addLexeme(p, createLexeme(token)) == false){
            errorQuit("Could not add word...exiting\n");
         }
         token = strtok(NULL, WHITESPACE);
      }
   }
   fclose(fp);
   return p;
}

bool ruleMain(program *p){
   p->code->current = p->code->start;
   if (!STREQ(p->code->current->word,"{")){
      return setProgError(p, "Error: Program did not start with {.");
   }
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Program did not end with }.");
   }
   p->code->current = p->code->current->next;
   return ruleInstrctList(p);
}

bool ruleInstrctList(program *p){
   if (STREQ(p->code->current->word,"}")){
      return p->valid;
   }
   if (ruleInstruction(p) == false){
      return p->valid;
   }
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Program did not end with }.");
   }
   p->code->current = p->code->current->next;
   return ruleInstrctList(p);
}

bool ruleInstruction(program *p){
   if (STREQ(p->code->current->word,"FD")){
      return ruleTransform(p);
   }
   else if (STREQ(p->code->current->word,"RT")){
      return ruleTransform(p);
   }
   else if (STREQ(p->code->current->word,"LT")){
      return ruleTransform(p);
   }
   else if (STREQ(p->code->current->word, "DO")){
      return ruleDo(p);
   }
   else if (STREQ(p->code->current->word, "SET")){
      return ruleSet(p);
   }
   return setProgError(p, "Error: No proper instruction found.");
}

bool ruleTransform(program *p){
   double value;
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: No VARNUM found.");
   }
   p->code->current = p->code->current->next;
   if (ruleVarnum(p) == false){
      return p->valid;
   }
   value = getValue(p);
   if (STREQ(p->code->current->prev->word,"FD")){
      if (p->sw != NULL){
         drawline(p, value);
      }
   }
   else if (STREQ(p->code->current->prev->word,"RT")){
      p->squirt.angle = getNewAngle(p->squirt.angle, value, true);
   }
   else if (STREQ(p->code->current->prev->word,"LT")){
      p->squirt.angle = getNewAngle(p->squirt.angle, value, false);
   }
   return p->valid;
}

void drawline(program *p, double distance){
   double x, y, newX, newY;
   x = p->squirt.xcoord;
   y = p->squirt.ycoord;
   newX = getNewX(distance, p->squirt);
   newY = getNewY(distance, p->squirt);
   SDL_RenderDrawLine(p->sw->renderer, x, y, newX, newY);
   SDL_Delay(MILLISECONDDELAY);
   Neill_SDL_UpdateScreen(p->sw);
   p->squirt.xcoord = newX;
   p->squirt.ycoord = newY;
}

double getNewX(double distance, turtle t){
   /*distance * sin(angle)*/
   double newX;
   newX = distance * cos(t.angle) + t.xcoord;
   return newX;
}

double getNewY(double distance, turtle t){
   /*distance * cos(angle)*/
   double newY;
   newY = distance * sin(t.angle) + t.ycoord;
   return newY;
}

double getNewAngle(double oldAng, double rotation, bool right){
   double newAng;
   rotation = rotation * DEGTORAD;
   if (right == true){
      newAng = oldAng - rotation;
   }
   else{
      newAng = oldAng + rotation;
   }
   return newAng;
}

bool ruleDo(program *p){
   loop doLoop;
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Null DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVar(p) == false){
      return p->valid;
   }
   doLoop.varIndex = getAlphaIndex(p->code->current->word[0]);
   if (ruleDoInfo(p, &doLoop) == false){
      return p->valid;
   }
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Expected { in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (!STREQ(p->code->current->word,"{")){
      return setProgError(p, "Error: Expected { in DO instruction.");
   }
   p->code->current = p->code->current->next;
   return ruleDoLoop(p, doLoop);
}

bool ruleDoInfo(program *p, loop *doLoop){
   if (ruleDoFrom(p, doLoop) == false){
      return p->valid;
   }
   if (ruleDoTo(p, doLoop) == false){
      return p->valid;
   }
   return p->valid;
}

bool ruleDoFrom(program *p, loop *doLoop){
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Expected FROM in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (!STREQ(p->code->current->word, "FROM")){
      return setProgError(p, "Error: Expected FROM in DO instruction.");
   }
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Expected VARNUM in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVarnum(p) == false){
      return p->valid;
   }
   doLoop->from = getValue(p);
   p->vars[doLoop->varIndex] = doLoop->from;
   return p->valid;
}

bool ruleDoTo(program *p, loop *doLoop){
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Expected TO in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (!STREQ(p->code->current->word, "TO")){
      return setProgError(p, "Error: Expected TO in DO instruction.");
   }
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Expected VARNUM in DO instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVarnum(p) == false){
      return p->valid;
   }
   doLoop->to = getValue(p);
   return p->valid;
}

bool ruleDoLoop(program *p, loop doLoop){
   lexeme *word;
   word = p->code->current;
   do{
      p->code->current = word;
      if (ruleInstrctList(p) == false){
         return p->valid;
      }
   } while(p->vars[doLoop.varIndex]++ < doLoop.to);
   return p->valid;
}

bool ruleSet(program *p){
   int alphaIndex;
   double result;
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Null SET instruction.");
   }
   p->code->current = p->code->current->next;
   if (ruleVar(p) == false){
      return p->valid;
   }
   alphaIndex = getAlphaIndex(p->code->current->word[0]);
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Expected := in SET instruction.");
   }
   p->code->current = p->code->current->next;
   if (!STREQ(p->code->current->word, ":=")){
      return setProgError(p, "Error: Expected := in SET instruction.");
   }
   if (rulePolish(p) == false){
      return p->valid;
   }
   if (p->sw != NULL){
      if (stack_pop(p->polish, &result) == false){
         return setProgError(p, "Error: Attempted to use SET with null value.");
      }
      p->vars[alphaIndex] = result;
      if (stack_peek(p->polish, &result) == true){
         return setProgError(p, "Error: Incorrect POLISH notation.");
      }
   }
   return p->valid;
}

bool rulePolish(program *p){
   double value;
   if (p->code->current->next == NULL){
      return setProgError(p, "Error: Null POLISH instruction.");
   }
   p->code->current = p->code->current->next;
   if (STREQ(p->code->current->word, ";")){
      return p->valid;
   }
   if (strspn(p->code->current->word, OPCHARS) > 0){
      if (ruleOp(p) == false){
         return p->valid;
      }
      return rulePolish(p);
   }
   else if (ruleVarnum(p) == false){
      return p->valid;
   }
   value = getValue(p);
   stack_push(p->polish, value);
   return rulePolish(p);
}

bool ruleOp(program *p){
   double v1, v2;
   if (strlen(p->code->current->word) > 1){
      return setProgError(p, "Error: OP is more than one character.");
   }
   if (p->sw != NULL){
      if ((stack_pop(p->polish, &v2) == false)
         || (stack_pop(p->polish, &v1) == false)){
         return setProgError(p, "Error: OP operated on a non-existant number.");
      }
      switch(p->code->current->word[0]){
         case '+':
            stack_push(p->polish, (v1 + v2));
            break;
         case '-':
            stack_push(p->polish, (v1 - v2));
            break;
         case '/':
            stack_push(p->polish, (v1 / v2));
            break;
         case '*':
            stack_push(p->polish, (v1 * v2));
            stack_peek(p->polish, &v1);
            break;
         default:
            return setProgError(p, "Error: OP used an invalid operator.");
      }
}
   return p->valid;
}

bool ruleVarnum(program *p){
   if (strspn(p->code->current->word, DIGITS)
      == strlen(p->code->current->word)){
      return p->valid;
   }
   if (strspn(p->code->current->word, NUMCHARS)
      == strlen(p->code->current->word)){
      if (charFrequency(p->code->current->word, '-') > 0){
         if ((charFrequency(p->code->current->word, '-') > 1)
            || (p->code->current->word[0] != '-')){
            return setProgError(p, "Error: VARNUM contains invalid characters.");
         }
      }
      if (charFrequency(p->code->current->word, '.') > 1){
         return setProgError(p, "Error: VARNUM contains invalid characters.");
      }
      return p->valid;
   }
   return ruleVar(p);
}

int charFrequency(char *str, char c){
   int i = 0, count = 0;
   while (i < (int) strlen(str)){
      if (str[i] == c){
         count++;
      }
      i++;
   }
   return count;
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

double getValue(program *p){
   double value;
   if (strspn(p->code->current->word, NUMCHARS)
      == strlen(p->code->current->word)){
      value = atof(p->code->current->word);
   }
   else{
      value = p->vars[getAlphaIndex(p->code->current->word[0])];
   }
   return value;
}

bool setProgError(program *p, char *message){
   char fullError[ERRORBUFFER];
   p->valid = false;
   sprintf(fullError,"%s Issue encountered at word %d: %s.\n",
      message, p->code->current->index, p->code->current->word);
   p->errMessage = (char *)smartCalloc(strlen(fullError) + 1,sizeof(char));
   strcpy(p->errMessage,fullError);
   if (p->sw != NULL){
      p->sw->finished = 1;
   }
   return false;
}

int getAlphaIndex(char c){
   return (c - 'A');
}

program *createProgram(){
   program *p;
   sequence *seq;
   stack *s;
   int i;
   p = (program *)smartCalloc(1,sizeof(program));
   seq = createSequence();
   s = stack_init();
   p->code = seq;
   p->length = 0;
   p->valid = true;
   p->squirt.xcoord = WWIDTH / 2;
   p->squirt.ycoord = WHEIGHT / 2;
   p->squirt.angle = FACENORTH * DEGTORAD;
   p->polish = s;
   /*initialise all vars to zero*/
   for (i = 0; i < ALPHANUM; i++){
      p->vars[i] = 0;
   }
   return p;
}

sequence *createSequence(){
   sequence *s;
   s = (sequence *)smartCalloc(1, sizeof(sequence));
   return s;
}

lexeme *createLexeme(char *word){
   lexeme *lex;
   lex = (lexeme *)smartCalloc(1,sizeof(lexeme));
   lex->word = (char *)smartCalloc(strlen(word) + 1, sizeof(char));
   lex->word = strcpy(lex->word,word);
   return lex;
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

void freeProgram(program *p){
   if (p->valid == false){
      free(p->errMessage);
   }
   freeSequence(p->code);
   stack_free(p->polish);
   free(p);
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

void freeLexeme(lexeme *lex){
   free(lex->word);
   free(lex);
}

void testInterp(){
   program *p;
   double distance, x1, y1, angle;
   p = createProgram();

   /*Test getting new coordinates*/
   addLexeme(p, createLexeme("30"));
   distance = getValue(p);
   assert(fabs(distance - 30.0) < 0.0001);
   assert(getAlphaIndex('A') == 0);
   assert(getAlphaIndex('B') == 1);
   assert(getAlphaIndex('Z') == 25);
   p->vars[0] = 20.0;
   addLexeme(p, createLexeme("A"));
   distance = getValue(p);
   assert(fabs(distance - 20.0) < 0.0001);
   x1 = getNewX(distance, p->squirt);
   y1 = getNewY(distance, p->squirt);
   assert(fabs(x1 - (WWIDTH / 2)) < 0.0001);
   assert(fabs(y1 - ((WHEIGHT / 2) + 20.0)) < 0.0001);
   p->squirt.angle = 0;
   x1 = getNewX(distance, p->squirt);
   y1 = getNewY(distance, p->squirt);
   assert(fabs(x1 - ((WWIDTH / 2) + 20.0)) < 0.0001);
   assert(fabs(y1 - (WHEIGHT / 2)) < 0.0001);
   p->squirt.angle = M_PI;
   x1 = getNewX(distance, p->squirt);
   y1 = getNewY(distance, p->squirt);
   assert(fabs(x1 - ((WWIDTH / 2) - 20)) < 0.0001);
   assert(fabs(y1 - (WHEIGHT / 2)) < 0.0001);

   /*test angle conversions are correct*/
   p->squirt.angle = 90 * DEGTORAD;
   angle = getValue(p);
   assert(fabs(angle - 20.0) < 0.0001);
   angle = getNewAngle(p->squirt.angle, angle, true);
   assert(fabs(angle - (70 * DEGTORAD)) < 0.0001);
   addLexeme(p, createLexeme("50"));
   p->squirt.angle = 70 * DEGTORAD;
   angle = getValue(p);
   angle = getNewAngle(p->squirt.angle, angle, false);
   assert(fabs(angle - (120 * DEGTORAD)) < 0.0001);
   freeProgram(p);
}

void testParse(){
   lexeme *lex0, *lex1, *lex2, *lex3, *lex4, *lex5, *lex6, *lex7, *lex8;
   sequence *seq1;
   program *prog1;
   char errorMessage[200], *callocTest;

   callocTest = smartCalloc(30, sizeof(char));
   assert(callocTest != NULL);

   assert(charFrequency("hello", 'l') == 2);
   assert(charFrequency("elephants", 'p') == 1);
   assert(charFrequency("octopodes", 'o') == 3);
   assert(charFrequency("zack", 'h') == 0);

   /*struct create testing*/
   lex1 = createLexeme("test");
   seq1 = createSequence();
   prog1 = createProgram();
   assert(lex1 != NULL);
   assert(STREQ(lex1->word, "test"));
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

   /*test adding lexemes*/
   assert(addLexeme(NULL,lex1) == false);
   assert(addLexeme(prog1,NULL) == false);
   assert(addLexeme(prog1,lex1) == true);
   assert(STREQ(prog1->code->start->word,lex1->word));
   assert(STREQ(prog1->code->current->word,lex1->word));
   assert(prog1->length == 1);
   assert(lex1->index == 1);
   assert(prog1->code->start->index == prog1->length);
   lex2 = createLexeme("test2");
   assert(STREQ(prog1->code->start->word, lex1->word));
   assert(addLexeme(prog1,lex2) == true);
   assert(STREQ(prog1->code->current->word, lex2->word));
   assert(STREQ(prog1->code->start->next->word, lex2->word));
   assert(STREQ(prog1->code->current->prev->word, lex1->word));
   assert(prog1->length == 2);
   assert(lex2->index == 2);
   assert(prog1->code->current->index == prog1->length);

   prog1->code->current = prog1->code->start;
   assert(setProgError(prog1,"test error") == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "test error Issue encountered at word 1: "
      "test.\n"));
   prog1->valid = true;
   free(prog1->errMessage);

   /*Test '{' and '}' start and end conditions for parser*/
   assert(ruleMain(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage,
         "Error: Program did not start with {. Issue encountered at word 1: "
         "test.\n"));
   freeProgram(prog1);
   prog1 = createProgram();
   lex3 = createLexeme("{");
   addLexeme(prog1, lex3);
   assert(ruleMain(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage,
         "Error: Program did not end with }. Issue encountered at word 1: "
         "{.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   lex5 = createLexeme("}");
   addLexeme(prog1,lex5);
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
   assert(STREQ(prog1->errMessage, "Error: VAR is too many characters. "
      "Issue encountered at word 7: TEST.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   assert(ruleVarnum(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: VAR is too many characters. "
      "Issue encountered at word 7: TEST.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, lex8);
   assert(ruleVar(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: VAR is an unexpected character. "
      "Issue encountered at word 8: }.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   assert(ruleVarnum(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: VAR is an unexpected character. "
      "Issue encountered at word 8: }.\n"));
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
   assert(STREQ(prog1->errMessage, "Error: No proper instruction found. "
      "Issue encountered at word 9: ABC.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   prog1->code->current = lex8->prev;
   assert(ruleInstruction(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: VAR is too many characters. "
      "Issue encountered at word 9: ABC.\n"));
   freeProgram(prog1);

   /*Test Polish and OP*/
   prog1 = createProgram();
   lex0 = createLexeme("POLISH");
   addLexeme(prog1, lex0);
   assert(rulePolish(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Null POLISH instruction. "
      "Issue encountered at word 1: POLISH.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   lex1 = createLexeme("+");
   addLexeme(prog1, lex1);
   assert(ruleOp(prog1) == true);
   prog1->code->current = lex1->prev;
   assert(rulePolish(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Null POLISH instruction. "
      "Issue encountered at word 2: +.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   lex2 = createLexeme(";");
   addLexeme(prog1,lex2);
   prog1->code->current = lex2->prev->prev;
   assert(rulePolish(prog1) == true);
   lex3 = createLexeme("++");
   addLexeme(prog1, lex3);
   assert(ruleOp(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: OP is more than one character. "
      "Issue encountered at word 4: ++.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   prog1->code->current = lex3->prev;
   assert(rulePolish(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: OP is more than one character. "
      "Issue encountered at word 4: ++.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   lex4 = createLexeme("A");
   lex5 = createLexeme(";");
   addLexeme(prog1, lex4);
   addLexeme(prog1, lex5);
   prog1->code->current = lex5->prev->prev;
   assert(rulePolish(prog1) == true);
   lex6 = createLexeme("AA");
   addLexeme(prog1, lex6);
   prog1->code->current = lex6->prev;
   assert(rulePolish(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: VAR is too many characters. "
      "Issue encountered at word 7: AA.\n"));
   freeProgram(prog1);

   /*Test Set*/
   prog1 = createProgram();
   addLexeme(prog1, createLexeme("SET"));
   assert(ruleSet(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Null SET instruction. "
      "Issue encountered at word 1: SET.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("A"));
   addLexeme(prog1, createLexeme(":="));
   assert(ruleSet(prog1) == false);
   assert(prog1->valid == false);
   assert(strstr(prog1->errMessage, "Error: Expected := in SET instruction.") == NULL);
   freeProgram(prog1);
   prog1 = createProgram();
   addLexeme(prog1, createLexeme("SET"));
   addLexeme(prog1, createLexeme("A"));
   prog1->code->current = prog1->code->current->prev;
   assert(ruleSet(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected := in SET instruction. "
      "Issue encountered at word 2: A.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("A"));
   prog1->code->current = prog1->code->current->prev->prev;
   assert(ruleSet(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected := in SET instruction. "
      "Issue encountered at word 3: A.\n"));
   freeProgram(prog1);
   prog1 = createProgram();
   addLexeme(prog1, createLexeme("SET"));
   addLexeme(prog1, createLexeme("A"));
   addLexeme(prog1, createLexeme(":="));
   addLexeme(prog1, createLexeme(";"));
   prog1->code->current = prog1->code->current->prev->prev->prev;
   assert(ruleSet(prog1) == true);
   freeProgram(prog1);

   /*Test Do*/
   prog1 = createProgram();
   addLexeme(prog1, createLexeme("DO"));
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Null DO instruction. "
      "Issue encountered at word 1: DO.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("A"));
   prog1->code->current = prog1->code->current->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected FROM in DO instruction. "
      "Issue encountered at word 2: A.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("FRO"));
   prog1->code->current = prog1->code->current->prev->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected FROM in DO instruction. "
      "Issue encountered at word 3: FRO.\n"));
   freeProgram(prog1);
   prog1 = createProgram();
   addLexeme(prog1, createLexeme("DO"));
   addLexeme(prog1, createLexeme("A"));
   addLexeme(prog1, createLexeme("FROM"));
   prog1->code->current = prog1->code->current->prev->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected VARNUM in DO instruction. "
      "Issue encountered at word 3: FROM.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("1"));
   prog1->code->current = prog1->code->current->prev->prev->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected TO in DO instruction. "
      "Issue encountered at word 4: 1.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("TOT"));
   prog1->code->current = prog1->code->current->prev->prev->prev->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected TO in DO instruction. "
      "Issue encountered at word 5: TOT.\n"));
   freeProgram(prog1);
   prog1 = createProgram();
   addLexeme(prog1, createLexeme("DO"));
   addLexeme(prog1, createLexeme("A"));
   addLexeme(prog1, createLexeme("FROM"));
   addLexeme(prog1, createLexeme("1"));
   addLexeme(prog1, createLexeme("TO"));
   prog1->code->current = prog1->code->current->prev->prev->prev->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected VARNUM in DO instruction. "
      "Issue encountered at word 5: TO.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("5"));
   prog1->code->current = prog1->code->current->prev->prev->prev->prev->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected { in DO instruction. "
      "Issue encountered at word 6: 5.\n"));
   prog1->valid = true;
   free(prog1->errMessage);
   addLexeme(prog1, createLexeme("{a"));
   prog1->code->current = prog1->code->current->prev->prev->prev->prev->prev->prev;
   assert(ruleDo(prog1) == false);
   assert(prog1->valid == false);
   assert(STREQ(prog1->errMessage, "Error: Expected { in DO instruction. "
      "Issue encountered at word 7: {a.\n"));

   /*More realistic testing.*/
   assert(testProgram("{ }", errorMessage));
   assert(!testProgram("{}", errorMessage));
   assert(STREQ("Error: Program did not start with {. Issue encountered at word 1: {}.\n", errorMessage));
   assert(!testProgram("{", errorMessage));
   assert(STREQ("Error: Program did not end with }. Issue encountered at word 1: {.\n", errorMessage));
   assert(!testProgram("FD 30", errorMessage));
   assert(STREQ("Error: Program did not start with {. Issue encountered at word 1: FD.\n", errorMessage));
   assert(!testProgram("{ FD 30", errorMessage));
   assert(STREQ("Error: Program did not end with }. Issue encountered at word 3: 30.\n", errorMessage));
   assert(!testProgram("{ FD }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 3: }.\n", errorMessage));
   assert(testProgram("{ FD 3.0 }", errorMessage));
   assert(testProgram("{ FD -30 }", errorMessage));
   assert(!testProgram("{ FD 3-0 }", errorMessage));
   assert(STREQ("Error: VARNUM contains invalid characters. Issue encountered at word 3: 3-0.\n", errorMessage));
   assert(!testProgram("{ FD 3a0 }", errorMessage));
   assert(STREQ("Error: VAR is too many characters. Issue encountered at word 3: 3a0.\n", errorMessage));
   assert(testProgram("{ FD A }", errorMessage));
   assert(!testProgram("{ FD AB }", errorMessage));
   assert(STREQ("Error: VAR is too many characters. Issue encountered at word 3: AB.\n", errorMessage));
   assert(!testProgram("{ FD 30 A }", errorMessage));
   assert(STREQ("Error: No proper instruction found. Issue encountered at word 4: A.\n", errorMessage));
   assert(testProgram("{ FD 30 }", errorMessage));
   assert(!testProgram("RT 30", errorMessage));
   assert(STREQ("Error: Program did not start with {. Issue encountered at word 1: RT.\n", errorMessage));
   assert(!testProgram("{ RT 30", errorMessage));
   assert(STREQ("Error: Program did not end with }. Issue encountered at word 3: 30.\n", errorMessage));
   assert(testProgram("{ RT 30 }", errorMessage));
   assert(!testProgram("LT 30", errorMessage));
   assert(STREQ("Error: Program did not start with {. Issue encountered at word 1: LT.\n", errorMessage));
   assert(!testProgram("{ LT 30", errorMessage));
   assert(STREQ("Error: Program did not end with }. Issue encountered at word 3: 30.\n", errorMessage));
   assert(testProgram("{ LT 30 }", errorMessage));
   assert(!testProgram("{ FD 30 RaT 30 LT 30 }", errorMessage));
   assert(STREQ("Error: No proper instruction found. Issue encountered at word 4: RaT.\n", errorMessage));
   assert(!testProgram("{ F 30 RT 30 LT 30 }", errorMessage));
   assert(STREQ("Error: No proper instruction found. Issue encountered at word 2: F.\n", errorMessage));
   assert(testProgram("{ FD 30 RT 30 LT 30 }", errorMessage));
   assert(!testProgram("{ FD 30 RT 3.0 LT 3..0 }", errorMessage));
   assert(STREQ("Error: VARNUM contains invalid characters. Issue encountered at word 7: 3..0.\n", errorMessage));
   assert(!testProgram("{ FD 30 RaT 3..0 LT 30 }", errorMessage));
   assert(STREQ("Error: No proper instruction found. Issue encountered at word 4: RaT.\n", errorMessage));
   assert(!testProgram("{ SET }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 3: }.\n", errorMessage));
   assert(!testProgram("{ SET AB }", errorMessage));
   assert(STREQ("Error: VAR is too many characters. Issue encountered at word 3: AB.\n", errorMessage));
   assert(!testProgram("{ SET 1 }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 3: 1.\n", errorMessage));
   assert(!testProgram("{ SET A }", errorMessage));
   assert(STREQ("Error: Expected := in SET instruction. Issue encountered at word 4: }.\n", errorMessage));
   assert(!testProgram("{ SET A : 1 ; }", errorMessage));
   assert(STREQ("Error: Expected := in SET instruction. Issue encountered at word 4: :.\n", errorMessage));
   assert(!testProgram("{ SET A := }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 5: }.\n", errorMessage));
   assert(!testProgram("{ SET A := 1 }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 6: }.\n", errorMessage));
   assert(!testProgram("{ SET A := 1 ++ ; }", errorMessage));
   assert(STREQ("Error: OP is more than one character. Issue encountered at word 6: ++.\n", errorMessage));
   assert(!testProgram("{ SET A := 1 & ; }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 6: &.\n", errorMessage));
   assert(testProgram("{ SET A := 1 1 1.1 + + + + ; }", errorMessage));
   assert(testProgram("{ SET A := 1 + + ; }", errorMessage));
   assert(!testProgram("{ SET A := 1; }", errorMessage));
   assert(STREQ("Error: VAR is too many characters. Issue encountered at word 5: 1;.\n", errorMessage));
   assert(testProgram("{ SET A := 1 ; }", errorMessage));
   assert(!testProgram("{ DO }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 3: }.\n", errorMessage));
   assert(!testProgram("{ DO { } }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 3: {.\n", errorMessage));
   assert(!testProgram("{ DO VAR }", errorMessage));
   assert(STREQ("Error: VAR is too many characters. Issue encountered at word 3: VAR.\n", errorMessage));
   assert(!testProgram("{ DO A }", errorMessage));
   assert(STREQ("Error: Expected FROM in DO instruction. Issue encountered at word 4: }.\n", errorMessage));
   assert(!testProgram("{ DO A FROM }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 5: }.\n", errorMessage));
   assert(!testProgram("{ DO A FRO }", errorMessage));
   assert(STREQ("Error: Expected FROM in DO instruction. Issue encountered at word 4: FRO.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X }", errorMessage));
   assert(STREQ("Error: Expected TO in DO instruction. Issue encountered at word 6: }.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X T }", errorMessage));
   assert(STREQ("Error: Expected TO in DO instruction. Issue encountered at word 6: T.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X TO }", errorMessage));
   assert(STREQ("Error: VAR is an unexpected character. Issue encountered at word 7: }.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X TO X }", errorMessage));
   assert(STREQ("Error: Expected { in DO instruction. Issue encountered at word 8: }.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X TO X FD 30 }", errorMessage));
   assert(STREQ("Error: Expected { in DO instruction. Issue encountered at word 8: FD.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X TO X { FD 30 }", errorMessage));
   assert(STREQ("Error: Program did not end with }. Issue encountered at word 11: }.\n", errorMessage));
   assert(testProgram("{ DO A FROM X TO X { FD 30 } }", errorMessage));
   assert(testProgram("{ DO A FROM X TO X { FD 30 FD 30 FD 30 } }", errorMessage));
   assert(testProgram("{ DO A FROM X TO X { FD 30 RT 30 LT 30 } }", errorMessage));
   assert(!testProgram("{ SET ", errorMessage));
   assert(STREQ("Error: Null SET instruction. Issue encountered at word 2: SET.\n", errorMessage));
   assert(!testProgram("{ SET A ", errorMessage));
   assert(STREQ("Error: Expected := in SET instruction. Issue encountered at word 3: A.\n", errorMessage));
   assert(!testProgram("{ SET A := ", errorMessage));
   assert(STREQ("Error: Null POLISH instruction. Issue encountered at word 4: :=.\n", errorMessage));
   assert(!testProgram("{ SET A := 1 ", errorMessage));
   assert(STREQ("Error: Null POLISH instruction. Issue encountered at word 5: 1.\n", errorMessage));
   assert(!testProgram("{ DO ", errorMessage));
   assert(STREQ("Error: Null DO instruction. Issue encountered at word 2: DO.\n", errorMessage));
   assert(!testProgram("{ DO A ", errorMessage));
   assert(STREQ("Error: Expected FROM in DO instruction. Issue encountered at word 3: A.\n", errorMessage));
   assert(!testProgram("{ DO A FROM ", errorMessage));
   assert(STREQ("Error: Expected VARNUM in DO instruction. Issue encountered at word 4: FROM.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X ", errorMessage));
   assert(STREQ("Error: Expected TO in DO instruction. Issue encountered at word 5: X.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X TO ", errorMessage));
   assert(STREQ("Error: Expected VARNUM in DO instruction. Issue encountered at word 6: TO.\n", errorMessage));
   assert(!testProgram("{ DO A FROM X TO X ", errorMessage));
   assert(STREQ("Error: Expected { in DO instruction. Issue encountered at word 7: X.\n", errorMessage));
   
   free(callocTest);
   free(seq1);
   freeProgram(prog1);
}

bool testProgram(char *progText, char *errorMessage){
   program *p;
   bool fileValid;
   char *token, text[200];
   strcpy(text, progText);
   p = createProgram();
   token = strtok(text, WHITESPACE);
   while (token != NULL){
      if (addLexeme(p, createLexeme(token)) == false){
         errorQuit("Couldn't add lexeme... exiting");
      }
      token = strtok(NULL, WHITESPACE);
   }
   ruleMain(p);
   if (p->valid == false){
      strcpy(errorMessage, p->errMessage);
   }
   fileValid = p->valid;
   freeProgram(p);
   return fileValid;
}
