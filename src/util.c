/*
 * gaim
 *
 * Copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#else
#include <direct.h>
#include <io.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#include <math.h>
#include "gaim.h"
#include "prpl.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include "win32dep.h"
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

static char home_dir[MAXPATHLEN];

char *full_date()
{
	char *date;
	time_t tme;

	time(&tme);
	date = ctime(&tme);
	date[strlen(date) - 1] = '\0';
	return date;
}

G_GNUC_CONST static gint badchar(char c)
{
	switch (c) {
	case ' ':
	case ',':
	case '(':
	case ')':
	case '\0':
	case '\n':
	case '<':
	case '>':
	case '"':
	case '\'':
		return 1;
	default:
		return 0;
	}
}


gchar *sec_to_text(guint sec)
{
	guint daze, hrs, min;
	char *ret = NULL;

	daze = sec / (60 * 60 * 24);
	hrs = (sec % (60 * 60 * 24)) / (60 * 60);
	min = (sec % (60 * 60)) / 60;
	sec = min % 60;

	if (daze) {
		if (hrs || min) {
			if (hrs) {
				if (min) {
					ret = g_strdup_printf(
						   "%d %s, %d %s, %d %s.",
						   daze, ngettext("day","days",daze),
						   hrs, ngettext("hour","hours",hrs), min, ngettext("minute","minutes",min));
				} else {
					ret = g_strdup_printf(
						   "%d %s, %d %s.",
						   daze, ngettext("day","days",daze), hrs, ngettext("hour","hours",hrs));
				}
			} else {
				ret = g_strdup_printf(
					   "%d %s, %d %s.",
					   daze, ngettext("day","days",daze), min, ngettext("minute","minutes",min));
			}
		} else
			ret = g_strdup_printf("%d %s.", daze, ngettext("day","days",daze));
	} else {
		if (hrs) {
			if (min) {
				ret = g_strdup_printf(
					   "%d %s, %d %s.",
					   hrs, ngettext("hour","hours",hrs), min, ngettext("minute","minutes",min));
			} else {
				ret = g_strdup_printf("%d %s.", hrs, ngettext("hour","hours",hrs));
			}
		} else {
			ret = g_strdup_printf("%d %s.", min, ngettext("minute","minutes",min));
		}
	}

	return ret;
}

char *linkify_text(const char *text)
{
	const char *c, *t, *q = NULL;
	char *tmp;
	char url_buf[BUF_LEN * 4];
	GString *ret = g_string_new("");
	/* Assumes you have a buffer able to cary at least BUF_LEN * 2 bytes */

	c = text;
	while (*c) {
		if(!q && (*c == '\"' || *c == '\'')) {
			q = c;
		} else if(q) {
			if(*c == *q)
				q = NULL;
		} else if (!g_ascii_strncasecmp(c, "<A", 2)) {
			while (1) {
				if (!g_ascii_strncasecmp(c, "/A>", 3)) {
					break;
				}
				ret = g_string_append_c(ret, *c);
				c++;
				if (!(*c))
					break;
			}
		} else if ((*c=='h') && (!g_ascii_strncasecmp(c, "http://", 7) ||
					(!g_ascii_strncasecmp(c, "https://", 8)))) {
			t = c;
			while (1) {
				if (badchar(*t)) {

					if (*(t) == ',' && (*(t + 1) != ' ')) {
						t++;
						continue;
					}

					if (*(t - 1) == '.')
						t--;
					strncpy(url_buf, c, t - c);
					url_buf[t - c] = 0;
					g_string_append_printf(ret, "<A HREF=\"%s\">%s</A>",
							url_buf, url_buf);
					c = t;
					break;
				}
				if (!t)
					break;
				t++;

			}
		} else if (!g_ascii_strncasecmp(c, "www.", 4)) {
			if (c[4] != '.') {
				t = c;
				while (1) {
					if (badchar(*t)) {
						if (t - c == 4) {
							break;
						}

						if (*(t) == ',' && (*(t + 1) != ' ')) {
							t++;
							continue;
						}

						if (*(t - 1) == '.')
							t--;
						strncpy(url_buf, c, t - c);
						url_buf[t - c] = 0;
						g_string_append_printf(ret,
								"<A HREF=\"http://%s\">%s</A>", url_buf,
								url_buf);
						c = t;
						break;
					}
					if (!t)
						break;
					t++;
				}
			}
		} else if (!g_ascii_strncasecmp(c, "ftp://", 6)) {
			t = c;
			while (1) {
				if (badchar(*t)) {
					if (*(t - 1) == '.')
						t--;
					strncpy(url_buf, c, t - c);
					url_buf[t - c] = 0;
					g_string_append_printf(ret, "<A HREF=\"%s\">%s</A>",
							url_buf, url_buf);
					c = t;
					break;
				}
				if (!t)
					break;
				t++;

			}
		} else if (!g_ascii_strncasecmp(c, "ftp.", 4)) {
			if (c[4] != '.') {
				t = c;
				while (1) {
					if (badchar(*t)) {
						if (t - c == 4) {
							break;
						}
						if (*(t - 1) == '.')
							t--;
						strncpy(url_buf, c, t - c);
						url_buf[t - c] = 0;
						g_string_append_printf(ret,
								"<A HREF=\"ftp://%s\">%s</A>", url_buf,
								url_buf);
						c = t;
						break;
					}
					if (!t)
						break;
					t++;
				}
			}
		} else if (!g_ascii_strncasecmp(c, "mailto:", 7)) {
			t = c;
			while (1) {
				if (badchar(*t)) {
					if (*(t - 1) == '.')
						t--;
					strncpy(url_buf, c, t - c);
					url_buf[t - c] = 0;
					g_string_append_printf(ret, "<A HREF=\"%s\">%s</A>",
							  url_buf, url_buf);
					c = t;
					break;
				}
				if (!t)
					break;
				t++;

			}
		} else if (c != text && (*c == '@')) {
			char *tmp;
			int flag;
			int len = 0;
			const char illegal_chars[] = "!@#$%^&*()[]{}/|\\<>\":;\r\n \0";
			url_buf[0] = 0;

			if (strchr(illegal_chars,*(c - 1)) || strchr(illegal_chars, *(c + 1)))
				flag = 0;
			else
				flag = 1;

			t = c;
			while (flag) {
				if (badchar(*t)) {
					ret = g_string_truncate(ret, ret->len - (len - 1));
					break;
				} else {
					len++;
					tmp = g_malloc(len + 1);
					tmp[len] = 0;
					tmp[0] = *t;
					strncpy(tmp + 1, url_buf, len - 1);
					strcpy(url_buf, tmp);
					url_buf[len] = 0;
					g_free(tmp);
					t--;
					if (t < text) {
						ret = g_string_assign(ret, "");
						break;
					}
				}
			}

			t = c + 1;

			while (flag) {
				if (badchar(*t)) {
					char *d;

					for (d = url_buf + strlen(url_buf) - 1; *d == '.'; d--, t--)
						*d = '\0';

					g_string_append_printf(ret, "<A HREF=\"mailto:%s\">%s</A>",
							url_buf, url_buf);
					c = t;

					break;
				} else {
					strncat(url_buf, t, 1);
					len++;
					url_buf[len] = 0;
				}

				t++;
			}
		}

		if (*c == 0)
			break;

		ret = g_string_append_c(ret, *c);
		c++;

	}
	tmp = ret->str;
	g_string_free(ret, FALSE);
	return tmp;
}


