/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "sfidl-module.h"
#include "sfidl-namespace.h"

#include <string.h>
#include <stdio.h>
#include <list>
#include <string>
#include <map>
#include <sfi/glib-extra.h>

using namespace std;
using namespace Sfidl;

static const gchar*
TypeName (const string &str)
{
  int pos = str.rfind (':');
  if (pos < 0)  // FIXME: not fully qualified, prolly an Sfi type (it's a shame that fundamental types aren't fully qualified)
    return g_intern_string (str == "void" ? "void" : ("Sfi" + str).c_str());
  return g_intern_string (str.substr (pos + 1).c_str());
}

static const gchar*
canonify_name (const string& s,
               const char    replace = '-')
{
  /* canonify type names which contain e.g. underscores (procedures) */
  gchar *tmp = g_strcanon (g_strdup (s.c_str()),
                           G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "+",
                           replace);
  string d = tmp;
  g_free (tmp);
  return g_intern_string (d.c_str());
}

static string
Qualified (const string &str)
{
  int pos = str.rfind (':');
  if (pos < 0)  // not fully qualified, prolly an Sfi type
    return TypeName (str);
  // return fully C++ qualified name
  return str;
}
#define cQualified(s)    Qualified (s).c_str()

const gchar*
CodeGeneratorModule::TypeRef (const string &type)
{
  string tname = Qualified (type);
  switch (parser.typeOf (type))
    {
    case BBLOCK:
    case FBLOCK:
    case SEQUENCE:
    case RECORD:
    case OBJECT:        return g_intern_string (string (tname + "*").c_str());
    default:            return g_intern_string (tname.c_str());
    }
}

string
CodeGeneratorModule::createTypeCode (const string& type, const string& name, TypeCodeModel model)
{
  /* currently, no special casing required */
  return CodeGeneratorCxxBase::createTypeCode (type, name, model);
}

const gchar*
CodeGeneratorModule::TypeField (const string& type)
{
  return cTypeField(type);
}

static vector<string>
split_string (const string &ctype)
{
  vector<string> vs;
  string type = ctype;
  int i;
  while ((i = type.find (':')) >= 0)
    {
      vs.push_back (type.substr (0, i));
      if (type[i + 1] == ':')
        type = type.substr (i + 2);
      else
        type = type.substr (i + 1);
    }
  vs.push_back (type);
  return vs;
}

static string
join_string (const vector<string> &vs,
             const string         &delim)
{
  string r;
  for (vector<string>::const_iterator vi = vs.begin(); vi != vs.end(); vi++)
    {
      if (vi != vs.begin())
        r += delim;
      r += *vi;
    }
  return r;
}

static string
UC_NAME (const string &cstr)
{
  vector<string> vs = split_string (cstr);
  string str = join_string (vs, "_");
  string r;
  char l = 0;
  for (string::const_iterator i = str.begin(); i != str.end(); i++)
    {
      if (islower (l) && isupper (*i))
        r += "_";
      r += toupper (l = *i);
    }
  return r;
}
#define cUC_NAME(s)    UC_NAME (s).c_str()

static const gchar*
TYPE_NAME (const string &type)
{
  vector<string> vs = split_string (type);
  string tname = vs.back();
  vs.pop_back();
  string nspace = join_string (vs, ":");
  string NAME_SPACE = UC_NAME (nspace);
  if (NAME_SPACE == "") // FIXME: need to special case not fully qualified fundamentals (Sfi types) here ;-(
    NAME_SPACE = "SFI";
  string result = NAME_SPACE + "_TYPE_" + cUC_NAME (tname);
  return g_intern_string (result.c_str());
}

static string
include_relative (string path,
                  string source_file)
{
  if (g_path_is_absolute (path.c_str()))
    return path;
  gchar *dir = g_path_get_dirname (source_file.c_str());
  string apath = string(dir) + G_DIR_SEPARATOR_S + path;
  g_free (dir);
  return apath;
}

