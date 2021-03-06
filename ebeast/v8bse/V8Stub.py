# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
"""AidaPyxxStub - Aida Cython Code Generator

More details at http://www.rapicorn.org/
"""
import Decls, re, sys, os

# == Utilities ==
def class_digest (class_info):
  return digest2cbytes (class_info.type_hash())
def digest2cbytes (digest):
  return '0x%02x%02x%02x%02x%02x%02x%02x%02xULL, 0x%02x%02x%02x%02x%02x%02x%02x%02xULL' % digest
def hasancestor (child, parent):
  for p in child.prerequisites:
    if p == parent or hasancestor (p, parent):
      return True
def inherit_reduce (type_list):
  # find the type(s) we *directly* derive from
  reduced = []
  while type_list:
    p = type_list.pop()
    skip = 0
    for c in type_list + reduced:
      if c == p or hasancestor (c, p):
        skip = 1
        break
    if not skip:
      reduced = [ p ] + reduced
  return reduced
def bases (tp):
  ancestors = [pr for pr in tp.prerequisites]
  return inherit_reduce (ancestors)
def type_namespace_names (tp):
  namespaces = tp.list_namespaces() # [ Namespace... ]
  return [d.name for d in namespaces if d.name]
def underscore_namespace (tp):
  return '__'.join (type_namespace_names (tp))
def colon_namespace (tp):
  return '::'.join (type_namespace_names (tp))
def identifier_name (joiner, type_name, member = None):
  parts = type_namespace_names (type_name) + [ type_name.name ]
  if member: parts += [ member ]
  return joiner.join (parts)
def underscore_typename (tp):
  if tp.storage == Decls.ANY:
    return 'Rapicorn__Any'
  return identifier_name ('__', tp)
def colon_typename (tp):
  name = identifier_name ('::', tp)
  if tp.storage == Decls.INTERFACE:
    name += 'H' # e.g. WidgetHandle
  return name
def v8ppclass_type (tp):
  return 'V8ppType_' + identifier_name ('', tp)
def v8ppclass (tp):
  return identifier_name ('', tp) + '_class_'