static const char alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

/* This was borrowed from the Kame source, and then tweaked to our needs */
char *tobase64(const unsigned char *buf, size_t len)
{
	char *s, *rv;
	unsigned tmp;

	if(len < 0)
		len = strlen(buf);

	s = g_malloc((4 * (len + 1)) / 3 + 1);

	rv = s;
	while (len >= 3) {
		tmp = buf[0] << 16 | buf[1] << 8 | buf[2];
		s[0] = alphabet[tmp >> 18];
		s[1] = alphabet[(tmp >> 12) & 077];
		s[2] = alphabet[(tmp >> 6) & 077];
		s[3] = alphabet[tmp & 077];
		len -= 3;
		buf += 3;
		s += 4;
	}

	/* RFC 1521 enumerates these three possibilities... */
	switch(len) {
		case 2:
			tmp = buf[0] << 16 | buf[1] << 8;
			s[0] = alphabet[(tmp >> 18) & 077];
			s[1] = alphabet[(tmp >> 12) & 077];
			s[2] = alphabet[(tmp >> 6) & 077];
			s[3] = '=';
			s[4] = '\0';
			break;
		case 1:
			tmp = buf[0] << 16;
			s[0] = alphabet[(tmp >> 18) & 077];
			s[1] = alphabet[(tmp >> 12) & 077];
			s[2] = s[3] = '=';
			s[4] = '\0';
			break;
		case 0:
			s[0] = '\0';
			break;
	}

	return rv;
}


