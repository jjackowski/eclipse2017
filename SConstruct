import os
import platform

#####
# setup the build options
buildopts = Variables('localbuildconfig.py')
buildopts.Add(BoolVariable('debug', 'Produce a debugging build', True))
buildopts.Add('CCDBGFLAGS',
	'The flags to use with the compiler for debugging builds.',
	'-g -fno-common -Og')
buildopts.Add('CCOPTFLAGS',
	'The flags to use with the compiler for optimized non-debugging builds.',
	'-O2 -ffunction-sections -fno-common -ffast-math')
buildopts.Add('LINKDBGFLAGS',
	'The flags to use with the linker for debugging builds.',
	'')
buildopts.Add('LINKOPTFLAGS',
	'The flags to use with the linker for optimized builds.',
	'-Wl,--gc-sections')
buildopts.Add(PathVariable('BOOSTINC',
	'The directory containing the Boost header files, or "." for the system default.',
	'.')) #, PathVariable.PathAccept))
buildopts.Add(PathVariable('BOOSTLIB',
	'The directory containing the Boost libraries, or "." for the system default.',
	'.')) #, PathVariable.PathAccept))
buildopts.Add('BOOSTTOOLSET',
	'The toolset tag for Boost libraries; may be needed on Windows. Include a leading dash.',
	'')
#buildopts.Add('BOOSTABI',
#	'The ABI tag for Boost libraries. Include a leading dash.',
#	'')
buildopts.Add('BOOSTTAG',
	'Additional tags for Boost libraries. The libraries must support threading. Include leading dashes.',
	'-mt')
buildopts.Add('BOOSTVER',
	'The version tag for Boost libraries; may be needed on Windows. Include a leading dash.',
	'')
# two dots for a relative path result in THREE dots given to gcc  !#@!#@!$!!!
buildopts.Add(PathVariable('DUDSINC',
	'The root of the directories containing the DUDS library header files; use ONE dot for relative path.',
	'./duds/', PathVariable.PathAccept))
buildopts.Add(PathVariable('DUDSLIB',
	'The directory containing the DUDS libraries.',
	Dir('../duds/bin/${PSYS}-${PARCH}-${BUILDTYPE}/lib/').abspath, PathVariable.PathAccept))

puname = platform.uname()

#####
# create the template build environment
env = Environment(variables = buildopts,
	PSYS = puname[0].lower(),
	PARCH = puname[4].lower(),
	BOOSTABI = '',
	#CC = 'distcc armv6j-hardfloat-linux-gnueabi-gcc',
	#CXX = 'distcc armv6j-hardfloat-linux-gnueabi-g++',
	# include paths
	CPPPATH = [
		#'.',  # this path looks surprised or confused
		#'..',
		#'$BOOSTINC',
		'#/.'
		'$DUDSINC'
	],
	# options passed to C and C++ (?) compiler
	CCFLAGS = [
		# flags always used with compiler
	],
	# options passed to C++ compiler
	CXXFLAGS = [
		'-std=gnu++11'  # allow gcc extentions to C++11, like __int128
		#'-std=c++11'    # no GNU extensions, no 128-bit integer
	],
	# macros
	CPPDEFINES = [
	],
	# options passed to the linker; using gcc to link
	LINKFLAGS = [
		# flags always used with linker
	],
	LIBPATH = [
		'$BOOSTLIB',
		'$DUDSLIB',
	],
	LIBS = [  # required libraries
		# a boost library must be fisrt
		#'libboost_date_time${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'libboost_thread${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'libboost_serialization${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'libboost_wserialization${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		'libboost_system${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
		#'m',
		'pthread',
		'gdal'
	]
)


#####
# Debugging build enviornment
dbgenv = env.Clone(LIBS = [ ])  # no libraries; needed for library check
dbgenv.AppendUnique(
	CCFLAGS = '$CCDBGFLAGS',
	LINKFLAGS = '$LINKDBGFLAGS',
	BINAPPEND = '-dbg',
	BUILDTYPE = 'dbg'
)

# while SCons has a nice way of dealing with configurations, it doesn't
# automatically add things in a way that works for the different build
# enviornments here, unless the configuration is done once for each, which
# doesn't seem to be necessary.
optionalLibs = {
	# key is the macro, value is the library
	#'LIBBOOST_FILESYSTEM' :
	#	'libboost_filesystem${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
	#'LIBBOOST_TEST' :
	#	'libboost_unit_test_framework${BOOSTTOOLSET}${BOOSTTAG}${BOOSTABI}${BOOSTVER}',
	'LIBGPS' : 'gps',
	'LIBDUDS' : 'duds',
}

#####
# extra cleaning
if GetOption('clean'):
	Execute(Delete(Glob('*~') + [
		'config.log',
	] ))

#####
# configure the build
else:
	#####
	# Configuration for Boost libraries
	conf = Configure(dbgenv, config_h = 'BuildConfig.h',
		conf_dir = env.subst('.conf/${PSYS}-${PARCH}')) #, help=False)
	# check for required libraries
	if not conf.CheckLib('gdal', language = 'C++', autoadd=0):
		print 'The GDAL library could not be found.'
		Exit(1)
	if not conf.CheckLib('geos', language = 'C++', autoadd=0):
		print 'The GEOS library could not be found.'
		Exit(1)
	# check optional libraries
	remlibs = [ ]
	for mac, lib in optionalLibs.iteritems():
		if conf.CheckLib(dbgenv.subst(lib), language = 'C++', autoadd=0):
			conf.Define('HAVE_' + mac, 1, 'optional library')
		else:
			remlibs.append(mac)
	for mac in remlibs:
		del optionalLibs[mac]
	dbgenv = conf.Finish()

# add back the libraries
dbgenv['LIBS'] = env['LIBS']
# remove Boost unit test library; should only be added for test programs
if 'LIBBOOST_TEST' in optionalLibs:
	del optionalLibs['LIBBOOST_TEST']
	havetestlib = True
else:
	havetestlib = False

#####
# Optimized build enviornment
optenv = env.Clone()
optenv.AppendUnique(
	CCFLAGS = '$CCOPTFLAGS',
	LINKFLAGS = '$LINKOPTFLAGS',
	BINAPPEND = '',
	BUILDTYPE = 'opt'
)

envs = [ dbgenv, optenv ]

for env in envs:
	# add in optional libraries
	for mac, lib in optionalLibs.iteritems():
		#env.Append(
		#	#CPPDEFINES = ('HAVE_' + mac, 1),
		#	LIBS = lib
		#)
		env[mac] = True
	# build code
	code = SConscript('SConscript', exports = 'env', duplicate=0,
		variant_dir = env.subst('bin/${PSYS}-${PARCH}-${BUILDTYPE}'))
	env.Clean(code, env.subst('bin/${PSYS}-${PARCH}-${BUILDTYPE}'))
	Alias(env['BUILDTYPE'], code)

Default('dbg')

#####
# setup help text for the build options
Help(buildopts.GenerateHelpText(dbgenv, cmp))
#if GetOption('help'):
#	print 'Build target aliases:'
#	print '  libs-dbg    - All libraries; debugging build. This is the default.'
#	print '  libs        - Same as libs-dbg'
#	print '  samples-dbg - All sample programs; debugging build.'
#	print '  samples     - Same as samples-dbg.'
#	print '  tests-dbg   - All unit test programs; debugging build.'
#	print '  tests       - Same as tests-dbg.'
