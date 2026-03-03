/* REPLY.C - Reply generation + dictionary integrity checks
   Compile & Link, all modules:

     C -V HTNBASE.C REPLY.C COMMANDS.C
*/

#include "NIALL.H"

/* ---------------- Local helpers ---------------- */
static int pick_next();
static int append_word();

/* ---------------- Reply (NIALLREPLIES) ---------------- */
/* Generates reply using weighted random walk from start token */
void generate_reply()
{
    int c;
    static char reply[256];     /* static reduces stack pressure */
    int rp;                     /* index instead of pointer */
    int added;
    int max_reply_len;
    int reply_count;
    int next;

    c = 0;
    rp = 0;
    added = 0;
    max_reply_len = 100;
    reply_count = 0;
    reply[0] = 0;

    if (!num_words) {
        printf("NIALL: I cannot speak yet!\n");
        return;
    }

    printf("Starting reply from c = %d\n", c);

    while (1) {

        next = pick_next(c);
        if (next < 0) return;
        c = next;

        printf("Next c = %d\n", c);

        if (c == END_TOKEN)
            break;

        if (!append_word(reply, &rp, words[c].word, &added)) {
            c = END_TOKEN;
            break;
        }

        reply_count++;
        if (reply_count > max_reply_len) {
            printf("Reply loop detected - truncating.\n");
            break;
        }
    }

    reply[rp] = 0;

    /* trim leading spaces */
    {
        int i;
        i = 0;
        while (reply[i] == ' ') i++;
        if (i > 0) strcpy(reply, &reply[i]);
    }

    /* trim trailing */
    {
        int len;
        len = strlen(reply);
        while (len > 0 && reply[len-1] == ' ') reply[--len] = 0;
    }

    if (reply[0]) {
        if (reply[0] >= 'a' && reply[0] <= 'z')
            reply[0] += 'A' - 'a';

        if ((int)strlen(reply) + 2 < (int)sizeof(reply))
            strcat(reply, ".");

        printf("NIALL: %s\n", reply);
    }
}

/* Choose the next state from word c based on its links weights. */
static int pick_next(c)
    int c;
{
    char *cstr;
    unsigned int total;
    unsigned int r;
    char *p;
    char *q;
    int id, cnt;

    cstr = words[c].links;
    if (cstr == (char *)0) return -1;

    total = (unsigned int)atoi(cstr);
    if (total == 0) return -1;

    r = (unsigned int)(rand() & 0x7fff);
    r = (r % total) + 1;

    p = strchr(cstr, '(');
    while (p) {
        q = p - 1;
        while (q > cstr && *q != '|' && *q != ')')
            q--;
        if (*q == '|' || *q == ')')
            q++;

        id = atoi(q);
        cnt = atoi(p + 1);

        r -= (unsigned int)cnt;
        if ((int)r <= 0) {
            if (!idx_ok(id)) return -1;
            return id;
        }

        p = strchr(p + 1, '(');
    }

    return -1;
}

/* Append a word to reply safely. */
static int append_word(reply, prp, wd, padded)
    char *reply;
    int *prp;
    char *wd;
    int *padded;
{
    int rp;
    int wlen;

    rp = *prp;

    if (wd == (char *)0) return 1;
    if (wd[0] == 0) return 1;
    if (wd[0] == ' ' && wd[1] == 0) return 1;
    if ((unsigned char)wd[0] < ' ') return 1;

    wlen = strlen(wd);

    if (*padded) {
        if (rp + 1 >= 255) return 0;
        reply[rp++] = ' ';
    }

    if (rp + wlen >= 255) return 0;

    strcpy(&reply[rp], wd);
    rp += wlen;

    *padded = 1;
    *prp = rp;
    return 1;
}

/* ---------------- Dictionary integrity checker ---------------- */
void dict_check()
{
    int i;

    if (!(words[0].word[0] == ' ' && words[0].word[1] == 0)) {
        printf("DICTCHK: word0 bad\n");
        return;
    }
    if (!(words[END_TOKEN].word[0] == '.' && words[END_TOKEN].word[1] == 0)) {
        printf("DICTCHK: endtok bad\n");
        return;
    }

    for (i = 1; i <= num_words; i++) {
        if (words[i].word[0] == 0) {
            printf("DICTCHK: BLANK at %d\n", i);
            return;
        }
        if ((unsigned char)words[i].word[0] < ' ') {
            printf("DICTCHK: word[%d] starts with ctrl %u\n",
                   i, (unsigned char)words[i].word[0]);
            return;
        }
        if (words[i].word[31] != 0) {
            printf("DICTCHK: word[%d] no NUL\n", i);
            return;
        }
    }
}