void frombase64(const char *text, char **data, int *size)
{
	char *out = NULL;
	char tmp = 0;
	const char *c;
	gint32 tmp2 = 0;
	int len = 0, n = 0;

	if (!text || !data)
		return;

	c = text;

	while (*c) {
		if (*c >= 'A' && *c <= 'Z') {
			tmp = *c - 'A';
		} else if (*c >= 'a' && *c <= 'z') {
			tmp = 26 + (*c - 'a');
		} else if (*c >= '0' && *c <= 57) {
			tmp = 52 + (*c - '0');
		} else if (*c == '+') {
			tmp = 62;
		} else if (*c == '/') {
			tmp = 63;
		} else if (*c == '\r' || *c == '\n') {
			c++;
			continue;
		} else if (*c == '=') {
			if (n == 3) {
				out = g_realloc(out, len + 2);
				out[len] = (char)(tmp2 >> 10) & 0xff;
				len++;
				out[len] = (char)(tmp2 >> 2) & 0xff;
				len++;
			} else if (n == 2) {
				out = g_realloc(out, len + 1);
				out[len] = (char)(tmp2 >> 4) & 0xff;
				len++;
			}
			break;
		}
		tmp2 = ((tmp2 << 6) | (tmp & 0xff));
		n++;
		if (n == 4) {
			out = g_realloc(out, len + 3);
			out[len] = (char)((tmp2 >> 16) & 0xff);
			len++;
			out[len] = (char)((tmp2 >> 8) & 0xff);
			len++;
			out[len] = (char)(tmp2 & 0xff);
			len++;
			tmp2 = 0;
			n = 0;
		}
		c++;
	}

	out = g_realloc(out, len + 1);
	out[len] = 0;

	*data = out;
	if (size)
		*size = len;
}

/*
 * Converts raw data to a pretty, null-terminated base16 string.
 */
unsigned char *tobase16(const unsigned char *data, int length)
{
	int i;
	unsigned char *ascii = NULL;

	if (!data || !length)
		return NULL;

	ascii = malloc(length*2 + 1);

	for (i=0; i<length; i++)
		snprintf(&ascii[i*2], 3, "%02hhx", data[i]);

	return ascii;
}

/*
 * Converts a null-terminated string of hexidecimal to raw data.
 */
int frombase16(const char *ascii, unsigned char **raw)
{
	int len, i, accumulator=0;
	unsigned char *data;

	if (!ascii || !(len = strlen(ascii)) || (len % 2))
		return 0;

	data = malloc(len/2);
	for (i=0; i<len; i++) {
		if (!(i % 2))
			accumulator = 0;
		else
			accumulator = accumulator << 4;
		if (isdigit(ascii[i]))
			accumulator |= ascii[i]-48;
		else switch(ascii[i]) {
			case 'a':  case 'A':  accumulator|=10;  break;
			case 'b':  case 'B':  accumulator|=11;  break;
			case 'c':  case 'C':  accumulator|=12;  break;
			case 'd':  case 'D':  accumulator|=13;  break;
			case 'e':  case 'E':  accumulator|=14;  break;
			case 'f':  case 'F':  accumulator|=15;  break;
		}
		if (i % 2)
			data[(i-1)/2] = accumulator;
	}

	*raw = data;
	return len/2;
}