string
CodeGeneratorModule::pspec_constructor (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case OBJECT:
      {
	const string group = (param.group != "") ? param.group.escaped() : "NULL";
        string pspec = "sfidl_pspec_Object";
        if (param.args == "")
          pspec += "_default";
        pspec += " (" + group + ", \"" + param.name + "\", ";
        if (param.args != "")
          pspec += param.args + ", ";
        vector<string> vs = split_string (param.type);
        string pname = vs.back();
        vs.pop_back();
        string nspace = join_string (vs, ":");
        pspec += UC_NAME (nspace) + "_TYPE_" + cUC_NAME (pname);
        pspec += ")";
        return pspec;
      }
    case CHOICE:
      {
	const string group = (param.group != "") ? param.group.escaped() : "NULL";
        string pspec = "sfidl_pspec_GEnum";
        if (param.args == "")
          pspec += "_default";
        pspec += " (" + group + ", \"" + param.name + "\", ";
        if (param.args != "")
          pspec += param.args + ", ";
        vector<string> vs = split_string (param.type);
        string pname = vs.back();
        vs.pop_back();
        string nspace = join_string (vs, ":");
        pspec += UC_NAME (nspace) + "_TYPE_" + cUC_NAME (pname);
        pspec += ")";
        return pspec;
      }
    default:    return makeParamSpec (param);
    }
}

const char*
CodeGeneratorModule::func_value_set_param (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case BOOL:          return "sfi_value_set_bool";
    case INT:           return "sfi_value_set_int";
    case NUM:           return "sfi_value_set_num";
    case REAL:          return "sfi_value_set_real";
    case STRING:        return "sfi_value_set_cxxstring";
    case CHOICE:        return "g_value_set_enum";
    case BBLOCK:        return "sfi_value_set_bblock";
    case FBLOCK:        return "sfi_value_set_fblock";
    case SEQUENCE:      return "sfi_value_set_seq";
    case RECORD:        return "sfi_value_set_rec";
    case OBJECT:        return "g_value_set_object";
    default:            return "*** ERROR ***";
    }
}

static const char*
abs_cxx_type_name (const string &dest)
{
  return g_intern_string (string(string ("::") + dest).c_str());
}

string
CodeGeneratorModule::func_value_get_param (const Param &param,
                                           const string dest = "")
{
  switch (parser.typeOf (param.type))
    {
    case BOOL:          return "sfi_value_get_bool";
    case INT:           return "sfi_value_get_int";
    case NUM:           return "sfi_value_get_num";
    case REAL:          return "sfi_value_get_real";
    case STRING:        return "sfi_value_get_cxxstring";
    case CHOICE:        return string ("(") + TypeName (param.type) + ") g_value_get_enum";
    case BBLOCK:        return "sfi_value_get_bblock";
    case FBLOCK:        return "sfi_value_get_fblock";
    case SEQUENCE:      return "sfi_value_get_seq";
    case RECORD:        return "sfi_value_get_rec";
    case OBJECT:
      if (dest != "")
        return "("+ dest +"*) ::Bse::g_value_get_object< "+ dest +"Base*>";
      else
        return "(GObject*) g_value_get_object";
    default:            return "*** ERROR ***";
    }
}

string
CodeGeneratorModule::func_value_dup_param (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case STRING:        return "sfi_value_dup_string";
    case BBLOCK:        return "sfi_value_dup_bblock";
    case FBLOCK:        return "sfi_value_dup_fblock";
    case SEQUENCE:      return "sfi_value_dup_seq";
    case RECORD:        return "sfi_value_dup_rec";
    case OBJECT:        return "g_value_dup_object";
    default:            return func_value_get_param (param);
    }
}

string
CodeGeneratorModule::func_param_return_free (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case BOOL:          return "";
    case INT:           return "";
    case NUM:           return "";
    case REAL:          return "";
    case STRING:        return "";
    case CHOICE:        return "";      /* enum value */
    case BBLOCK:        return "sfi_bblock_unref";
    case FBLOCK:        return "sfi_fblock_unref";
    case SEQUENCE:      return "sfi_seq_unref";
    case RECORD:        return "";
    case OBJECT:        return "";
    default:            return "*** ERROR ***";
    }
}

string
CodeGeneratorModule::func_param_free (const Param &param)
{
  switch (parser.typeOf (param.type))
    {
    case OBJECT:        return "g_object_unref";
    default:            return func_param_return_free (param);
    }
}

