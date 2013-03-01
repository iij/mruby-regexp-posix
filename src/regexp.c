/*
** regexp.c - Regular Expression
**
*/

#include "mruby.h"
#include "mruby/array.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/string.h"
#include "mruby/variable.h"

#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  MRB_REGEXP_IGNORECASE = 1
};

struct mrb_matchdata {
  regmatch_t *pmatch;
  unsigned int nsub;
};

struct mrb_regexp {
  regex_t preg;
};

void
mrb_matchdata_free(mrb_state *mrb, void *ptr)
{
  struct mrb_matchdata *mmd = ptr;

  if (mmd->pmatch) {
    mrb_free(mrb, mmd->pmatch);
  }
  mrb_free(mrb, mmd);
}

void
mrb_regexp_free(mrb_state *mrb, void *ptr)
{
  struct mrb_regexp *mrb_re = ptr;

  //regfree(&mrb_re->preg);  // XXX: check we can free it
  mrb_free(mrb, mrb_re);
}

static struct mrb_data_type mrb_matchdata_type = { "MatchData", mrb_matchdata_free };
static struct mrb_data_type mrb_regexp_type = { "Regexp", mrb_regexp_free };


mrb_value
mrb_matchdata_init(mrb_state *mrb, mrb_value self)
{
  struct mrb_matchdata *mrb_md;

  mrb_md = (struct mrb_matchdata *)mrb_get_datatype(mrb, self, &mrb_matchdata_type);
  if (mrb_md) {
    mrb_matchdata_free(mrb, mrb_md);
  }

  mrb_md = (struct mrb_matchdata *)mrb_malloc(mrb, sizeof(*mrb_md));
  mrb_md->pmatch = NULL;
  mrb_md->nsub = 0;

  DATA_PTR(self) = mrb_md;
  DATA_TYPE(self) = &mrb_matchdata_type;

  return self;
}

mrb_value
mrb_matchdata_begin(mrb_state *mrb, mrb_value self)
{
  struct mrb_matchdata *mrb_md;
  mrb_int n;

  mrb_md = (struct mrb_matchdata *)mrb_get_datatype(mrb, self, &mrb_matchdata_type);
  if (!mrb_md) return mrb_nil_value();

  mrb_get_args(mrb, "i", &n);
  if (n < 0 || n >= mrb_md->nsub)
    mrb_raisef(mrb, E_INDEX_ERROR, "index %d out of matches", n);

  return mrb_fixnum_value((mrb_int)mrb_md->pmatch[n].rm_so);
}

mrb_value
mrb_matchdata_end(mrb_state *mrb, mrb_value self)
{
  struct mrb_matchdata *mrb_md;
  mrb_int n;

  mrb_md = (struct mrb_matchdata *)mrb_get_datatype(mrb, self, &mrb_matchdata_type);
  if (!mrb_md) return mrb_nil_value();

  mrb_get_args(mrb, "i", &n);
  if (n < 0 || n >= mrb_md->nsub)
    mrb_raisef(mrb, E_INDEX_ERROR, "index %d out of matches", n);

  return mrb_fixnum_value((mrb_int)mrb_md->pmatch[n].rm_eo);
}

mrb_value
mrb_regexp_init(mrb_state *mrb, mrb_value self)
{
  struct mrb_regexp *mrb_re;
  mrb_value option, str;
  int error;

  mrb_re = (struct mrb_regexp *)mrb_get_datatype(mrb, self, &mrb_regexp_type);
  if (mrb_re) {
    mrb_regexp_free(mrb, mrb_re);
  }

  mrb_get_args(mrb, "S|o", &str, &option);
  // XXX: str_to_cstr
  // XXX: opts

  mrb_re = (struct mrb_regexp *)mrb_malloc(mrb, sizeof(*mrb_re));
  DATA_PTR(self) = mrb_re;
  DATA_TYPE(self) = &mrb_regexp_type;

  error = regcomp(&mrb_re->preg, RSTRING_PTR(str), REG_EXTENDED);
  if (error != 0) { // XXX: raise RegexpError
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "regcomp failed: %d", error);
  }
  mrb_iv_set(mrb, self, mrb_intern(mrb, "@source"), mrb_str_dup(mrb, str));
  mrb_iv_set(mrb, self, mrb_intern(mrb, "@options"), mrb_fixnum_value(0));

  return self;
}

mrb_value
mrb_regexp_match(mrb_state *mrb, mrb_value self)
{
  struct RClass *c;
  struct mrb_matchdata *mrb_md;
  struct mrb_regexp *mre;
  mrb_int pos;
  mrb_value md, str;
  regmatch_t *pmatch;
  int nsub;

  mre = (struct mrb_regexp *)mrb_get_datatype(mrb, self, &mrb_regexp_type);
  if (!mre) return mrb_nil_value();

  pos = 0;
  mrb_get_args(mrb, "S|i", &str, &pos);

  nsub = mre->preg.re_nsub + 1;
  pmatch = mrb_malloc(mrb, nsub * sizeof(regmatch_t));

  if (regexec(&mre->preg, RSTRING_PTR(str), nsub, pmatch, 0) != 0) {
    // any errors?
    mrb_free(mrb, pmatch);
    return mrb_nil_value();
  }

  c = mrb_class_get(mrb, "MatchData");
  md = mrb_funcall(mrb, mrb_obj_value(c), "new", 0);

  mrb_md = (struct mrb_matchdata *)mrb_get_datatype(mrb, md, &mrb_matchdata_type);
  mrb_md->pmatch = pmatch;
  mrb_md->nsub = nsub;

  mrb_iv_set(mrb, md, mrb_intern(mrb, "@length"), mrb_fixnum_value(nsub));
  mrb_iv_set(mrb, md, mrb_intern(mrb, "@regexp"), self);
  mrb_iv_set(mrb, md, mrb_intern(mrb, "@string"), mrb_str_dup(mrb, str));

  return md;
}

void
mrb_mruby_regexp_posix_gem_init(mrb_state *mrb)
{
  struct RClass *regexp, *matchdata;

  regexp = mrb_define_class(mrb, "Regexp", mrb->object_class);
  MRB_SET_INSTANCE_TT(regexp, MRB_TT_DATA);

  mrb_define_method(mrb, regexp, "initialize", mrb_regexp_init, ARGS_REQ(1)|ARGS_OPT(2));
  mrb_define_method(mrb, regexp, "match", mrb_regexp_match, ARGS_REQ(1)|ARGS_OPT(1));

  mrb_define_const(mrb, regexp, "IGNORECASE", mrb_fixnum_value(MRB_REGEXP_IGNORECASE));

  matchdata = mrb_define_class(mrb, "MatchData", mrb->object_class);
  MRB_SET_INSTANCE_TT(matchdata, MRB_TT_DATA);

  mrb_define_method(mrb, matchdata, "initialize", mrb_matchdata_init, ARGS_NONE());
  mrb_define_method(mrb, matchdata, "begin", mrb_matchdata_begin, ARGS_REQ(1));
  mrb_define_method(mrb, matchdata, "end", mrb_matchdata_end, ARGS_REQ(1));
}

void
mrb_mruby_regexp_posix_gem_final(mrb_state *mrb)
{
}