char *normalize(const char *s)
{
	static char buf[BUF_LEN];
	char *tmp;
	int i, j;

	g_return_val_if_fail((s != NULL), NULL);

	strncpy(buf, s, BUF_LEN);
	for (i=0, j=0; buf[j]; i++, j++) {
		while (buf[j] == ' ')
			j++;
		buf[i] = buf[j];
	}
	buf[i] = '\0';

	tmp = g_utf8_strdown(buf, -1);
	g_snprintf(buf, sizeof(buf), "%s", tmp);
	g_free(tmp);
	tmp = g_utf8_normalize(buf, -1, G_NORMALIZE_DEFAULT);
	g_snprintf(buf, sizeof(buf), "%s", tmp);
	g_free(tmp);

	return buf;
}

char *date()
{
	static char date[80];
	time_t tme;
	time(&tme);
	strftime(date, sizeof(date), "%H:%M:%S", localtime(&tme));
	return date;
}

void clean_pid(void)
{
#ifndef _WIN32
	int status;
	pid_t pid;

	do {
		pid = waitpid(-1, &status, WNOHANG);
	} while (pid != 0 && pid != (pid_t)-1);
	if(pid == (pid_t)-1 && errno != ECHILD) {
		char errmsg[BUFSIZ];
		snprintf(errmsg, BUFSIZ, "Warning: waitpid() returned %d", pid);
		perror(errmsg);
	}
#endif
}

struct gaim_account *gaim_account_find(const char *name, int protocol)
{
	char *who = g_strdup(normalize(name));
	GSList *accts = gaim_accounts;
	struct gaim_account *account;

	while (accts) {
		account = (struct gaim_account *)accts->data;
		if (!strcmp(normalize(account->username), who)) {
			if (protocol != -1) {
				if (account->protocol == protocol) {
					g_free(who);
					return account;
				}
			} else {
				g_free(who);
				return account;
			}

		}
		accts = accts->next;
	}
	g_free(who);
	return NULL;
}


/* Look for %n, %d, or %t in msg, and replace with the sender's name, date,
   or time */
char *away_subs(const char *msg, const char *name)
{
	char *c;
	static char cpy[BUF_LONG];
	int cnt = 0;
	time_t t = time(0);
	struct tm *tme = localtime(&t);
	char tmp[20];

	cpy[0] = '\0';
	c = (char *)msg;
	while (*c) {
		switch (*c) {
		case '%':
			if (*(c + 1)) {
				switch (*(c + 1)) {
				case 'n':
					/* append name */
					strcpy(cpy + cnt, name);
					cnt += strlen(name);
					c++;
					break;
				case 'd':
					/* append date */
					strftime(tmp, 20, "%m/%d/%Y", tme);
					strcpy(cpy + cnt, tmp);
					cnt += strlen(tmp);
					c++;
					break;
				case 't':
					/* append time */
					strftime(tmp, 20, "%r", tme);
					strcpy(cpy + cnt, tmp);
					cnt += strlen(tmp);
					c++;
					break;
				default:
					cpy[cnt++] = *c;
				}
			}
			break;
		default:
			cpy[cnt++] = *c;
		}
		c++;
	}
	cpy[cnt] = '\0';
	return (cpy);
}

