REM ********************************************************************
REM **    NIALL  Non-Intelligent AMOS Language Learner Version 1.0    **
REM **   ==========================================================   **
REM **                Written By Matthew Peck in 1990                 **
REM ********************************************************************

DECLARE SUB SETUP ()
DECLARE SUB COMMANDS ()
DECLARE SUB USERINPUT ()
DECLARE SUB NIALLREPLIES ()
DECLARE SUB SEEINSTRUCTIONS ()
DECLARE SUB ANALYSESENTENCE ()

COMMON SHARED /all/ comm, T$, WORD$(), NUM, W$(), W, HARD, CO(), MNW

MNW = 3000

DIM WORD$(MNW, 1), W$(255), CO(1)

WORD$(0, 0) = " ": WORD$(MNW, 0) = "."

RANDOMIZE (TIMER + VAL(RIGHT$(TIME$, 2)))

SETUP

DO
   USERINPUT
   IF comm = 1 THEN GOTO SKIPIT
   NIALLREPLIES

SKIPIT:

LOOP


DATA "                               COMMAND WORDS"
DATA " "
DATA " When you are given the prompt USER: you will normally type in  a  sentence,"
DATA "but you can also type in a command word.    All command words  are  prefixed"
DATA "by a '#'.  This version (1.0) can support the following commands."
DATA " "
DATA "#LIST        Displays Nialls current dictionary on the screen."
DATA " "
DATA "#LLIST       Outputs the dictionary to a printer."
DATA " "
DATA "#LPRINT      Toggles direct output to printer."
DATA " "
DATA "#SAVE        Saves the current dictionary to disc under any name."
DATA " "
DATA "#LOAD        Loads a previously saved dictionary from disc."
DATA " "
DATA "#CORRECT     Corrects a mispelt word in the dictionary."
DATA " "
DATA "#FRESH       Clears the current dictionary."
DATA " "
DATA "#QUIT        Leaves the program."
DATA " "
DATA " "
DATA " "
DATA "                                PRESS A KEY"
DATA "                                  CREDITS"
DATA " "
DATA "         Written by Matthew Peck in AMOS Version 1.1 in August 1990"
DATA "  The original idea for NIALL was adapted from a program called DANI, which"
DATA "was written for the MSX machines and published in a magazine,MSX Computing."
DATA "NIALL is a complete re-write but I would like to give credit to the  author"
DATA "of DANI for the inspiration.  However, I no longer have the magazine and do"
DATA "not know the author's name. If you wrote DANI then contact me so that I can"
DATA "place your name in the credit list."
DATA " "
DATA "  Many thanks go to the following people for their help, suggestions etc."
DATA "Adam Wright................................For waiting for me to code this."
DATA "Pat Fox and Julian Brown.....For playtesting my BBC basic version to death."
DATA "Kevin Tye...................................For being a generally nice guy."
DATA "Gary Keogh...............For being a great room-mate and friend at college."
DATA "Francios Lionet........................For AMOS, what else, and finally to "
DATA "Sarah Gould...........For being such a caring and understanding girlfriend."
DATA " "
DATA "My address is:"
DATA "      23 Welbeck Avenue, Bedgrove, Aylesbury, Bucks, ENGLAND, HP21 9BH."
DATA " Contact me with/for ideas, programs, NIALL dictionaries and NIALL updates."
DATA " "
DATA "                                PRESS A KEY"
DATA " "
DATA "                        AMOS (c) 1990 Mandarin/Jawx"

SUB ANALYSESENTENCE
  
   REM --- Loop for the number of words +1 ---
   OLD = 0
  
   FOR H = 1 TO W + 1
      IF H = W + 1 THEN F = MNW: GOTO FOUNDIT
      FOR F = 1 TO NUM
          IF W$(H) = WORD$(F, 0) THEN GOTO FOUNDIT
      NEXT F
      REM --- The word is a new one ---
      NUM = NUM + 1
      WORD$(NUM, 0) = W$(H)
      F = NUM