void
CodeGeneratorModule::generate_procedures (const std::string          &outer_nspace,
                                          std::vector<const Method*> &procs,
                                          std::vector<Image>         &images)
{
  /* generate procedures */
  printf ("\n/* procedure classes */\n");
  printf ("namespace Procedure {\n");
  for (vector<const Method*>::const_iterator ppi = procs.begin(); ppi != procs.end(); ppi++)
    {
      printf ("\n");
      const Method *mi = *ppi;  // FIXME: things containing maps shouldn't be constant
      const Map<std::string, IString> &infos = mi->infos;
      string ptName = string ("") + TypeName (mi->name);
      string ptFullName = g_type_name_to_sname ((outer_nspace + "-" + ptName).c_str());
      bool is_void = mi->result.type == "void";
      if (parser.fromInclude (mi->name))
        continue;
      
      /* skeleton class declaration + type macro */
      printf ("#define %s_TYPE_%s (BSE_CXX_DECLARED_PROC_TYPE (%s))\n",
              cUC_NAME (outer_nspace), cUC_NAME (TypeName (mi->name)), ptName.c_str());
      printf ("BSE_CXX_DECLARE_PROC (%s);\n", ptName.c_str());
      printf ("class %s {\n", ptName.c_str());
      
      /* class Info strings */
      /* pixstream(), this is a bit of a hack, we make it a template rather than
       * a normal inline method to avoid huge images in debugging code
       */
      string icon = infos.get("icon");
      string pstream = "NULL";
      if (icon != "")
        {
          printf ("  template<bool> static inline const unsigned char* inlined_pixstream();\n");
          images.push_back (Image (include_relative (icon, mi->file),
                                   "template<bool> const unsigned char*\n" +
                                   ptName +
                                   "::inlined_pixstream()"));
          pstream = "inlined_pixstream<true>()";
        }
      printf ("public:\n");
      printf ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
      printf ("  static inline const char* options   () { return %s; }\n", infos.get("options").escaped().c_str());
      printf ("  static inline const char* category  () { static const char *c = NULL; \n");
      printf ("    const char *root_category = %s, *category = %s;\n",
              infos.get("root_category").escaped().c_str(), infos.get("category").escaped().c_str());
      printf ("    if (!c && category[0]) c = g_intern_strconcat (root_category && root_category[0] ? root_category : \"/Proc\",\n");
      printf ("                                    category[0] == '/' ? \"\" : \"/\", category, NULL);\n");
      printf ("    return c; }\n");
      printf ("  static inline const char* blurb     () { return %s; }\n", infos.get("blurb").escaped().c_str());
      printf ("  static inline const char* authors   () { return %s; }\n", infos.get("authors").escaped().c_str());
      printf ("  static inline const char* license   () { return %s; }\n", infos.get("license").escaped().c_str());
      printf ("  static inline const char* type_name () { return \"%s\"; }\n", ptFullName.c_str());
      
      /* return type */
      printf ("  static %s exec (", TypeRef (mi->result.type));
      /* args */
      for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
        {
          if (ai != mi->params.begin())
            printf (", ");
          printf ("%s %s", TypeRef (ai->type), ai->name.c_str());
        }
      printf (");\n");
      
      /* marshal */
      printf ("  static BseErrorType marshal (BseProcedureClass *procedure,\n"
              "                               const GValue      *in_values,\n"
              "                               GValue            *out_values)\n");
      printf ("  {\n");
      printf ("    try {\n");
      if (!is_void)
        printf ("      %s __return_value =\n", TypeRef (mi->result.type));
      printf ("        exec (\n");
      int i = 0;
      for (vector<Param>::const_iterator pi = mi->params.begin(); pi != mi->params.end(); pi++)
        printf ("              %s (in_values + %u)%c\n", func_value_get_param (*pi, abs_cxx_type_name (pi->type)).c_str(), i++,
                &(*pi) == &(mi->params.back()) ? ' ' : ',');
      printf ("             );\n");
      if (!is_void)
        printf ("      %s (out_values, __return_value);\n", func_value_set_param (mi->result));
      if (!is_void && func_param_return_free (mi->result) != "")
        printf ("      if (__return_value) %s (__return_value);\n", func_param_return_free (mi->result).c_str());
      printf ("    } catch (std::exception &e) {\n");
      printf ("      sfi_debug (\"%%s: %%s\", \"%s\", e.what());\n", ptName.c_str());
      printf ("      return BSE_ERROR_PROC_EXECUTION;\n");
      printf ("    } catch (...) {\n");
      printf ("      sfi_debug (\"%%s: %%s\", \"%s\", \"unknown exception\");\n", ptName.c_str());
      printf ("      return BSE_ERROR_PROC_EXECUTION;\n");
      printf ("    }\n");
      printf ("    return BSE_ERROR_NONE;\n");
      printf ("  }\n");
      
      /* init */
      printf ("  static void init (BseProcedureClass *proc,\n"
              "                    GParamSpec       **in_pspecs,\n"
              "                    GParamSpec       **out_pspecs)\n");
      printf ("  {\n");
      for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
        printf ("    *(in_pspecs++) = %s;\n", pspec_constructor (*ai).c_str());
      if (!is_void)
        printf ("    *(out_pspecs++) = %s;\n", pspec_constructor (mi->result).c_str());
      printf ("  }\n");
      
      /* done */
      printf ("};\n"); /* finish: class ... { }; */
    }
  printf ("\n}; // Procedure\n");
}