class Generator:
  def __init__ (self, idl_file, module_name):
    assert isinstance (module_name, str)
    self.ntab = 26
    self.idl_file = idl_file
    self.module_name = module_name
    self.strip_path = ""
    self.idcounter = 1001
    self.marshallers = {}
  @staticmethod
  def prefix_namespaced_identifier (prefix, ident, postfix = ''):     # Foo::bar -> Foo::PREFIX_bar_POSTFIX
    cc = ident.rfind ('::')
    if cc >= 0:
      ns, base = ident[:cc+2], ident[cc+2:]
    else:
      ns, base = '', ident
    return ns + prefix + base + postfix
  def type_absolute_namespaces (self, type_node):
    tnsl = type_node.list_namespaces() # type namespace list
    namespace_names = [d.name for d in tnsl if d.name]
    return namespace_names
  def type2cpp (self, type_node):
    tstorage = type_node.storage
    if tstorage == Decls.VOID:          return 'void'
    if tstorage == Decls.BOOL:          return 'bool'
    if tstorage == Decls.INT32:         return 'int'
    if tstorage == Decls.INT64:         return 'int64_t'
    if tstorage == Decls.FLOAT64:       return 'double'
    if tstorage == Decls.STRING:        return 'std::string'
    if tstorage == Decls.ANY:           return 'Rapicorn::Aida::Any'
    fullnsname = '::'.join (self.type_absolute_namespaces (type_node) + [ type_node.name ])
    return fullnsname
  def C4client (self, type_node):
    tname = self.type2cpp (type_node)
    if type_node.storage == Decls.INTERFACE:
      return tname + 'Handle'                           # construct client class RemoteHandle
    elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
      return self.prefix_namespaced_identifier ('ClnT_', tname)
    return tname
  def C (self, type_node, mode = None):                 # construct Class name
    return self.C4client (type_node)
  def R (self, type_node):                              # construct Return type
    tname = self.C (type_node)
    return tname
  def A (self, ident, type_node, defaultinit = None):   # construct call Argument
    constref = type_node.storage in (Decls.STRING, Decls.SEQUENCE, Decls.RECORD, Decls.ANY)
    needsref = constref or type_node.storage == Decls.INTERFACE
    s = self.C (type_node)                      # const {Obj} &foo = 3
    s += ' ' if ident else ''                   # const Obj{ }&foo = 3
    if constref:
      s = 'const ' + s                          # {const }Obj &foo = 3
    if needsref:
      s += '&'                                  # const Obj {&}foo = 3
    s += ident                                  # const Obj &{foo} = 3
    if defaultinit != None:
      if type_node.storage == Decls.ENUM:
        s += ' = %s (%s)' % (self.C (type_node), defaultinit)  # { = 3}
      elif type_node.storage in (Decls.SEQUENCE, Decls.RECORD):
        s += ' = %s()' % self.C (type_node)                    # { = 3}
      elif type_node.storage == Decls.INTERFACE:
        s += ' = *(%s*) NULL' % self.C (type_node)             # { = 3}
      else:
        s += ' = %s' % defaultinit                             # { = 3}
    return s
  def generate_types_v8 (self, implementation_types):
    s  = '// === Generated by V8Stub.py ===            -*-mode:javascript;-*-\n'
    # includes
    s += '#include "v8pp/context.hpp"\n'
    s += '#include "v8pp/module.hpp"\n'
    s += '#include "v8pp/class.hpp"\n'
    s += '#include "v8pp/call_v8.hpp"\n'
    s += '\n'
    s += '#define __v8return_exception(iso,args,what)  ({ args.GetReturnValue().Set (v8pp::throw_ex (iso, what)); return; })\n'
    s += '\n'
    s += 'typedef v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> V8ppCopyablePersistentFunction;\n'
    s += 'typedef v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> V8ppCopyablePersistentObject;\n'
    # collect impl types
    namespaces = []
    base_objects = []
    types = []
    for tp in implementation_types:
      if tp.isimpl:
        types += [ tp ]
        namespaces.append (tp.list_namespaces())
    if not types:
      return s
    # check unique namespace
    while len (namespaces) >= 2 and namespaces[0] == namespaces[1]:
      namespaces = namespaces[1:]
    namespaces = [[n for n in names if n.name] for names in namespaces] # strip (initial) empty names
    max_namespaces = max (len (namespaces), len (namespaces[0]))
    if max_namespaces != 1:
      raise NotImplementedError ('V8 code generation requires exactly 1 namespace (%d given)' % max_namespaces)
    self.namespace = namespaces[0][0].name
    del namespaces, max_namespaces
    # Collect v8pp::class_ types
    v8pp_class_types = []
    for tp in types:
      if tp.is_forward:
        continue
      if tp.storage in (Decls.SEQUENCE, Decls.RECORD, Decls.INTERFACE):
        v8pp_class_types += [ tp ]
    # C++ class type aliases for v8pp::class_
    s += '\n// v8pp::class_ aliases\n'
    s += 'typedef %-40s V8ppType_AidaRemoteHandle;\n' % 'v8pp::class_<Aida::RemoteHandle>'
    for tp in v8pp_class_types:
      s += 'typedef %-40s %s;\n' % ('v8pp::class_<%s>' % colon_typename (tp), v8ppclass_type (tp))
    # C++ class specialisations for v8pp::convert
    s += '\n// v8pp::convert<> specialisations\n'
    s += 'namespace v8pp {\n'
    s += 'template<> struct convert%-40s : convert_AidaRemoteHandle<Aida::RemoteHandle> {};\n' % '<Aida::RemoteHandle>'
    s += 'template<> struct convert%-40s : convert_AidaRemoteHandle<Aida::RemoteHandle*> {};\n' % '<Aida::RemoteHandle*>'
    for tp in v8pp_class_types:
      cn = colon_typename (tp)
      if tp.storage == Decls.INTERFACE:
        s += 'template<> struct convert%-40s : convert_AidaRemoteHandle<%s>  {};\n' % ('<%s>' % cn, cn)
        s += 'template<> struct convert%-40s : convert_AidaRemoteHandle<%s*> {};\n' % ('<%s*>' % cn, cn)
        s += 'template<> struct convert%-40s : convert_AidaRemoteHandle<%s>  {};\n' % ('<Aida::RemoteMember<%s>>' % cn, cn)
        s += 'template<> struct convert%-40s : convert_AidaRemoteHandle<%s*> {};\n' % ('<Aida::RemoteMember<%s>*>' % cn, cn)
      elif tp.storage == Decls.SEQUENCE:
        s += 'template<> struct convert%-40s : convert_AidaSequence<%s> {};\n' % ('<%s>' % cn, cn)
    s += '} // v8pp\n'
    # V8stub - main binding stub
    s += '\n// Main binding stub\n'
    s += 'struct V8stub final {\n'
    s += '  v8::Isolate                             *const isolate_;\n'
    s += '  %-40s %s;\n' % ('V8ppType_AidaRemoteHandle', 'AidaRemoteHandle_class_')
    for tp in v8pp_class_types:
      s += '  %-40s %s;\n' % (v8ppclass_type (tp), v8ppclass (tp))
    s += '  v8pp::module                             module_;\n'
    s += 'public:\n'
    s += '  explicit    V8stub (v8::Isolate *const __v8isolate);\n'
    s += '  void        jsinit (v8::Local<v8::Context> context, v8::Local<v8::Object> exports) const;\n'
    s += '};\n'
    # V8stub ctor - begin
    s += '\nV8stub::V8stub (v8::Isolate *const __v8isolate) :\n'
    s += '  isolate_ (__v8isolate),\n'
    s += '  AidaRemoteHandle_class_ (__v8isolate),\n'
    for tp in v8pp_class_types:
      s += '  %s (__v8isolate),\n' % v8ppclass (tp)
    s += '  module_ (__v8isolate)\n'
    s += '{\n'
    s += '  v8::HandleScope __v8scope (__v8isolate);\n'
    # Wrapper registration
    for tp in v8pp_class_types:
      cn = colon_typename (tp)
      if tp.storage == Decls.INTERFACE:
        s += '  aida_remote_handle_wrapper_map (Aida::TypeHash '
        s += '(%s), ' % class_digest (tp)
        s += 'aida_remote_handle_wrapper_impl<%s>);\n' % cn
    # Class bindings
    for tp in v8pp_class_types:
      cn = colon_typename (tp)
      b, g = '', ''
      # Class inheritance
      if tp.storage == Decls.INTERFACE:
        for tb in bases (tp):
          b += '    .inherit<%s>()\n' % colon_typename (tb)
        if len (bases (tp)) == 0:
          b += '    .inherit<%s>()\n' % 'Aida::RemoteHandle'
          base_objects += [ tp.name ]
      # Class ctor
      if tp.storage == Decls.SEQUENCE or tp.storage == Decls.RECORD:
        b += '    .ctor()\n'
      # Class properties
      if tp.storage != Decls.SEQUENCE:
        for fname, ftp in tp.fields:
          if tp.storage == Decls.INTERFACE:
            b += '    .set ("%s", ' % fname
            b += 'v8pp::property (&%s::%s, &%s::%s))\n' % (cn, fname, cn, fname)
          elif tp.storage == Decls.RECORD:
            b += '    .set ("%s", &%s::%s)\n' % (fname, cn, fname)
      # Class methods
      if tp.storage == Decls.INTERFACE:
        for mtp in tp.methods:
          rtp, mname = mtp.rtype, mtp.name
          b += '    .set ("%s", &%s::%s)\n' % (mname, cn, mname)
      # Class signal handlers
      if tp.storage == Decls.INTERFACE:
        for sg in tp.signals:
          g += '  auto onsig_%d = [__v8isolate] (v8::FunctionCallbackInfo<v8::Value> const &__v8args) -> void {\n' % self.idcounter
          g += '    %s *__bsehandle = %s::unwrap_object (__v8isolate, __v8args.This());\n' % (colon_typename (tp), v8ppclass_type (tp))
          g += '    if (!__bsehandle || __v8args.Length() != 1 || !__v8args[0]->IsFunction())\n'
          g += '      __v8return_exception (__v8isolate, __v8args, "V8stub: on(): invalid invocation");\n'
          g += '    V8ppCopyablePersistentObject __v8pthis (__v8isolate, __v8args[0]->ToObject());\n'
          g += '    V8ppCopyablePersistentFunction __v8pfunc (__v8isolate, v8::Local<v8::Function>::Cast (__v8args[0]));\n'
          l, k = [], []
          for sa in sg.args:
            l += [ self.A (sa[0], sa[1]) ]
            k += [ sa[0] ]
          rtype = self.R (sg.rtype)
          g += '    auto sighandler = [__v8isolate, __bsehandle, __v8pthis, __v8pfunc] '
          g += '(%s) -> %s {\n' % (', '.join (l), rtype)
          g += '      v8::HandleScope __v8scope (__v8isolate);\n'
          g += '      v8::Local<v8::Object> __v8this = v8pp::to_local (__v8isolate, __v8pthis);\n'
          g += '      v8::Local<v8::Function> __v8func = v8pp::to_local (__v8isolate, __v8pfunc);\n'
          g += '      v8::Local<v8::Value> __v8result = v8pp::call_v8 (__v8isolate, __v8func, %s);\n' % ', '.join ([ '__v8this' ] + k)
          if sg.rtype.storage != Decls.VOID:
            g += '      return v8pp::from_v8<%s> (__v8isolate, __v8result);\n' % rtype
          else:
            g += '      (void) __v8result;\n' # avoid 'unused' warnings
          g += '    };\n'
          g += '    __bsehandle->sig_%s() += sighandler;\n' % sg.name
          g += '    __v8args.GetReturnValue().Set (v8::Null (__v8isolate));\n' # TODO: return handle for disconnect?
          g += '  };\n'
          b += '    .set ("__on_%s", onsig_%d)\n' % (sg.name, self.idcounter)
          # TODO: provide a method to disconnect signal handlers, i.e. off()
          self.idcounter += 1
      # output generated signal handlers
      if g:
        s += g
      # output only non-empty bindings
      if b:
        s += '  %s\n' % v8ppclass (tp)
        s += b
        s += '  ;\n'
    # Class registration
    s += '\n'
    for tp in v8pp_class_types:
      s += '  module_.set ("%s", %s);\n' % (tp.name, v8ppclass (tp))
    # V8stub ctor - end
    s += '}\n'
    # jsinit - begin
    s += '\nvoid\n'
    s += 'V8stub::jsinit (v8::Local<v8::Context> context, v8::Local<v8::Object> exports) const\n'
    s += '{\n'
    jsinit_def = '{ base_objects: [ %s ] }' % ', '.join ('exports.' + e for e in base_objects)
    j = re.sub (r'@jsinit_def@', jsinit_def, jsinit)
    j = re.sub (r'(["\\])', r'\\\1', j)
    j = re.sub (r'\n', r'\\n"\n  "', j)
    s += '  v8::Isolate *const isolate = context->GetIsolate();\n'
    s += '  v8::HandleScope __v8scope (isolate);\n'
    s += '  v8::ScriptOrigin org = v8::ScriptOrigin (v8pp::to_v8 (isolate, __FILE__),\n'
    s += '                                           v8::Integer::New (isolate, __LINE__));\n'
    s += '  const char *const script = "%s";\n' % j
    s += '  v8::Local<v8::String> code = v8pp::to_v8 (isolate, script);\n'
    s += '  v8::Local<v8::Value> bcode = v8::Script::Compile (context, code, &org).ToLocalChecked()->Run();\n'
    s += '  v8::Local<v8::Function> fun = v8::Local<v8::Function>::Cast (bcode);\n'
    s += '  assert (!fun.IsEmpty());\n'
    s += '  v8::Local<v8::Value> args[] = { exports };\n'
    s += '  fun->Call (context, context->Global(), sizeof (args) / sizeof (args[0]), args);\n'
    # jsinit - end
    s += '}\n'
    return s