FOUNDIT:
      IF WORD$(OLD, 1) = "" THEN WORD$(OLD, 1) = "1|" + STR$(F) + "(1)": GOTO ALLDONE
      C$ = WORD$(OLD, 1)
      C$ = LTRIM$(STR$(VAL(C$) + 1)) + RIGHT$(C$, LEN(C$) - INSTR(C$, "|") + 1)
      FOR G = INSTR(C$, "|") + 1 TO LEN(C$)
         IF VAL(RIGHT$(C$, LEN(C$) - G)) = F THEN GOTO HERE
         G = INSTR(G, C$, ")")
      NEXT G
      C$ = C$ + STR$(F) + "(1)"
      WORD$(OLD, 1) = C$
      GOTO ALLDONE
HERE:
      G = INSTR(G, C$, "("): I = INSTR(G, C$, ")") - 1
      C$ = LEFT$(C$, G) + LTRIM$(STR$(VAL(RIGHT$(C$, LEN(C$) - G)) + 1)) + RIGHT$(C$, LEN(C$) - I)
      WORD$(OLD, 1) = C$

ALLDONE:
      OLD = F
   NEXT H

END SUB

SUB COMMANDS
   IF T$ = "#list" THEN
      FOR F = 0 TO NUM STEP 20
         FOR G = 0 TO 19
               IF F + G <= NUM THEN PRINT F + G, WORD$(F + G, 0), WORD$(F + G, 1)
         NEXT G
         PRINT : PRINT "PRESS A KEY"
         INPUT an$
      NEXT F: EXIT SUB
   END IF
   IF T$ = "#llist" THEN
      FOR F = 0 TO NUM: LPRINT F, WORD$(F, 0), WORD$(F, 1): NEXT F
   END IF
   IF T$ = "#save" THEN
      INPUT "Enter File Name: "; A$
      IF A$ = "" THEN A$ = "niall.dat"
      OPEN A$ FOR OUTPUT AS #1
      PRINT #1, NUM
      x = CSRLIN
      Y = POS(x)
      FOR F = 0 TO NUM
          LOCATE x, Y: PRINT "Saveing:"; F; "  ";
          PRINT #1, RTRIM$(WORD$(F, 0)): PRINT #1, LTRIM$(WORD$(F, 1))
      NEXT F
      CLOSE #1
   END IF
   IF T$ = "#load" THEN
      INPUT "Enter File Name: "; A$
      IF A$ = "" THEN A$ = "niall.dat"
      OPEN A$ FOR INPUT AS #1
      INPUT #1, NUM
      x = CSRLIN
      Y = POS(x)
      FOR F = 0 TO NUM
          LOCATE x, Y: PRINT "Countdown:"; NUM - F; "  ";
          INPUT #1, WORD$(F, 0): INPUT #1, WORD$(F, 1)
      NEXT F
      CLOSE #1
      PRINT
   END IF
   IF T$ = "#correct" THEN
      PRINT "Enter incorrect word: "; : INPUT O$
      FOR F = 1 TO NUM
         IF LCASE$(O$) = WORD$(F, 0) THEN
            PRINT "Enter correct spelling: "; : INPUT N$
            WORD$(F, 0) = LCASE$(N$)
            EXIT FOR
         END IF
      NEXT F
      IF F > NUM THEN PRINT "Cannot find word "; O$
   END IF
   IF T$ = "#fresh" THEN
      PRINT "Are you sure (Y/N)": A$ = INPUT$(1): PRINT
      IF UCASE$(A$) = "Y" THEN
         FOR F = 0 TO NUM
             WORD$(F, 0) = ""
             WORD$(F, 1) = ""
         NEXT F
         NUM = 0
      END IF
   END IF
   IF T$ = "#quit" THEN
      PRINT "Do you really want to quit (Y/N)": A$ = INPUT$(1): PRINT
      IF UCASE$(A$) = "Y" THEN
         CLS : END
      END IF
   END IF
   IF T$ = "#lprint" THEN
      IF HARD = 0 THEN
         HARD = 1
      ELSE
         HARD = 0
      END IF
   END IF
END SUB

SUB NIALLREPLIES
  
   IF NUM = 0 THEN PRINT "I cannot speak yet! Talk to me or #load a file.": EXIT SUB
   REP$ = "": C = 0
