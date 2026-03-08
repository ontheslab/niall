' ********************************************************************
' **    NIALL  Non-Intelligent AMOS Language Learner Version 1.0    **
' **   ==========================================================   **
' **                Written By Matthew Peck in 1990                 **
' ********************************************************************
' COMx: Remote / Local Sub-Programs
' Ver 1.20  By - Mark Casteleijn 1988/89/90/91 .

' Put string to print in a$ then `CALL IO'
' To get an answer use `CALL IOA' answer will be put in t$

' Ansi Colour control function
' Options :
'           ansi = 0  - No Ansi No local colour
'           ansi = 1  - Ansi on No Local colour
'           ansi = 2  - Ansi on Local colour on
'
' anc$ - contains the ansi codes to send out.
'
' eg.  ansi=2:CALL Colour(1, 2) This will set local colour to blue on green
'                               and put the ansi codes for blue on green
'                               in anc$ (ESC[0;34;42m)
'
' ansiloc x, y  works the same as locate x,y

REM $DYNAMIC

DECLARE SUB COMMANDS ()
DECLARE SUB USERINPUT ()
DECLARE SUB NIALLREPLIES ()
DECLARE SUB SEEINSTRUCTIONS ()
DECLARE SUB ANALYSESENTENCE ()

DECLARE SUB IO ()
DECLARE SUB IOA ()
DECLARE SUB Crunch (In$)
DECLARE SUB Colour (fg!, bg!)
DECLARE SUB Ansiloc (x!, y!)

DECLARE SUB getchr (keyout$)

COMMON SHARED /Colour/ anc$, ansi, mono
COMMON SHARED /ALL/ t$, a$, okp, r$, Online, ufname$, ulname$, defname$, sec
COMMON SHARED /ALL/ comm, tt$, WORD$(), NUM, W$(), W, MNW, board$, debug

MNW = 1000

ON ERROR GOTO Exiterror

DIM WORD$(MNW, 1), W$(255)

WORD$(0, 0) = " ": WORD$(MNW, 0) = "."

ansi = 2: r$ = CHR$(13): okp = 1: mono = 0: debug = 0

CLS

LOCATE 24, 1: PRINT STRING$(80, "-");

VIEW PRINT 1 TO 23

GOSUB lucon
  
IF ansi <> 0 AND Online <> 0 THEN a$ = CHR$(27) + "[2J": CALL IO
       
timeset = INT(timelimit * 60) - 30
ON TIMER(timeset) GOSUB Getoff
PRINT timeset

LOCATE 25, 1: PRINT "User: "; ufname$; " "; ulname$;
LOCATE 25, 45: PRINT "Words:";
LOCATE 3, 1

TIMER ON

RANDOMIZE INT(VAL(RIGHT$(TIME$, 2)) + timeset + VAL(RIGHT$(DATE$, 4)) + LEN(junk5$) + (VAL(LEFT$(DATE$, 2)) * 2) + ((VAL(RIGHT$(TIME$, 2)) * 5) / 2) + VAL(LEFT$(TIME$, 2)))

starttime = TIMER

Colour 15, 0

Crunch "Welcome to NIALL   (Non Intelligent AMOS Language Learner)" + r$
Crunch "Original Program Written in AMOS Basic Version 1.1 (Amiga)"
Crunch "by Matthew Peck in 1990"
Crunch "This program is fully public Domain" + r$
Crunch "This is a Test BBS Ver 1.10a (17/11/1993) By David Bowman." + r$

OPEN "niall.dat" FOR INPUT AS #2
a$ = "Loading - "
IO
x = CSRLIN
y = POS(x)
INPUT #2, NUM
FOR F = 0 TO NUM
    INPUT #2, WORD$(F, 0): INPUT #2, WORD$(F, 1)
    LOCATE x, y
    PRINT F;
NEXT F
CLOSE #2
LOCATE x, y, 1
Crunch LTRIM$(STR$(NUM)) + " Words Loaded." + r$

