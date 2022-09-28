/******************************************************************************
* Copyright (C) 1994-2008 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
/*
Adapted by Domingo Alvarez Duarte 2012
http://code.google.com/p/lua-regex-standalone/
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>

#include "lua-regex.h"

/* macro to `unsign' a character */
#define uchar(c)    ((unsigned char)(c))

#define L_ESC        '%'
#define SPECIALS    "^$*+?.([%-"

#define LUA_QL(x)    "'" x "'"
#define LUA_QS        LUA_QL("%s")

static ptrdiff_t posrelat (ptrdiff_t pos, size_t len) {
  /* relative string position: negative means back from end */
  if (pos < 0) pos += (ptrdiff_t)len;
  return (pos >= 0) ? pos : 0;
}

static int check_capture_all_closed (LuaMatchState *ms) {
  int i;
  for(i=0; i<ms->level; ++i){
      if(ms->capture[i].len == CAP_UNFINISHED){
          ms->error = "unfinished capture";
          return 0;
      }
  }
  return 1;
}

static int check_capture_is_closed (LuaMatchState *ms, int l) {
  if (l < 0 || l >= ms->level){
      ms->error = "invalid capture index";
      return 0;
  }
  if (ms->capture[l].len == CAP_UNFINISHED){
      ms->error = "unfinished capture";
      return 0;
  }
  return 1;
}

static int check_capture (LuaMatchState *ms, int *l_out) {
  int l;
  *l_out -= '1';
  l = *l_out;
  return check_capture_is_closed(ms, l);
}

static int capture_to_close (LuaMatchState *ms, int *level_out) {
  int level = ms->level;
  for (level--; level>=0; level--)
    if (ms->capture[level].len == CAP_UNFINISHED) {
        *level_out = level;
        return 1;
    }
  ms->error = "invalid pattern capture";
  return 0;
}


static int classend (LuaMatchState *ms, const char *p, const char **result) {
  switch (*p++) {
    case L_ESC: {
      if (p == ms->p_end){
          ms->error = "malformed pattern (ends with " LUA_QL("%%") ")";
          return 0;
      }
      *result = p+1;
      return 1;
    }
    case '[': {
      if (*p == '^') p++;
      do {  /* look for a `]' */
        if (p == ms->p_end){
            ms->error = "malformed pattern (missing " LUA_QL("]") ")";
            return 0;
        }
        if (*(p++) == L_ESC && p < ms->p_end)
          p++;  /* skip escapes (e.g. `%]') */
      } while (*p != ']');
      *result = p+1;
      return 1;
    }
    default: {
      *result = p;
      return 1;
    }
  }
}


static int match_class (int c, int cl) {
  int res;
  switch (tolower(cl)) {
    case 'a' : res = isalpha(c); break;
    case 'c' : res = iscntrl(c); break;
    case 'd' : res = isdigit(c); break;
    case 'g' : res = isgraph(c); break;
    case 'l' : res = islower(c); break;
    case 'p' : res = ispunct(c); break;
    case 's' : res = isspace(c); break;
    case 'u' : res = isupper(c); break;
    case 'w' : res = isalnum(c); break;
    case 'x' : res = isxdigit(c); break;
    case 'z' : res = (c == 0); break;  /* deprecated option */
    default: return (cl == c);
  }
  return (islower(cl) ? res : !res);
}


static int matchbracketclass (int c, const char *p, const char *ec) {
  int sig = 1;
  if (*(p+1) == '^') {
    sig = 0;
    p++;  /* skip the `^' */
  }
  while (++p < ec) {
    if (*p == L_ESC) {
      p++;
      if (match_class(c, uchar(*p)))
        return sig;
    }
    else if ((*(p+1) == '-') && (p+2 < ec)) {
      p+=2;
      if (uchar(*(p-2)) <= c && c <= uchar(*p))
        return sig;
    }
    else if (uchar(*p) == c) return sig;
  }
  return !sig;
}