char *stylize(const gchar *text, int length)
{
	gchar *buf;
	char *tmp = g_malloc(length);

	buf = g_malloc(length);
	g_snprintf(buf, length, "%s", text);

	if (font_options & OPT_FONT_BOLD) {
		g_snprintf(tmp, length, "<B>%s</B>", buf);
		strcpy(buf, tmp);
	}

	if (font_options & OPT_FONT_ITALIC) {
		g_snprintf(tmp, length, "<I>%s</I>", buf);
		strcpy(buf, tmp);
	}

	if (font_options & OPT_FONT_UNDERLINE) {
		g_snprintf(tmp, length, "<U>%s</U>", buf);
		strcpy(buf, tmp);
	}

	if (font_options & OPT_FONT_STRIKE) {
		g_snprintf(tmp, length, "<S>%s</S>", buf);
		strcpy(buf, tmp);
	}

	if (font_options & OPT_FONT_FACE) {
		g_snprintf(tmp, length, "<FONT FACE=\"%s\">%s</FONT>", fontface, buf);
		strcpy(buf, tmp);
	}

	if (font_options & OPT_FONT_SIZE) {
		g_snprintf(tmp, length, "<FONT SIZE=\"%d\">%s</FONT>", fontsize, buf);
		strcpy(buf, tmp);
	}

	if (font_options & OPT_FONT_FGCOL) {
		g_snprintf(tmp, length, "<FONT COLOR=\"#%02X%02X%02X\">%s</FONT>", fgcolor.red/256,
			   fgcolor.green/256, fgcolor.blue/256, buf);
		strcpy(buf, tmp);
	}

	if (font_options & OPT_FONT_BGCOL) {
		g_snprintf(tmp, length, "<BODY BGCOLOR=\"#%02X%02X%02X\">%s</BODY>", bgcolor.red/256,
			   bgcolor.green/256, bgcolor.blue/256, buf);
		strcpy(buf, tmp);
	}

	g_free(tmp);
	return buf;
}

void show_usage(int mode, const char *name)
{
	switch (mode) {
	case 0:		/* full help text */
		printf(_("Gaim %s\n"
		       "Usage: %s [OPTION]...\n\n"
		       "  -a, --acct          display account editor window\n"
		       "  -w, --away[=MESG]   make away on signon (optional argument MESG specifies\n"
		       "                      name of away message to use)\n"
		       "  -l, --login[=NAME]  automatically login (optional argument NAME specifies\n"
		       "                      account(s) to use, seperated by commas)\n"
		       "  -n, --loginwin      don't automatically login; show login window\n"
		       "  -u, --user=NAME     use account NAME\n"
		       "  -f, --file=FILE     use FILE as config\n"
		       "  -d, --debug         print debugging messages to stdout\n"
		       "  -v, --version       display the current version and exit\n"
		       "  -h, --help          display this help and exit\n"), VERSION, name);
		break;
	case 1:		/* short message */
		printf(_("Gaim %s. Try `%s -h' for more information.\n"), VERSION, name);
		break;
	}
}

GSList *message_split(char *message, int limit)
{
	static GSList *ret = NULL;
	int lastgood = 0, curgood = 0, curpos = 0, len = strlen(message);
	gboolean intag = FALSE;

	if (ret) {
		GSList *tmp = ret;
		while (tmp) {
			g_free(tmp->data);
			tmp = g_slist_remove(tmp, tmp->data);
		}
		ret = NULL;
	}

	while (TRUE) {
		if (lastgood >= len)
			return ret;

		if (len - lastgood < limit) {
			ret = g_slist_append(ret, g_strdup(&message[lastgood]));
			return ret;
		}

		curgood = curpos = 0;
		intag = FALSE;
		while (curpos <= limit) {
			if (isspace(message[curpos + lastgood]) && !intag)
				curgood = curpos;
			if (message[curpos + lastgood] == '<')
				intag = TRUE;
			if (message[curpos + lastgood] == '>')
				intag = FALSE;
			curpos++;
		}

		if (curgood) {
			ret = g_slist_append(ret, g_strndup(&message[lastgood], curgood));
			if (isspace(message[curgood + lastgood]))
				lastgood += curgood + 1;
			else
				lastgood += curgood;
		} else {
			/* whoops, guess we have to fudge it here */
			ret = g_slist_append(ret, g_strndup(&message[lastgood], limit));
			lastgood += limit;
		}
	}
}