a$ = "Would you like further instructions [Please Read] (Y/N) "
IO
CALL getchr(an$)
IF an$ = "Y" THEN
   SEEINSTRUCTIONS
END IF

CLS 2
IF ansi <> 0 AND Online <> 0 THEN a$ = CHR$(27) + "[2J": CALL IO

Crunch r$ + "Enter (#quit) To Return To BBS (#?) For Commands" + r$
Crunch "You have " + LTRIM$(STR$(INT((timeset - (TIMER - starttime)) / 60))) + " minutes left online" + r$
Crunch ".......... Get Typing ..........." + r$

DO

   x = CSRLIN
   y = POS(x)
   LOCATE 25, 74: PRINT USING "###.##"; ((timeset - (TIMER - starttime)) / 60);
   COLOR 7, 0
   LOCATE 25, 51: PRINT NUM;
   COLOR 15, 0
   LOCATE x, y, 1
   USERINPUT
   IF comm = 1 THEN GOTO SKIPIT
   NIALLREPLIES

SKIPIT:

LOOP

GOTO QuitGame

Getoff:
        Crunch r$ + r$ + "Sorry. Times up! - Exit." + r$
        tt$ = "#quit"
        COMMANDS
        GOTO QuitGame
        RETURN

Exiterror:
        PRINT
        PRINT "Error ! - "; ERR
        PRINT "Please Yell at David Bowman."

QuitGame:
        FOR xx = 1 TO 1000: NEXT xx
        CLOSE
        END

lucon:
        test$ = UCASE$(COMMAND$)
        timelimit = 31

        IF INSTR(test$, "-M") > 0 THEN mono = 1
        IF INSTR(test$, "-DEBUG") > 0 THEN debug = 1
        Online = 1
        PRINT "Reading `DORINFO1.DEF' file."
        OPEN "dorinfo1.def" FOR INPUT AS #3
       
        LINE INPUT #3, board$
        LINE INPUT #3, sfname$
        LINE INPUT #3, slname$
        LINE INPUT #3, inport$
        LINE INPUT #3, inspeed$
        LINE INPUT #3, junk4$
        LINE INPUT #3, ufname$
        LINE INPUT #3, ulname$
        LINE INPUT #3, junk5$
        LINE INPUT #3, ansi$
        LINE INPUT #3, priv$
        LINE INPUT #3, onlinetime$
                  
        CLOSE #3
        timelimit = VAL(onlinetime$)
        IF timelimit > 2 THEN timelimit = timelimit - 1
        sec = VAL(priv$)

        IF ansi$ = "1" THEN ansi = 2 ELSE ansi = 0
       
        commport$ = RTRIM$(inport$)
        IF commport$ = "COM0" THEN Online = 0 ELSE Online = 1
       
        x = INSTR(LTRIM$(inspeed$), " ")
        commspeed$ = RTRIM$(LEFT$(inspeed$, x))

        IF INSTR(test$, "-P0") > 0 THEN commport$ = "COM1": Online = 1
        IF INSTR(test$, "-P1") > 0 THEN commport$ = "COM2": Online = 1
        IF INSTR(test$, "-B2400") > 0 THEN commspeed$ = "2400": Online = 1
        IF INSTR(test$, "-B1200") > 0 THEN commspeed$ = "1200": Online = 1
        IF INSTR(test$, "-B300") > 0 THEN commspeed$ = "300": Online = 1

        comset$ = RTRIM$(commport$) + ":" + commspeed$ + ",N,8,1,CS1000,DS1000,CD1000,LF"

        PRINT comset$; " - ";

        IF Online = 1 THEN OPEN comset$ FOR RANDOM AS #1 LEN = 512

        PRINT "Online ="; Online; " Time =";

        RETURN