RLOOP:
   C$ = WORD$(C, 1): R = RND * VAL(C$) ' not happy with this yet
   'PRINT "(R="; R; " C$="; C$; " C="; C; ")"
   IF R = 0 THEN GOTO RLOOP
   FOR G = INSTR(C$, "(") TO LEN(C$)
      R = R - VAL(RIGHT$(C$, LEN(C$) - G))
      IF R <= 0 THEN GOTO THISWORD
      G = INSTR(G + 1, C$, "(") - 1
   NEXT G
   END
THISWORD:
   FOR F = G TO 1 STEP -1
      F$ = MID$(C$, F, 1): IF (F$ = ")") OR (F$ = "|") THEN GOTO WORD
   NEXT F
WORD:
   D = VAL(RIGHT$(C$, LEN(C$) - F))
   REP$ = REP$ + WORD$(C, 0) + " "
   C = D
   IF C <> MNW THEN GOTO RLOOP
   DO
      IF LEFT$(REP$, 1) <> " " THEN EXIT DO
      REP$ = RIGHT$(REP$, LEN(REP$) - 1)
   LOOP
   IF REP$ = "" THEN EXIT SUB
   'Left$(REP$,1)=Chr$(Asc(Left$(REP$,1))-32)
   'Right$(REP$,1)="."
   PRINT "NIALL:"; REP$
   IF HARD = 1 THEN LPRINT "NIALL:"; REP$

END SUB

SUB SEEINSTRUCTIONS
   CLS
   FOR F = 1 TO 2
      FOR G = 1 TO 25
          READ A$
          PRINT A$
      NEXT G
   INPUT an$
   NEXT F
   CLS
END SUB

SUB SETUP
   CLS
   PRINT "Welcome to NIALL": PRINT
   PRINT "Non Intelligent AMOS Language Learner": PRINT
   PRINT "Written in AMOS Basic Version 1.1": PRINT
   PRINT "by Matthew Peck in 1990": PRINT : PRINT
   PRINT "This program is fully Public Domain": PRINT : PRINT
   PRINT "Would you like further instructions (Y/N)": A$ = INPUT$(1): PRINT
   A$ = UCASE$(A$)
   IF A$ = "Y" THEN
      SEEINSTRUCTIONS
   END IF
   PRINT ".......... Get Typing ..........."
   PRINT
END SUB

SUB USERINPUT
   comm = 0
   PRINT
   PRINT "USER: "; : LINE INPUT T$
   IF HARD = 1 THEN LPRINT " USER:"; T$
   IF T$ = "" THEN EXIT SUB
   T$ = LCASE$(T$)
   REM --- Is it a command string ---
   IF LEFT$(T$, 1) = "#" THEN
      COMMANDS
      comm = 1
      EXIT SUB
   END IF
   REM --- Cuts out the rubbish ---
   R$ = ""
   FOR F = 1 TO LEN(T$)
      IF MID$(T$, F, 1) = "," THEN MID$(T$, F, 1) = "."
      IF INSTR("!~$%^&*()-+|}{@?></#\=" + CHR$(34), MID$(T$, F, 1)) = 0 THEN R$ = R$ + MID$(T$, F, 1)
   NEXT F
   T$ = R$
   REM --- Seperates into words ---
   IF RIGHT$(T$, 1) = "." THEN T$ = LEFT$(T$, LEN(T$) - 1)
   Q$ = T$
FULLSTOP:
   IF INSTR(Q$, ".") <> 0 THEN FS = 1: T$ = LEFT$(Q$, INSTR(Q$, ".") - 1): Q$ = RIGHT$(Q$, LEN(Q$) - INSTR(Q$, "."))
   W = 1: W$(W) = ""
   FOR F = 1 TO LEN(T$)
      IF MID$(T$, F, 1) = " " THEN IF MID$(T$, F + 1, 1) <> " " THEN W = W + 1: W$(W) = "": GOTO SKIP
      W$(W) = W$(W) + MID$(T$, F, 1)
SKIP:
   NEXT F
   IF NUM <> MNW - 1 THEN
      ANALYSESENTENCE
   END IF
   IF FS = 1 THEN FS = 0: T$ = Q$: GOTO FULLSTOP

END SUB