static int singlematch (int c, const char *p, const char *ep) {
  switch (*p) {
    case '.': return 1;  /* matches any char */
    case L_ESC: return match_class(c, uchar(*(p+1)));
    case '[': return matchbracketclass(c, p, ep-1);
    default:  return (uchar(*p) == c);
  }
}


static const char *match (LuaMatchState *ms, const char *s, const char *p);

//add escape char extension from https://github.com/jcgoble3/lua-matchext
static const char *matchbalance (LuaMatchState *ms, const char *s,
                                   const char *p) {
  int escaped = (*(p-1) == 'B'); /* EXT */
  if (p >= ms->p_end - 1 - escaped){
    ms->error = "malformed pattern "
                      "(missing arguments to " LUA_QL("%%b") ")";
    return NULL;
  }
  if (*s != *p) return NULL;
  else {
    int b = *p;
    int e = *(p + (escaped ? 2 : 1));  /* EXT */
    int esc = escaped ? *(p + 1) : INT_MAX;  /* EXT */
    int cont = 1;
    while (++s < ms->src_end) {
      if (*s == esc) s++; /* EXT */
      else if (*s == e) {
        if (--cont == 0) return s+1;
      }
      else if (*s == b) cont++;
    }
  }
  return NULL;  /* string ends out of balance */
}


static const char *max_expand (LuaMatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  ptrdiff_t i = 0;  /* counts maximum expand for item */
  while ((s+i)<ms->src_end && singlematch(uchar(*(s+i)), p, ep))
    i++;
  /* keeps trying to match with the maximum repetitions */
  while (i>=0) {
    const char *res = match(ms, (s+i), ep+1);
    if (res) return res;
    i--;  /* else didn't match; reduce 1 repetition to try again */
  }
  return NULL;
}


static const char *min_expand (LuaMatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  for (;;) {
    const char *res = match(ms, s, ep+1);
    if (res != NULL)
      return res;
    else if (s<ms->src_end && singlematch(uchar(*s), p, ep))
      s++;  /* try with one more repetition */
    else return NULL;
  }
}


static const char *start_capture (LuaMatchState *ms, const char *s,
                                    const char *p, int what) {
  const char *res;
  int level = ms->level;
  if (level >= LUA_REGEX_MAXCAPTURES) {
      ms->error = "too many captures";
      return NULL;
  }
  ms->capture[level].init = s;
  ms->capture[level].len = what;
  ms->level = level+1;
  if ((res=match(ms, s, p)) == NULL)  /* match failed? */
    ms->level--;  /* undo capture */
  return res;
}


static const char *end_capture (LuaMatchState *ms, const char *s,
                                  const char *p) {
  int l;
  const char *res;
  if(!capture_to_close(ms, &l)) return NULL;
  ms->capture[l].len = s - ms->capture[l].init;  /* close capture */
  if ((res = match(ms, s, p)) == NULL)  /* match failed? */
    ms->capture[l].len = CAP_UNFINISHED;  /* undo capture */
  return res;
}


static const char *match_capture (LuaMatchState *ms, const char *s, int l) {
  size_t len;
  if(check_capture(ms, &l)){
      len = ms->capture[l].len;
      if ((size_t)(ms->src_end-s) >= len &&
          memcmp(ms->capture[l].init, s, len) == 0)
        return s+len;
  }
  return NULL;
}