REM $STATIC
SUB ANALYSESENTENCE
  
   ' --- Loop for the number of words +1 ---
   OLD = 0
  
   FOR H = 1 TO W + 1
      IF H = W + 1 THEN F = MNW: GOTO FOUNDIT
      FOR F = 1 TO NUM
          IF W$(H) = WORD$(F, 0) THEN GOTO FOUNDIT
      NEXT F
      ' --- The word is a new one ---
      NUM = NUM + 1
      WORD$(NUM, 0) = RTRIM$(W$(H))
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
      G = INSTR(G, C$, "("): i = INSTR(G, C$, ")") - 1
      C$ = LEFT$(C$, G) + LTRIM$(STR$(VAL(RIGHT$(C$, LEN(C$) - G)) + 1)) + RIGHT$(C$, LEN(C$) - i)
      WORD$(OLD, 1) = C$

ALLDONE:
      OLD = F
   NEXT H

END SUB

SUB Ansiloc (x, y)

        LOCATE x, y
       
        IF ansi <> 0 AND Online <> 0 THEN
           anc$ = CHR$(27) + "[" + LTRIM$(STR$(x)) + ";" + LTRIM$(STR$(y)) + "H"
           FOR j = 1 TO LEN(anc$)
               P$ = MID$(anc$, j, 1)
               PRINT #1, P$;
           NEXT j
        END IF
          
        P$ = "": anc$ = ""

END SUB

SUB Colour (fg, bg)

atr = 0: afg = 0: abg = 0

SELECT CASE fg

        CASE 0
             afg = 30      'BLACK

        CASE 1
             afg = 34      'BLUE

        CASE 2
             afg = 32      'GREEN

        CASE 3
             afg = 36      'CYAN

        CASE 4
             afg = 31      'RED

        CASE 5
             afg = 35      'MAGENTA

        CASE 6
             afg = 33      'BROWN

        CASE 7
             afg = 37      'WHITE

        CASE 8
             afg = 77      'GREY

        CASE 9
             afg = 34      'LT.-BLUE
             atr = 1

        CASE 10
             afg = 32      'LT.-GREEN
             atr = 1

        CASE 11
             afg = 36      'LT.-CYAN
             atr = 1

        CASE 12
             afg = 31      'LT.-RED
             atr = 1

        CASE 13
             afg = 35      'LT.-MAGENTA
             atr = 1

        CASE 14
             afg = 33      'YELLOW
             atr = 1

        CASE 15
             afg = 37      'HT-WHITE
             atr = 1

END SELECT

SELECT CASE bg

        CASE 0
             abg = 40      'BLACK

        CASE 1
             abg = 44      'BLUE

        CASE 2
             abg = 42      'GREEN

        CASE 3
             abg = 46      'CYAN

        CASE 4
             abg = 41      'RED

        CASE 5
             abg = 45      'MAGENTA

        CASE 6
             abg = 43      'BROWN

        CASE 7
             abg = 47      'WHITE

        CASE 8
             abg = 47      'GREY (SAME AS WHITE)

END SELECT

out$ = CHR$(27) + "[" + LTRIM$(STR$(atr)) + ";"

anc$ = out$ + LTRIM$(STR$(afg)) + ";" + LTRIM$(STR$(abg)) + "m"

IF ansi = 0 THEN anc$ = ""
IF ansi = 2 AND mono = 0 THEN COLOR fg, bg

IF Online = 0 THEN EXIT SUB
       
        FOR j = 1 TO LEN(anc$)
            P$ = MID$(anc$, j, 1)
            PRINT #1, P$;
        NEXT j
       
        P$ = "": anc$ = ""
      
END SUB

