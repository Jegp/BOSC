#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

/* --- symbolic constants --- */
#define COMMANDMAX 20
#define BUFFERMAX 256
#define PBUFFERMAX 50
#define PIPE  ('|')
#define BG    ('&')
#define RIN   ('<')
#define RUT   ('>')
#define IDCHARS "_-.,/~+"

/* --- symbolic macros --- */
#define ispipe(c) ((c) == PIPE)
#define isbg(c)   ((c) == BG)
#define isrin(c)  ((c) == RIN)
#define isrut(c)  ((c) == RUT)
#define isspec(c) (ispipe(c) || isbg(c) || isrin(c) || isrut(c))

/* --- static memory allocation --- */
static Cmd  cmdbuf[COMMANDMAX], *cmds;
static char cbuf[BUFFERMAX], *cp;
static char *pbuf[PBUFFERMAX], **pp;

/*
 * parse : A simple commandline parser.
 */

/* --- parse the commandline and build shell commmand structure --- */
int parsecommand(char *cmdline, Shellcmd *shellcmd) {
  int i, n;
  Cmd *cmd0;

  char *t = cmdline;
  char *tok;

  // Initialize list
  for (i = 0; i < COMMANDMAX-1; i++) cmdbuf[i].next = &cmdbuf[i+1];
    
  cmdbuf[COMMANDMAX-1].next = NULL;
  cmds = cmdbuf;
  cp = cbuf;
  pp = pbuf;

  shellcmd->rd_stdin    = NULL;
  shellcmd->rd_stdout   = NULL;
  shellcmd->rd_stderr   = NULL;
  shellcmd->background  = 0; // false 
  shellcmd->the_cmds    = NULL;

  do {
    if ((n = acmd(t, &cmd0)) <= 0)
      return -1;
    t += n;

    cmd0->next = shellcmd->the_cmds;
    shellcmd->the_cmds = cmd0;

    int newtoken = 1;
    while (newtoken) {
      n = nexttoken(t, &tok);
      if (n == 0)
	{
	  return 1;
	}
      t += n;

      switch(*tok) {
      case PIPE:
	newtoken = 0;
	break;
      case BG:
	n = nexttoken(t, &tok);
	if (n == 0)
	  {
	    shellcmd->background = 1;
	    return 1;
	  }
	else
	  {
	    fprintf(stderr, "illegal backgrounding\n");
	    return -1;
	  }
	newtoken = 0;
	break;
      case RIN:
	if (shellcmd->rd_stdin != NULL)
	  {
	    fprintf(stderr, "duplicate redirection of stdin\n");
	    return -1;
	  }
	if ((n = nexttoken(t, &(shellcmd->rd_stdin))) < 0) 
	  return -1;
	if (!isidentifier(shellcmd->rd_stdin))
	  {
	    fprintf(stderr, "Illegal filename: \"%s\"\n", shellcmd->rd_stdin);
	    return -1;
	  }
	t += n;
	break;
      case RUT:
	if (shellcmd->rd_stdout != NULL)
	  {
	    fprintf(stderr, "duplicate redirection of stdout\n");
	    return -1;
	  }
	if ((n = nexttoken(t, &(shellcmd->rd_stdout))) < 0) 
	  return -1;
	if (!isidentifier(shellcmd->rd_stdout))
	  {
	    fprintf(stderr, "Illegal filename: \"%s\"\n", shellcmd->rd_stdout);
	    return -1;
	  }
	t += n;
	break;
      default:
	return -1;
      }
    }
  } while (1);
  return 0;
}

int nexttoken( char *s, char **tok)
{
  char *s0 = s;
  char c;

  *tok = cp;
  while (isspace(c = *s++) && c);
  if (c == '\0')
    {
      *cp++ = '\0';
      return 0;
    }
  if (isspec(c))
    {
      *cp++ = c;
      *cp++ = '\0';
    }
  else
    {
      *cp++ = c;
      do
	{
	  c = *cp++ = *s++;
	} while (!isspace(c) && !isspec(c) && (c != '\0'));
      --s;
      --cp;
      *cp++ = '\0';
    }
  return s - s0;
}

int acmd (char *s, Cmd **cmd)
{
  char *tok;
  int n, cnt = 0;
  Cmd *cmd0 = cmds;
  cmds = cmds->next;
  cmd0->next = NULL;
  cmd0->cmd = pp;

  while (1) {
    n = nexttoken(s, &tok);
    if (n == 0 || isspec(*tok))
      {
	*cmd = cmd0;
	*pp++ = NULL;
	return cnt;
      }
    else
      {
	*pp++ = tok;
	cnt += n;
	s += n;
      }
  }
}

int isidentifier (char *s)
{
  while (*s)
    {
      char *p = strrchr (IDCHARS,  *s);
      if (! isalnum(*s++) && (p == NULL))
	return 0;
    }
  return 1;
}