static const char *match (LuaMatchState *ms, const char *s, const char *p) {
  init: /* using goto's to optimize tail recursion */
  if (p == ms->p_end)  /* end of pattern? */
    return s;  /* match succeeded */
  switch (*p) {
    case '(': {  /* start capture */
      if (*(p+1) == ')')  /* position capture? */
        return start_capture(ms, s, p+2, CAP_POSITION);
      else
        return start_capture(ms, s, p+1, CAP_UNFINISHED);
    }
    case ')': {  /* end capture */
      return end_capture(ms, s, p+1);
    }
    case '$': {
      if ((p+1) == ms->p_end)  /* is the `$' the last char in pattern? */
        return (s == ms->src_end) ? s : NULL;  /* check end of string */
      else goto dflt;
    }
    case L_ESC: {  /* escaped sequences not in the format class[*+?-]? */
      switch (*(p+1)) {
        case 'b': case 'B': { /* balanced string? */ /* EXT */
          s = matchbalance(ms, s, p+2);
          if (s == NULL) return NULL;
          p += (*(p + 1) == 'b') ? 4 : 5; /* EXT */ goto init;  /* else return match(ms, s, p+4); */
        }
        case 'f': {  /* frontier? */
          const char *ep; char previous;
          p += 2;
          if (*p != '['){
            ms->error = "missing " LUA_QL("[") " after "
                               LUA_QL("%%f") " in pattern";
            return NULL;
          }
          if(!classend(ms, p, &ep)) return NULL;  /* points to what is next */
          previous = (s == ms->src_init) ? '\0' : *(s-1);
          if (matchbracketclass(uchar(previous), p, ep-1) ||
             !matchbracketclass(uchar(*s), p, ep-1)) return NULL;
          p=ep; goto init;  /* else return match(ms, s, ep); */
        }
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9': {  /* capture results (%0-%9)? */
          s = match_capture(ms, s, uchar(*(p+1)));
          if (s == NULL) return NULL;
          p+=2; goto init;  /* else return match(ms, s, p+2) */
        }
        default: goto dflt;
      }
    }
    default: dflt: {  /* pattern class plus optional suffix */
      const char *ep;
	  int m;
      if(!classend(ms, p, &ep)) return NULL;  /* points to what is next */
      m = s < ms->src_end && singlematch(uchar(*s), p, ep);
      switch (*ep) {
        case '?': {  /* optional */
          const char *res;
          if (m && ((res=match(ms, s+1, ep+1)) != NULL))
            return res;
          p=ep+1; goto init;  /* else return match(ms, s, ep+1); */
        }
        case '*': {  /* 0 or more repetitions */
          return max_expand(ms, s, p, ep);
        }
        case '+': {  /* 1 or more repetitions */
          return (m ? max_expand(ms, s+1, p, ep) : NULL);
        }
        case '-': {  /* 0 or more repetitions (minimum) */
          return min_expand(ms, s, p, ep);
        }
        default: {
          if (!m) return NULL;
          s++; p=ep; goto init;  /* else return match(ms, s+1, ep); */
        }
      }
    }
  }
}


static const char *lmemfind (const char *s1, size_t l1,
                               const char *s2, size_t l2) {
  if (l2 == 0) return s1;  /* empty strings are everywhere */
  else if (l2 > l1) return NULL;  /* avoids a negative `l1' */
  else {
    const char *init;  /* to search for a `*s2' inside `s1' */
    l2--;  /* 1st char will be checked by `memchr' */
    l1 = l1-l2;  /* `s2' cannot be found after that */
    while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
      init++;   /* 1st char is already checked */
      if (memcmp(init, s2+1, l2) == 0)
        return init-1;
      else {  /* correct `l1' and `s1' to try again */
        l1 -= init-s1;
        s1 = init;
      }
    }
    return NULL;  /* not found */
  }
}


/* check whether pattern has no special characters */
static int nospecials (const char *p, size_t l) {
  size_t upto = 0;
  do {
    if (strpbrk(p + upto, SPECIALS))
      return 0;  /* pattern has a special character */
    upto += strlen(p + upto) + 1;  /* may have more after \0 */
  } while (upto <= l);
  return 1;  /* no special chars found */
}