SUB COMMANDS
   IF tt$ = "#list" THEN
      FOR F = 0 TO NUM STEP 20
         FOR G = 0 TO 19
               IF F + G <= NUM THEN
                  Crunch LTRIM$(STR$(F + G)) + SPACE$(5 - LEN(LTRIM$(STR$(F + G)))) + WORD$(F + G, 0)
               END IF
         NEXT G
         GOSUB ganykey
      NEXT F
      EXIT SUB
   END IF
   IF tt$ = "#flist" THEN
      FOR F = 0 TO NUM STEP 20
         FOR G = 0 TO 19
               IF F + G <= NUM THEN
                  Crunch LTRIM$(STR$(F + G)) + SPACE$(5 - LEN(LTRIM$(STR$(F + G)))) + WORD$(F + G, 0) + "   " + WORD$(F + G, 1)
               END IF
         NEXT G
         GOSUB ganykey
      NEXT F
      EXIT SUB
   END IF
   IF tt$ = "#save" THEN
      TIMER STOP
      OPEN "niall.dat" FOR OUTPUT AS #2
      a$ = "Saving - "
      IO
      PRINT #2, NUM
      FOR F = 0 TO NUM
          PRINT #2, RTRIM$(WORD$(F, 0)): PRINT #2, LTRIM$(WORD$(F, 1))
      NEXT F
      CLOSE #2
      Crunch LTRIM$(STR$(NUM)) + " Words Saved."
      TIMER ON
   END IF
   IF tt$ = "#load" AND sec > 29999 THEN
      TIMER STOP
      OPEN "niall.dat" FOR INPUT AS #2
      Crunch r$ + "Loading"
      INPUT #2, NUM
      FOR F = 0 TO NUM
          INPUT #2, WORD$(F, 0): INPUT #2, WORD$(F, 1)
      NEXT F
      CLOSE #2
      TIMER ON
   END IF
   IF tt$ = "#correct" AND sec > 4999 THEN
      a$ = "Enter incorrect word: "
      IO
      IOA
      O$ = t$
      FOR F = 1 TO NUM
         IF LCASE$(O$) = WORD$(F, 0) THEN
            a$ = "Enter correct spelling: "
            IO
            IOA
            N$ = t$
            WORD$(F, 0) = LCASE$(N$)
            EXIT FOR
         END IF
      NEXT F
      IF F > NUM THEN Crunch "Cannot find word: " + CHR$(34) + O$ + CHR$(34)
   END IF
   IF tt$ = "#?" THEN
      SEEINSTRUCTIONS
   END IF
   IF tt$ = "#quit" THEN
      TIMER OFF
      OPEN "niall.dat" FOR OUTPUT AS #2
      a$ = "Saving - "
      IO
      PRINT #2, NUM
      FOR F = 0 TO NUM
          PRINT #2, RTRIM$(WORD$(F, 0)): PRINT #2, LTRIM$(WORD$(F, 1))
      NEXT F
      CLOSE #2
      Crunch LTRIM$(STR$(NUM)) + " Words Saved." + r$
      Crunch "Returning you to: " + board$ + r$
      FOR xx = 1 TO 2000: NEXT xx
      CLOSE
      END
   END IF
   EXIT SUB

ganykey:
   a$ = r$ + "PRESS ANY KEY `Q' - Quit "
   IO
   CALL getchr(an$)
   IF an$ = "Q" THEN EXIT SUB
   RETURN

END SUB

SUB Crunch (In$)

    a$ = In$ + r$

    IO

END SUB

SUB getchr (keyout$)

getkey:
    an$ = INKEY$
    IF Online = 1 THEN
       IF LOC(1) > 0 THEN an$ = INPUT$(1, 1)
    END IF
    IF an$ = "" THEN GOTO getkey
    IF an$ = CHR$(13) THEN an$ = ""
    Crunch UCASE$(an$)
    keyout$ = UCASE$(an$)
            
END SUB

SUB IO

Modem:
        IF Online = 0 THEN GOTO OtherModem
        FOR j = 1 TO LEN(a$)
            P$ = MID$(a$, j, 1)
            'IF P$ = CHR$(8) THEN PRINT CHR$(29); "_"; CHR$(29);  ELSE PRINT P$;
            'IF P$ = CHR$(8) THEN PRINT CHR$(29);  ELSE PRINT P$;
            IF P$ = CHR$(8) THEN PRINT CHR$(29); " "; CHR$(29);  ELSE PRINT P$;
            PRINT #1, P$;
        NEXT j
        a$ = "": P$ = "": anc$ = ""
       
        EXIT SUB