const gchar *gaim_home_dir()
{
	if(g_get_home_dir())
		return g_get_home_dir();
	else
#ifndef _WIN32
		return NULL;
#else
		/* Win9x and WinME don't have a home dir */
		return "C:";
#endif

}

/* returns a string of the form ~/.gaim, where ~ is replaced by the user's home
 * dir. Note that there is no trailing slash after .gaim. */
gchar *gaim_user_dir()
{
	const gchar *hd = gaim_home_dir();
        if(hd) {
		strcpy( (char*)&home_dir, hd );
		strcat( (char*)&home_dir, G_DIR_SEPARATOR_S ".gaim" );
		return (gchar*)&home_dir;
	}
	else {
   	        return NULL;
	}
}

/*
 * rcg10312000 This could be more robust, but it works for my current
 *  goal: to remove those annoying <BR> tags.  :)
 * dtf12162000 made the loop more readable. i am a neat freak. ;) */
void strncpy_nohtml(gchar *dest, const gchar *src, size_t destsize)
{
	gchar *ptr;
	g_snprintf(dest, destsize, "%s", src);

	while ((ptr = strstr(dest, "<BR>")) != NULL) {
		/* replace <BR> with a newline. */
		*ptr = '\n';
		memmove(ptr + 1, ptr + 4, strlen(ptr + 4) + 1);
	}
}

void strncpy_withhtml(gchar *dest, const gchar *src, size_t destsize)
{
	gchar *end = dest + destsize;

	while (dest < end) {
		if (*src == '\n' && dest < end - 5) {
			strcpy(dest, "<BR>");
			src++;
			dest += 4;
		} else {
			*dest++ = *src;
			if (*src == '\0')
				return;
			else
				src++;
		}
	}
}


/*
 * Like strncpy_withhtml (above), but malloc()'s the necessary space
 *
 * The caller is responsible for freeing the space pointed to by the
 * return value.
 */

gchar *strdup_withhtml(const gchar *src)
{
	gchar *sp, *dest;
	gulong destsize;

	if(!src)
		return NULL;

	/*
	 * All we need do is multiply the number of newlines by 3 (the
	 * additional length of "<BR>" over "\n"), account for the
	 * terminator, malloc the space and call strncpy_withhtml.
	 */
	for(destsize = 0, sp = (gchar *)src; (sp = strchr(sp, '\n')) != NULL; ++sp, ++destsize)
		;
	destsize *= 3;
	destsize += strlen(src) + 1;
	dest = g_malloc(destsize);
	strncpy_withhtml(dest, src, destsize);

	return(dest);
}

void strip_linefeed(gchar *text)
{
	int i, j;
	gchar *text2 = g_malloc(strlen(text) + 1);

	for (i = 0, j = 0; text[i]; i++)
		if (text[i] != '\r')
			text2[j++] = text[i];
	text2[j] = '\0';

	strcpy(text, text2);
	g_free(text2);
}

char *add_cr(const char *text)
{
	char *ret = NULL;
	int count = 0, i, j;

	if (text[0] == '\n')
		count++;
	for (i = 1; i < strlen(text); i++)
		if (text[i] == '\n' && text[i - 1] != '\r')
			count++;

	if (count == 0)
		return g_strdup(text);

	ret = g_malloc0(strlen(text) + count + 1);

	i = 0; j = 0;
	if (text[i] == '\n')
		ret[j++] = '\r';
	ret[j++] = text[i++];
	for (; i < strlen(text); i++) {
		if (text[i] == '\n' && text[i - 1] != '\r')
			ret[j++] = '\r';
		ret[j++] = text[i];
	}

	gaim_debug(GAIM_DEBUG_INFO, "add_cr", "got: %s, leaving with %s\n",
			   text, ret);

	return ret;
}

time_t get_time(int year, int month, int day, int hour, int min, int sec)
{
	struct tm tm;

	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec >= 0 ? sec : time(NULL) % 60;
	return mktime(&tm);
}