static ptrdiff_t str_find_aux (LuaMatchState *ms, int find, const char *s, ptrdiff_t ls,
                         const char *p, ptrdiff_t lp, ptrdiff_t init, int raw_find,
                         luaregex_func_param fp, void *udata) {
  ptrdiff_t result;
  ms->error = NULL;
  if(ls < 0) ls = strlen(s);
  assert(ls >= 0);
  if(lp < 0) lp = strlen(p);
  assert(lp >= 0);
  init = posrelat(init, ls);
  if (init < 0) init = 0;
  else if (init > ls + 1) {  /* start after string's end? */
    return 0; /* cannot find anything */
  }
  ms->src_init = s;
  ms->src_end = s + ls;

do_again:
  result = -1; /* not found */
  /* explicit request or no special characters? */
  if (find && (raw_find || nospecials(p, lp))) {
    /* do a plain search */
    const char *s2 = lmemfind(s + init, ls - init, p, lp);
    if (s2) {
      ms->start_pos = ((int)(s2 - s));
      result = ms->end_pos = ms->start_pos+lp;
      ms->level = 0;
    }
  }
  else {
    const char *s1 = s + init;
    int anchor = (*p == '^');
    if (anchor) {
      p++; lp--;  /* skip anchor character */
    }
    ms->p_end = p + lp;
    do {
      const char *res;
      ms->level = 0;
      if ((res=match(ms, s1, p)) != NULL) {
          ms->start_pos = s1-s;
          result = ms->end_pos = res-s;
          goto eofunc;
      }
    } while (s1++ < ms->src_end && !anchor);
  }

eofunc:

  if(result >= 0){
      if(!check_capture_all_closed(ms)) return 0;
      if(fp && (*fp)(ms, udata, 0)) {
          init = result;
          if (init == ms->start_pos) ++init;  /* empty match? go at least one position */
          if (init < ls) goto do_again;
      }
  }
  return result > 0 ? ms->start_pos : result; //returning the start position
}


ptrdiff_t lua_str_find (LuaMatchState *ms, const char *s, ptrdiff_t ls,
              const char *p, ptrdiff_t lp, ptrdiff_t init, int raw_find,
              luaregex_func_param fp, void *udata) {
  return str_find_aux(ms, 1, s, ls, p, lp, init, raw_find, fp, udata);
}


ptrdiff_t lua_str_match (LuaMatchState *ms, const char *s, ptrdiff_t ls,
               const char *p, ptrdiff_t lp, ptrdiff_t init, int raw_find,
               luaregex_func_param fp, void *udata) {
  return str_find_aux(ms, 0, s, ls, p, lp, init, raw_find, fp, udata);
}

#define MIN_ALLOC_SIZE 2048
#define NEW_SIZE(sz) (((sz/MIN_ALLOC_SIZE)+1)*MIN_ALLOC_SIZE)

int char_buffer_add_char(LuaMatchState *ms, lua_char_buffer_st **b, char c){
    lua_char_buffer_st *tmp = *b;
    if(tmp->used+1 >= tmp->size){
        int new_size = tmp->size+MIN_ALLOC_SIZE;
        tmp = (lua_char_buffer_st*)realloc(tmp, sizeof(lua_char_buffer_st) + new_size);
        if(!tmp){
            ms->error = "not enough memory when reallocating";
            return 0;
        }
        *b = tmp;
        tmp->size = new_size;
    }
    tmp->buf[tmp->used++] = c;
    return 1;
}


int char_buffer_add_str(LuaMatchState *ms, lua_char_buffer_st **b, const char *str, ptrdiff_t len){
    lua_char_buffer_st *tmp = *b;
    if(len < 0) len = strlen(str);
    assert(len >= 0);
    if(tmp->used+len >= tmp->size){
        size_t new_size = tmp->size + NEW_SIZE(len);
        tmp = (lua_char_buffer_st*)realloc(tmp, sizeof(lua_char_buffer_st) + new_size);
        if(!tmp){
            ms->error = "not enough memory when reallocating";
            return 0;
        }
        *b = tmp;
        tmp->size = new_size;
    }
    memcpy(&tmp->buf[tmp->used], str, len);
    tmp->used += len;
    return 1;
}