OtherModem:
        FOR j = 1 TO LEN(a$)
            P$ = MID$(a$, j, 1)
            'IF P$ = CHR$(8) THEN PRINT CHR$(29); "_"; CHR$(29);  ELSE PRINT P$;
            'IF P$ = CHR$(8) THEN PRINT CHR$(29);  ELSE PRINT P$;
            IF P$ = CHR$(8) THEN PRINT CHR$(29); " "; CHR$(29);  ELSE PRINT P$;
        NEXT j
        a$ = "": P$ = "": anc$ = ""

END SUB

SUB IOA

IOA:
telly = 0: t$ = "": t = 0: i$ = "": ch$ = "": alter = 0
IF Online = 0 THEN GOTO SomeIOA

WHILE 1 AND alter < 1
  WHILE LOC(1) <> 0
    ch$ = INPUT$(1, 1)
    equivs = ASC(ch$) AND 127: IF equivs <> 1 THEN ch$ = CHR$(equivs)
    IF ch$ = CHR$(10) OR ch$ = CHR$(13) OR ch$ = r$ THEN
       alter = 3: telly = 0
    END IF
    i$ = i$ + ch$: equivs = 0
    telly = telly + 1
    IF telly = 127 THEN ch$ = r$: telly = 0
    IF ch$ = CHR$(8) AND LEN(i$) >= 2 THEN i$ = LEFT$(i$, LEN(i$) - 2): telly = telly - 2
    a$ = ch$
    CALL IO
    ch$ = ""
  WEND
  ch$ = INKEY$: i$ = i$ + ch$
  a$ = ch$: CALL IO
  IF ch$ = CHR$(10) OR ch$ = CHR$(13) OR ch$ = r$ THEN telly = 0: alter = 3
  IF ch$ = CHR$(8) AND LEN(i$) >= 2 THEN i$ = LEFT$(i$, LEN(i$) - 2): telly = telly - 2
  ch$ = ""
WEND
IF i$ <> "" THEN GOTO MoreIOA
SomeIOA:
ch$ = INKEY$: i$ = i$ + ch$: a$ = ch$: CALL IO
IF ch$ = CHR$(10) OR ch$ = CHR$(13) OR ch$ = r$ THEN GOTO MoreIOA
IF ch$ = CHR$(8) AND LEN(i$) >= 2 THEN i$ = LEFT$(i$, LEN(i$) - 2)
ch$ = ""
GOTO SomeIOA
MoreIOA:
IF i$ = "" THEN ch$ = "": GOTO IOA
t$ = LEFT$(i$, LEN(i$) - 1)
IF LEN(t$) > 128 THEN t$ = LEFT$(t$, 128)

END SUB

SUB NIALLREPLIES
  
   IF NUM = 0 THEN
      Crunch "I cannot speak yet! Talk to me or `#load' a data file."
      EXIT SUB
   END IF
  
   REP$ = "": C = 0: count = 0

   IF debug THEN PRINT "Proccessing reply"

RLOOP:
   count = count + 1
   IF count > 999 THEN EXIT SUB 'To trap pos. cont. loop error
   C$ = WORD$(C, 1)
   r = RND * VAL(C$) ' not happy with this yet
'   PRINT "(R="; r; " C$="; C$; " C="; C; " Val="; VAL(C$); " count="; count; ")"
   IF debug THEN PRINT "(R="; r; " C="; C; " Val="; VAL(C$); " count="; count; ")"
   IF r = 0 THEN GOTO RLOOP
   FOR G = INSTR(C$, "(") TO LEN(C$)
      r = r - VAL(RIGHT$(C$, LEN(C$) - G))
      IF r <= 0 THEN GOTO THISWORD
      G = INSTR(G + 1, C$, "(") - 1
   NEXT G
   EXIT SUB 'END
THISWORD:
   FOR F = G TO 1 STEP -1
      F$ = MID$(C$, F, 1): IF (F$ = ")") OR (F$ = "|") THEN EXIT FOR
   NEXT F