# Javacript code to be executed at v8stub setup
jsinit = r"""
(function (exports) {
  const jsinit = @jsinit_def@;
  jsinit.base_objects.forEach (obj => {
    obj.prototype.on = function (signal, handler) {
      const connector = this['__on_' + signal.replace (/[^a-z0-9]/gi, '_')];
      if (connector instanceof Function)
        return connector.call (this, handler);
      throw new ReferenceError (signal + " is not a signal");
    };
  });
})
"""

def generate (namespace_list, **args):
  import sys, tempfile, os
  config = {}
  config.update (args)
  if '--print-include-path' in config.get ('backend-options', []):
    includestem = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
    includepath = os.path.join (includestem, 'v8')
    print includepath
  else:
    outname = config.get ('output', 'testmodule')
    if outname == '-':
      raise RuntimeError ("-: stdout is not support for generation of multiple files")
    idlfiles = config['files']
    if len (idlfiles) != 1:
      raise RuntimeError ("V8Stub: exactly one IDL input file is required")
    gg = Generator (idlfiles[0], outname)
    for opt in config['backend-options']:
      if opt.startswith ('strip-path='):
        gg.strip_path += opt[11:]
    fname = outname
    fout = open (fname, 'w')
    textstring = gg.generate_types_v8 (config['implementation_types'])
    fout.write (textstring)
    fout.close()

# register extension hooks
__Aida__.add_backend (__file__, generate, __doc__)