/*
 * Like mkstemp() but returns a file pointer, uses a pre-set template,
 * uses the semantics of tempnam() for the directory to use and allocates
 * the space for the filepath.
 *
 * Caller is responsible for closing the file and removing it when done,
 * as well as freeing the space pointed-to by "path" with g_free().
 *
 * Returns NULL on failure and cleans up after itself if so.
 */
static const char *gaim_mkstemp_templ = {"gaimXXXXXX"};

FILE *gaim_mkstemp(gchar **fpath)
{
	const gchar *tmpdir;
#ifndef _WIN32
	int fd;
#endif
	FILE *fp = NULL;

	if((tmpdir = (gchar*)g_get_tmp_dir()) != NULL) {
		if((*fpath = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", tmpdir, gaim_mkstemp_templ)) != NULL) {
#ifdef _WIN32
			char* result = _mktemp( *fpath );
			if( result == NULL )
				gaim_debug(GAIM_DEBUG_ERROR, "gaim_mkstemp",
						   "Problem creating the template\n");
			else
			{
				if( (fp = fopen( result, "w+" )) == NULL ) {
					gaim_debug(GAIM_DEBUG_ERROR, "gaim_mkstemp",
							   "Couldn't fopen() %s\n", result);
				}
			}
#else
			if((fd = mkstemp(*fpath)) == -1) {
				gaim_debug(GAIM_DEBUG_ERROR, "gaim_mkstemp",
						   "Couldn't make \"%s\", error: %d\n",
						   *fpath, errno);
			} else {
				if((fp = fdopen(fd, "r+")) == NULL) {
					close(fd);
					gaim_debug(GAIM_DEBUG_ERROR, "gaim_mkstemp",
							   "Couldn't fdopen(), error: %d\n", errno);
				}
			}
#endif
			if(!fp) {
				g_free(*fpath);
				*fpath = NULL;
			}
		}
	} else {
		gaim_debug(GAIM_DEBUG_ERROR, "gaim_mkstemp",
				   "g_get_tmp_dir() failed!");
	}

	return fp;
}

/* AIM URI's ARE FUN :-D */
const char *handle_uri(char *uri) {
	GString *str;
	GSList *conn = connections;
	struct gaim_connection *gc = NULL;

	gaim_debug(GAIM_DEBUG_INFO, "handle_uri", "Handling URI: %s\n", uri);

	/* Well, we'd better check to make sure we have at least one
	   AIM account connected. */
	while (conn) {
		gc = conn->data;
		if (gc->protocol == GAIM_PROTO_OSCAR && isalpha(gc->username[0])) {
			break;
		}
		conn = conn->next;
	}

	if (gc == NULL)
		return _("Not connected to AIM");

 	/* aim:goim?screenname=screenname&message=message */
	if (!g_ascii_strncasecmp(uri, "aim:goim?", strlen("aim:goim?"))) {
		char *who, *what;
		struct gaim_conversation *c;
		uri = uri + strlen("aim:goim?");
		
		if (!(who = strstr(uri, "screenname="))) {
			return _("No screenname given.");
		}
		/* spaces are encoded as +'s */
		who = who + strlen("screenname=");
		str = g_string_new(NULL);
		while (*who && (*who != '&')) {
			g_string_append_c(str, *who == '+' ? ' ' : *who);
			who++;
		}
		who = g_strdup(str->str);
		g_string_free(str, TRUE);
		
		what = strstr(uri, "message=");
		if (what) {
			what = what + strlen("message=");
			str = g_string_new(NULL);
			while (*what && (*what != '&' || !g_ascii_strncasecmp(what, "&amp;", 5))) {
				g_string_append_c(str, *what == '+' ? ' ' : *what);
				what++;
			}
			what = g_strdup(str->str);
			g_string_free(str, TRUE);
		}

		c = gaim_conversation_new(GAIM_CONV_IM, gc->account, who);
		g_free(who);

		if (what) {
			struct gaim_gtk_conversation *gtkconv = GAIM_GTK_CONVERSATION(c);

			gtk_text_buffer_insert_at_cursor(gtkconv->entry_buffer, what, -1);
			g_free(what);
		}
	} else if (!g_ascii_strncasecmp(uri, "aim:addbuddy?", strlen("aim:addbuddy?"))) {
		char *who, *group;
		uri = uri + strlen("aim:addbuddy?");
		/* spaces are encoded as +'s */

		if (!(who = strstr(uri, "screenname="))) {
			return _("No screenname given.");
		}
		who = who + strlen("screenname=");
		str = g_string_new(NULL);
		while (*who && (*who != '&')) {
			g_string_append_c(str, *who == '+' ? ' ' : *who);
			who++;
		}
		who = g_strdup(str->str);
		g_string_free(str, TRUE);

		group = strstr(uri, "group=");
		if (group) {
			group = group + strlen("group=");
			str = g_string_new(NULL);
			while (*group && (*group != '&' || !g_ascii_strncasecmp(group, "&amp;", 5))) {
				g_string_append_c(str, *group == '+' ? ' ' : *group);
				group++;
			}
			group = g_strdup(str->str);
			g_string_free(str, TRUE);
		}

		gaim_debug(GAIM_DEBUG_MISC, "handle_uri", "who: %s\n", who);
		show_add_buddy(gc, who, group, NULL);
		g_free(who);
		if (group)
			g_free(group);
	} else if (!g_ascii_strncasecmp(uri, "aim:gochat?", strlen("aim:gochat?"))) {
		char *room;
		GHashTable *components;
		int exch = 5;
		
		uri = uri + strlen("aim:gochat?");
		/* spaces are encoded as +'s */
		
		if (!(room = strstr(uri, "roomname="))) {
			return _("No roomname given.");
		}
		room = room + strlen("roomname=");
		str = g_string_new(NULL);
		while (*room && (*room != '&')) {
			g_string_append_c(str, *room == '+' ? ' ' : *room);
			room++;
		}
		room = g_strdup(str->str);
		g_string_free(str, TRUE);
		components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
				g_free);
		g_hash_table_replace(components, g_strdup("room"), room);
		g_hash_table_replace(components, g_strdup("exchange"),
				g_strdup_printf("%d", exch));

		serv_join_chat(gc, components);
		g_hash_table_destroy(components);
	} else {
		return _("Invalid AIM URI");
	}
	
	
	return NULL;
}