void
CodeGeneratorModule::run ()
{
  string nspace = "Foo";
  vector<Image> images;
  std::vector<const Method*> procs;
  
  printf("\n/*-------- begin %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());

  /* standard includes */
  printf ("\n#include <bse/bsecxxplugin.h>\n");
  printf ("#include <bse/bsecxxsmart.h>\n");
  
  /* sigh, we can't query things by namespace from the parser. // FIXME
   * so here's a gross hack, figure the namespace from the
   * first class to output (cross fingers there is any) and
   * assume the rest goes into the same namespace ;-(
   */
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    if (!parser.fromInclude (ci->name))
      {
        nspace = ci->name.substr (0, ci->name.find (':'));
        break;
      }
  printf ("\nnamespace %s {\n", nspace.c_str());

  /* class prototypes */
  printf ("\n/* class prototypes */\n");
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      /* class prototypes */
      printf ("class %s%s;\n", TypeName (ci->name), "Base");
      printf ("class %s;\n", TypeName (ci->name));
    }
  
  /* enumerations */
  printf ("\n/* choice/enum types */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;

      printf ("#define %s_TYPE_%s (BSE_CXX_DECLARED_ENUM_TYPE (%s))\n",
              cUC_NAME (nspace), cUC_NAME (TypeName (ci->name)), TypeName (ci->name));
      printf ("enum %s {\n", TypeName (ci->name));
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        printf ("  %s = %d,\n", cUC_NAME (vi->name), vi->sequentialValue );
      printf ("};\n");
      printf ("BSE_CXX_DECLARE_ENUM (%s, \"%s\" \"%s\", %u,\n",
              TypeName (ci->name), nspace.c_str(), TypeName (ci->name), ci->contents.size());
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        printf ("                      *v++ = ::Bse::EnumValue (%d, \"%s\", \"%s\" );\n",
                vi->sequentialValue, cUC_NAME (vi->name), vi->text.c_str());
      printf ("                      );\n");
    }

  printf ("\n}; /* namespace %s */\n", nspace.c_str()); /* XXX */

  /* records and sequences */

  /*
   * FIXME: we follow the declaration order of the idl file for generating records and sequences.
   * This is quite good, as if no prototypes are used, we won't refer to undefined types.
   * However, this breaks with prototypes.
   */
  NamespaceHelper nsh(stdout);
  printf ("\n/* record/sequence types */\n");

  /* prototypes for records */
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
  {
    if (parser.fromInclude (ri->name)) continue;

    nsh.setFromSymbol(ri->name);
    string name = nsh.printableForm (ri->name);

    printf("\n");
    printf("class %s;\n", name.c_str());
    printf("typedef Bse::SmartPtr<%s,CountablePointer<RefCountable> > %sPtr;\n",
	name.c_str(), name.c_str());
    printf("typedef Bse::SmartPtr<const %s,CountablePointer<const RefCountable> > %sCPtr;\n",
	name.c_str(), name.c_str());
  }

  /* records */
  bool first = true;
  for(vector<string>::const_iterator ti = parser.getTypes().begin(); ti != parser.getTypes().end(); ti++)
    {
      if (parser.fromInclude (*ti)) continue;

      if (parser.isRecord (*ti) || parser.isSequence (*ti))
	{
	  if(!first) printf("\n");
	  first = false;
	}

      if (parser.isRecord (*ti))
	{
	  const Record& rdef = parser.findRecord (*ti);

	  nsh.setFromSymbol(rdef.name);
	  string name = nsh.printableForm (rdef.name);

	  printf("class %s : public RefCountable {\n", name.c_str());
	  printf("public:\n");
	  for (vector<Param>::const_iterator pi = rdef.contents.begin(); pi != rdef.contents.end(); pi++)
	    {
	      printf("  %s %s;\n", TypeField(pi->type), pi->name.c_str());
	    }
	  printf("  static %sPtr _from_rec (SfiRec *rec) { return 0; }\n", name.c_str());
	  printf("  static SfiRec *_to_rec (%sPtr ptr) { return 0; }\n", name.c_str());
	  printf("};\n");
	}
      if (parser.isSequence (*ti))
	{
	  const Sequence& sdef = parser.findSequence (*ti);

	  printf("//%s\n", sdef.name.c_str());
#if 0
	  string name = makeLowerName (sdef.name);
	  // int f = 0;

	  if (options.generateIdlLineNumbers)
	    printf("#line %u \"%s\"\n", sdef.content.line, parser.fileName().c_str());
	  printf("  %s_content = %s;\n", name.c_str(), makeParamSpec (sdef.content).c_str());
#endif
	}
    }

  nsh.leaveAll();

  printf ("\nnamespace %s {\n", nspace.c_str()); /* XXX */

  /* class definitions */
  printf ("\n/* classes */\n");
  for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
    {
      string ctName = TypeName (ci->name);
      string ctFullName = nspace + ctName;
      const char* FULL_TYPE_NAME = g_intern_string ((UC_NAME (nspace) + "_TYPE_" + UC_NAME (TypeName (ci->name))).c_str());
      string ctNameBase = TypeName (ci->name) + string ("Base");
      string ctProperties = TypeName (ci->name) + string ("Properties");
      string ctPropertyID = TypeName (ci->name) + string ("PropertyID");
      vector<string> destroy_jobs;
      if (parser.fromInclude (ci->name))
        continue;
      
      /* skeleton class declaration + type macro */
      printf ("#define %s (BSE_CXX_DECLARED_CLASS_TYPE (%s))\n", FULL_TYPE_NAME, TypeName (ci->name));
      printf ("BSE_CXX_DECLARE_CLASS (%s);\n", TypeName (ci->name));
      printf ("class %s : public ::%s {\n", ctNameBase.c_str(), cQualified (ci->inherits));
      
      /* class Info strings */
      /* pixstream(), this is a bit of a hack, we make it a template rather than
       * a normal inline method to avoid huge images in debugging code
       */
      string icon = ci->infos.get("icon");
      string pstream = "NULL";
      if (icon != "")
        {
          printf ("  template<bool> static inline const unsigned char* inlined_pixstream();\n");
          images.push_back (Image (include_relative (icon, ci->file),
                                   "template<bool> const unsigned char*\n" +
                                   ctNameBase +
                                   "::inlined_pixstream()"));
          pstream = "inlined_pixstream<true>()";
        }
      printf ("public:\n");
      printf ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
      printf ("  static inline const char* options   () { return %s; }\n", ci->infos.get("options").escaped().c_str());
      printf ("  static inline const char* category  () { static const char *c = NULL; const char *category = %s; \n",
              ci->infos.get("category").escaped().c_str());
      printf ("    if (!c && category[0]) c = g_intern_strconcat (\"/Modules\", category[0] == '/' ? \"\" : \"/\", category, NULL);\n");
      printf ("    return c; }\n");
      printf ("  static inline const char* blurb     () { return %s; }\n", ci->infos.get("blurb").escaped().c_str());
      printf ("  static inline const char* authors   () { return %s; }\n", ci->infos.get("authors").escaped().c_str());
      printf ("  static inline const char* license   () { return %s; }\n", ci->infos.get("license").escaped().c_str());
      printf ("  static inline const char* type_name () { return \"%s\"; }\n", ctFullName.c_str());
      
      /* i/j/o channel names */
      int is_public = 0;
      if (ci->istreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  enum {\n");
          for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
            printf ("    ICHANNEL_%s,\n", cUC_NAME (si->ident));
          printf ("    N_ICHANNELS\n  };\n");
        }
      if (ci->jstreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  enum {\n");
          for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
            printf ("    JCHANNEL_%s,\n", cUC_NAME (si->ident));
          printf ("    N_JCHANNELS\n  };\n");
        }
      if (ci->ostreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  enum {\n");
          for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
            printf ("    OCHANNEL_%s,\n", cUC_NAME (si->ident));
          printf ("    N_OCHANNELS\n  };\n");
        }

      /* "Properties" structure for synthesis modules */
      if (ci->properties.size() && ci->istreams.size() + ci->jstreams.size() + ci->ostreams.size())
        {
          if (!is_public++)
            printf ("public:\n");
          printf ("  /* \"transport\" structure to configure synthesis modules from properties */\n");
          printf ("  struct %s {\n", ctProperties.c_str());
          for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
            printf ("    %s %s;\n", TypeRef (pi->type), pi->name.c_str());
          printf ("    explicit %s (%s *p) ", ctProperties.c_str(), ctNameBase.c_str());
          for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
            printf ("%c\n      %s (p->%s)", pi == ci->properties.begin() ? ':' : ',', pi->name.c_str(), pi->name.c_str());
          printf ("\n    {\n");
          printf ("    }\n");
          printf ("  };\n");
        }
      
      /* property fields */
      printf ("protected:\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        printf ("  %s %s;\n", TypeRef (pi->type), pi->name.c_str());
      
      /* property IDs */
      if (ci->properties.begin() != ci->properties.end())
        {
          printf ("protected:\n  enum %s {\n", ctPropertyID.c_str());
          vector<Param>::const_iterator pi = ci->properties.begin();
          printf ("    PROP_%s = 1,\n", cUC_NAME (pi->name));
          for (pi++; pi != ci->properties.end(); pi++)
            printf ("    PROP_%s,\n", cUC_NAME (pi->name));
          printf ("  };\n");
        }
      
      /* property setter */
      printf ("public:\n");
      printf ("  void set_property (guint prop_id, const ::Bse::Value &value, GParamSpec *pspec)\n");
      printf ("  {\n");
      printf ("    switch (prop_id) {\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        {
          printf ("    case PROP_%s:\n", cUC_NAME (pi->name));
          string f = func_param_free (*pi).c_str();
          if (f.size())
            printf ("      %s (%s);\n", f.c_str(), pi->name.c_str());
          printf ("      %s = %s (&value);\n", pi->name.c_str(), func_value_dup_param (*pi).c_str());
          printf ("    break;\n");
        }
      printf ("    };\n");
      printf ("    property_changed ((%s) prop_id);\n", ctPropertyID.c_str());
      printf ("    update_modules();\n");
      /* reset triggers */
      printf ("    switch (prop_id) {\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        {
          if (pi->pspec != "Trigger")
            continue;
          printf ("    case PROP_%s:\n", cUC_NAME (pi->name));
          printf ("      %s = FALSE;\n", pi->name.c_str());
          printf ("    break;\n");
        }
      printf ("    default: ;\n");
      printf ("    };\n");
      printf ("  }\n");
      
      /* property getter */
      printf ("  void get_property (guint prop_id, ::Bse::Value &value, GParamSpec *pspec)\n");
      printf ("  {\n");
      printf ("    switch (prop_id) {\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        {
          printf ("    case PROP_%s:\n", cUC_NAME (pi->name));
          printf ("      %s (&value, %s);\n", func_value_set_param (*pi), pi->name.c_str());
          printf ("    break;\n");
        }
      printf ("    };\n");
      printf ("  }\n");

      /* static data */
      printf ("private:\n");
      printf ("  static struct StaticData {\n");
      printf ("    int dummy;\n");
      for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
        {
          const gchar *sig_name = canonify_name (si->name, '_');
          printf ("    guint signal_%s;\n", sig_name);
        }
      printf ("  } static_data;\n");

      /* property-changed hooking */
      printf ("protected:\n");
      printf ("  virtual void property_changed (%s) {}\n", ctPropertyID.c_str());

      /* methods */
      for (vector<Method>::const_iterator mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
        procs.push_back (&(*mi));
      
      /* destructor */
      printf ("  virtual ~%s ()\n", ctNameBase.c_str());
      printf ("  {\n");
      /* property deletion */
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        {
          string f = func_param_free (*pi).c_str();
          if (f.size())
            printf ("    %s (%s);\n", f.c_str(), pi->name.c_str());
        }
      for (vector<string>::const_iterator vi = destroy_jobs.begin(); vi != destroy_jobs.end(); vi++)
        printf ("    %s;\n", vi->c_str());
      printf ("  }\n");

      /* class_init */
      printf ("public:\n");
      printf ("  static void class_init (::Bse::CxxBaseClass *klass)\n");
      printf ("  {\n");
      for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
        printf ("    klass->add_param (PROP_%s, %s);\n", cUC_NAME (pi->name), pspec_constructor (*pi).c_str());
      for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
        printf ("    klass->add_ichannel (%s, %s, ICHANNEL_%s);\n",
                si->name.escaped().c_str(), si->blurb.escaped().c_str(), cUC_NAME (si->ident));
      for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
        printf ("    klass->add_jchannel (%s, %s, JCHANNEL_%s);\n",
                si->name.escaped().c_str(), si->blurb.escaped().c_str(), cUC_NAME (si->ident));
      for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
        printf ("    klass->add_ochannel (%s, %s, OCHANNEL_%s);\n",
                si->name.escaped().c_str(), si->blurb.escaped().c_str(), cUC_NAME (si->ident));
      for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
        {
          const gchar *sig_name = canonify_name (si->name, '_');
          const gchar *sig_string = canonify_name (si->name);
          printf ("    static_data.signal_%s =\n      klass->add_signal (\"%s\", (GSignalFlags) 0, %u",
                  sig_name, sig_string, si->params.size());
          for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++)
            printf (",\n                         %s", TYPE_NAME (ai->type));
          printf (");\n");
        }
      printf ("  }\n");

      /* signal emission methods */
      printf ("public:\n");
      for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
        {
          const gchar *sig_name = canonify_name (si->name, '_');
          printf ("  void emit_%s (", sig_name);
          for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++)
            {
              if (ai != si->params.begin())
                printf (", ");
              printf ("%s %s", TypeRef (ai->type), ai->name.c_str());
            }
          printf (")\n");
          printf ("  {\n");
          printf ("    GValue args[1 + %u];\n", si->params.size());
          printf ("    args[0].g_type = 0, g_value_init (args + 0, %s);\n", FULL_TYPE_NAME);
          printf ("    g_value_set_object (args + 0, gobject());\n");
          guint i = 1;
          for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++, i++)
            {
              printf ("    args[%u].g_type = 0, g_value_init (args + %u, %s);\n", i, i, TYPE_NAME (ai->type));
              printf ("    %s (args + %u, %s);\n", func_value_set_param (*ai), i, ai->name.c_str());
            }
          printf ("    g_signal_emitv (args, static_data.signal_%s, 0, NULL);\n", sig_name);
          for (i = 0; i <= si->params.size(); i++)
            printf ("    g_value_unset (args + %u);\n", i);
          printf ("  }\n");
        }

      /* done */
      printf ("};\n"); /* finish: class ... { }; */
    }
  printf ("\n");
  
  /* collect procedures */
  for (vector<Method>::const_iterator mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
    procs.push_back (&(*mi));

  /* generate procedures */
  generate_procedures (nspace, procs, images);

  /* image method implementations */
  for (vector<Image>::const_iterator ii = images.begin(); ii != images.end(); ii++)
    {
      printf ("%s\n", ii->method.c_str());
      printf ("{\n");
      gint estatus = 0;
      GError *error = NULL;
      gchar *out, *err = NULL;
      string cmd = string() + "gdk-pixbuf-csource " + "--name=local_pixstream " + ii->file;
      g_spawn_command_line_sync (cmd.c_str(), &out, &err, &estatus, &error);
      if (err && *err)
        g_printerr ("gdk-pixbuf-csource: %s", err);
      if (error || estatus)
        {
          if (error)
            g_printerr ("failed to convert image file \"%s\" with gdk-pixbuf-csource%c %s",
                        ii->file.c_str(), error ? ':' : ' ', error->message);
          exit (estatus & 255 ? estatus : 1);
        }
      g_clear_error (&error);
      g_free (err);
      printf ("  %s\n", out);
      g_free (out);
      printf ("  return local_pixstream;\n");
      printf ("}\n");
    }
  
  /* close namespace */
  printf ("\n}; /* %s */\n", nspace.c_str());
  printf("\n/*-------- end %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
}

/* vim:set ts=8 sts=2 sw=2: */