WORD:
   D = VAL(RIGHT$(C$, LEN(C$) - F))
   REP$ = REP$ + WORD$(C, 0) + " "
   C = D
   IF C <> MNW THEN GOTO RLOOP
   'DO
   '   IF LEFT$(REP$, 1) <> " " THEN EXIT DO
   '   REP$ = RIGHT$(REP$, LEN(REP$) - 1)
   'LOOP
   REP$ = LTRIM$(REP$) ' This line replaces the above code
   IF REP$ = "" THEN EXIT SUB
   out$ = RTRIM$(UCASE$(LEFT$(REP$, 1)) + RIGHT$(REP$, LEN(REP$) - 1)) + "."
   IF debug THEN PRINT "Finished"
   Colour 14, 0
   a$ = "NIALL: "
   IO
   Colour 15, 0
   Crunch out$

END SUB

SUB SEEINSTRUCTIONS
  
   CLS 2
   IF ansi <> 0 AND Online <> 0 THEN a$ = CHR$(27) + "[2J": CALL IO
  
   OPEN "niallbbs.hlp" FOR INPUT AS #3
   G = 0
   WHILE NOT EOF(3)
        LINE INPUT #3, b$
        Crunch b$
        G = G + 1
        IF G = 22 THEN
                GOSUB anykey
                G = 0
        END IF
   WEND
  
   CLOSE #3
   GOSUB anykey
   EXIT SUB

anykey:
   a$ = SPACE$(32) + "PRESS ANY KEY"
   CALL IO
   CALL getchr(an$)
   RETURN

END SUB

SUB USERINPUT
   comm = 0
   a$ = r$ + "USER: "
   IO
   IOA
   IF t$ = "" THEN EXIT SUB
   tt$ = LCASE$(t$)
  
   ' --- Is it a command string ---
   IF LEFT$(tt$, 1) = "#" OR LEFT$(tt$, 1) = "?" THEN
      IF LEFT$(tt$, 1) = "?" THEN tt$ = "#?"
      COMMANDS
      comm = 1
      EXIT SUB
   END IF
       
   ' --- Cuts out the rubbish data ---
   a$ = "!~$%^&*-+|}{@?></#\=[_]" + CHR$(34)
   rr$ = ""
   FOR F = 1 TO LEN(tt$)
      IF MID$(tt$, F, 1) = "," OR MID$(tt$, F, 1) = "?" THEN MID$(tt$, F, 1) = "."
      IF INSTR(a$, MID$(tt$, F, 1)) = 0 THEN
         IF ASC(MID$(tt$, F, 1)) > 31 AND ASC(MID$(tt$, F, 1)) < 127 THEN rr$ = rr$ + MID$(tt$, F, 1)
      END IF
   NEXT F
   tt$ = rr$
  
   ' --- Separates into words ---
   IF RIGHT$(tt$, 1) = "." THEN tt$ = LEFT$(tt$, LEN(tt$) - 1)
   Q$ = tt$
FULLSTOP:
   IF INSTR(Q$, ".") <> 0 THEN
      FS = 1
      tt$ = LEFT$(Q$, INSTR(Q$, ".") - 1)
      Q$ = LTRIM$(RIGHT$(Q$, LEN(Q$) - INSTR(Q$, "."))) 'added ltrim to strip leading spaces
   END IF
   W = 1: W$(W) = ""
   FOR F = 1 TO LEN(tt$)
      IF MID$(tt$, F, 1) = " " THEN IF MID$(tt$, F + 1, 1) <> " " THEN W = W + 1: W$(W) = "": GOTO SKIP
      W$(W) = W$(W) + MID$(tt$, F, 1)
SKIP:
   NEXT F
   
   IF NUM <> MNW - 1 THEN
      IF debug THEN PRINT "Start analysis"
      ANALYSESENTENCE
      IF debug THEN PRINT "End analysis"
   END IF
   IF FS = 1 THEN FS = 0: tt$ = Q$: GOTO FULLSTOP

END SUB