char *gaim_try_conv_to_utf8(const char *str)
{
	int converted;
	char *utf8;

	if (str == NULL) {
		return NULL;
	}

	if (g_utf8_validate(str, -1, NULL)) {
		return g_strdup(str);
	}

	utf8 = g_locale_to_utf8(str, -1, &converted, NULL, NULL);
	if (utf8 && converted == strlen (str)) {
		return(utf8);
	} else if (utf8) {
		g_free(utf8);
	}

	utf8 = g_convert(str, -1, "UTF-8", "ISO-8859-15", &converted, NULL, NULL);
	if (utf8 && converted == strlen (str)) {
		return(utf8);
	} else if (utf8) {
		g_free(utf8);
	}

	return(NULL);
}

char *gaim_getip_from_fd(int fd)
{
	struct sockaddr addr;
	socklen_t namelen = sizeof(addr);

	if (getsockname(fd, &addr, &namelen))
		return NULL;

	return g_strdup(inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr));
}

gint gaim_utf8_strcasecmp(const gchar *a, const gchar *b) {
	gchar *a_norm = g_utf8_casefold(a, -1);
	gchar *b_norm = g_utf8_casefold(b, -1);
	gint ret = g_utf8_collate(a_norm, b_norm);
	g_free(a_norm);
	g_free(b_norm);
	return ret;
}

gchar *gaim_strreplace(const gchar *string, const gchar *delimiter, const gchar *replacement) {
	gchar **split;
	gchar *ret;

	split = g_strsplit(string, delimiter, 0);
	ret = g_strjoinv(replacement, split);
	g_strfreev(split);

	return ret;
}