static int add_value (LuaMatchState *ms, lua_char_buffer_st **b, const char *s,
                                       const char *e, const char *news, size_t lnews) {
  size_t i;
  for (i = 0; i < lnews; i++) {
    if (news[i] != L_ESC){
      if(!char_buffer_add_char(ms, b, news[i])) return 0;
    }
    else {
      i++;  /* skip ESC */
      if (!isdigit(uchar(news[i]))) {
        if (news[i] != L_ESC){
            ms->error = "invalid use of replacement string";
            return 0;
        }
        if(!char_buffer_add_char(ms, b, news[i])) return 0;
      }
      else if (news[i] == '0'){
          if(!char_buffer_add_str(ms, b, s, e - s)) return 0;
      }
      else {
          int il = news[i] - '1';
          if (il >= ms->level) {
            if (il == 0){  /* ms->level == 0, too */
              if(!char_buffer_add_str(ms, b, s, e - s)) return 0;  /* add whole match */
            }
            else{
                ms->error = "invalid capture index";
                return 0;
            }
          }
          else {
            ptrdiff_t cl = ms->capture[il].len;
            if (cl == CAP_UNFINISHED) {
                ms->error = "unfinished capture";
                return 0;
            }
            if (cl == CAP_POSITION){
                char buf[32];
                snprintf(buf, sizeof(buf), "%" PRIdPTR, ms->capture[il].init - ms->src_init + 1);
                if(!char_buffer_add_str(ms, b, buf, strlen(buf))) return 0;
            }
            else
              if(!char_buffer_add_str(ms, b, ms->capture[il].init, cl)) return 0;
          }
      }
    }
  }
  return 1;
}

lua_char_buffer_st *lua_str_gsub (const char *src, ptrdiff_t srcl, const char *p, ptrdiff_t lp,
                          const char *tr, ptrdiff_t ltr, size_t max_s, const char **error_ptr,
                          luaregex_func_param fp, void *udata) {
  int anchor;
  size_t n;
  LuaMatchState ms;
  lua_char_buffer_st *b;
  if(srcl < 0) srcl = strlen(src);
  assert(srcl >= 0);
  if(lp < 0) lp = strlen(p);
  assert(lp >= 0);
  if(ltr < 0) ltr = strlen(tr);
  assert(ltr >= 0);
  if(max_s == 0) max_s = srcl+1;
  anchor = (*p == '^');
  n = NEW_SIZE(srcl);
  b = (lua_char_buffer_st*)malloc(sizeof(lua_char_buffer_st) + n);
  if(!b) return NULL;
  b->size = n;
  b->used = 0;

  n = 0;
  if (anchor) {
    p++; lp--;  /* skip anchor character */
  }
  ms.error = 0;
  ms.start_pos = ms.end_pos = 0;
  ms.src_init = src;
  ms.src_end = src+srcl;
  ms.p_end = p + lp;
  while (n < max_s) {
    const char *e;
    ms.level = 0;
    e = match(&ms, src, p);
    if(ms.error || !check_capture_all_closed(&ms)) goto free_and_null;
    if (e) {
      n++;
      if(fp){
          ms.end_pos = e-ms.src_init;
          if(!(*fp)(&ms, udata, &b)) goto free_and_null;
      }
      else if(!add_value(&ms, &b, src, e, tr, ltr)) goto free_and_null;
    }
    if (e && e>src){ /* non empty match? */
      ms.start_pos = e-ms.src_init;
      src = e;  /* skip it */
    }
    else if (src < ms.src_end){
      if(!char_buffer_add_char(&ms, &b, *src++)) goto free_and_null;
      ++ms.start_pos;
    }
    else break;
    if (anchor) break;
  }
  if(!char_buffer_add_str(&ms, &b, src, ms.src_end-src)) goto free_and_null;
  b->buf[b->used] = '\0';
  return b;

free_and_null:
  if(b) free(b);
  if(error_ptr) *error_ptr = ms.error;
  return NULL;
}

