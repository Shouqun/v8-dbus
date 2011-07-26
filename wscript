#!/usr/bin/env python
import re
import Options
import sys, os, shutil, glob
import Utils
from Utils import cmd_output
from os.path import join, dirname, abspath, normpath
from logging import fatal

cwd = os.getcwd();
sys.path.append(sys.argv[0] + '/tools')

supported_archs = ('arm', 'ia32', 'x64') 

def safe_path(path):
  return path.replace("\\", "/")

def canonical_cpu_type(arch):
  m = {'x86': 'ia32', 'i386':'ia32', 'x86_64':'x64', 'amd64':'x64'}
  if arch in m: arch = m[arch]
  if not arch in supported_archs:
    raise Exception("supported architectures are "+', '.join(supported_archs)+\
                    " but NOT '" + arch + "'.")
  return arch

def set_options(opt):
  opt.tool_options('compiler_cxx')
  opt.tool_options('compiler_cc')

  opt.add_option( '--dest-cpu'
                , action='store'
                , default=None
                , help='CPU architecture to build for. Valid values are: '+\
                       ', '.join(supported_archs)
                , dest='dest_cpu'
                )

#test code program 
def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('compiler_cc') 
  
  conf.env.append_value("LIB_EXPAT", "expat");
  conf.check_cfg(package='dbus-1', args='--cflags --libs', uselib_store='DBUS');
  conf.check_cfg(package='dbus-glib-1', args='--cflags --libs', uselib_store='GDBUS');
  
  # normalize DEST_CPU from --dest-cpu, DEST_CPU or built-in value
  if Options.options.dest_cpu and Options.options.dest_cpu:
    conf.env['DEST_CPU'] = canonical_cpu_type(Options.options.dest_cpu)
  elif 'DEST_CPU' in os.environ and os.environ['DEST_CPU']:
    conf.env['DEST_CPU'] = canonical_cpu_type(os.environ['DEST_CPU'])
  elif 'DEST_CPU' in conf.env and conf.env['DEST_CPU']:
    conf.env['DEST_CPU'] = canonical_cpu_type(conf.env['DEST_CPU'])

  if sys.platform.startswith("darwin"):
    # used by platform_darwin_*.cc
    conf.env.append_value('LINKFLAGS', ['-framework','Carbon'])
    # cross compile for architecture specified by DEST_CPU
    if 'DEST_CPU' in conf.env:
      arch = conf.env['DEST_CPU']
      # map supported_archs to GCC names:
      arch_mappings = {'ia32': 'i386', 'x64': 'x86_64'}
      if arch in arch_mappings:
        arch = arch_mappings[arch]
      flags = ['-arch', arch]
      conf.env.append_value('CCFLAGS', flags)
      conf.env.append_value('CXXFLAGS', flags)
      conf.env.append_value('LINKFLAGS', flags)
  if 'DEST_CPU' in conf.env:
    arch = conf.env['DEST_CPU']
    # TODO: -m32 is only available on 64 bit machines, so check host type
    flags = None
    if arch == 'ia32':
      flags = '-m32'
    if flags:
      conf.env.append_value('CCFLAGS', flags)
      conf.env.append_value('CXXFLAGS', flags)
      conf.env.append_value('LINKFLAGS', flags)


#test code  program
def build(bld):
  import Build;
  #build v8
  build_v8(bld);
  #build the dbus extension
  build_dbus(bld);

def build_dbus(bld):
  obj = bld.new_task_gen('cxx', 'program'); 
  obj.target = 'dbus';
  obj.source = '''
    src/dbus_library.cc
    src/dbus_introspect.cc
    src/dbus_main.cc
    '''
  obj.uselib = 'DBUS GDBUS EXPAT V8'
  obj.includes = 'deps/v8/include'

def v8_cmd(bld, variant):
  scons = join(cwd, 'tools/scons/scons.py')
  deps_src = join(bld.path.abspath(),"deps/")
  v8dir_src = join(deps_src,"v8")

  # NOTE: We want to compile V8 to export its symbols. I.E. Do not want
  # -fvisibility=hidden. When using dlopen() it seems that the loaded DSO
  # cannot see symbols in the executable which are hidden, even if the
  # executable is statically linked together...

  # XXX Change this when v8 defaults x86_64 to native builds
  # Possible values are (arm, ia32, x64, mips).
  arch = "x64"
  if bld.env['DEST_CPU']:
    arch = "arch="+bld.env['DEST_CPU']

  toolchain = "gcc"

  if variant == "default":
    mode = "release"
  else:
    mode = "debug"

  if bld.env["SNAPSHOT_V8"]:
    snapshot = "snapshot=on"
  else:
    snapshot = ""

  cmd_R = sys.executable + ' "%s" -j %d -C "%s" -Y "%s" visibility=default mode=%s %s toolchain=%s library=static %s'

  cmd = cmd_R % ( scons
                , Options.options.jobs
                , safe_path(bld.srcnode.abspath(bld.env_of_name(variant)))
                , safe_path(v8dir_src)
                , mode
                , arch
                , toolchain
                , snapshot
                )

  if bld.env["USE_GDBJIT"]:
    cmd += ' gdbjit=on '

  if sys.platform.startswith("sunos"):
    cmd += ' toolchain=gcc strictaliasing=off'

  return ("echo '%s' && " % cmd) + cmd

#test code program
def build_v8(bld):
  print "To build " + ( bld.env["staticlib_PATTERN"] % ('v8') )
  v8 = bld.new_task_gen(
    source        = 'deps/v8/SConstruct ', 
    target        = bld.env["staticlib_PATTERN"] % ('v8'),
    rule          = v8_cmd(bld, "default"), 
    before        = "cxx",
    install_path  = None)
  v8.uselib = "EXECINFO"
  bld.env["CPPPATH_V8"] = "deps/v8/include"
  t = join(bld.srcnode.abspath(bld.env_of_name("default")), v8.target)
  bld.env_of_name('default').append_value("LINKFLAGS_V8", t)

