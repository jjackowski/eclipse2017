Import('*')

lib = env.StaticLibrary('eclipse2017', Glob('[FU]*.cpp'))

env_duds = env.Clone()
env_duds.AppendUnique(
	LIBS = 'duds'
)

env_gps = env.Clone()
env_gps.AppendUnique(
	LIBS = 'gps'
)

env_duds_gps = env_duds.Clone()
env_duds_gps.AppendUnique(
	LIBS = 'gps'
)

targets = [
	#env_duds.Program('umbra_test', ['umbra_test.cpp'] + lib),
]

if 'LIBDUDS' in env:
	targets.append(
		env_duds.Program('umbra_test', ['umbra_test.cpp'] + lib)
	)
else:
	targets.append(
		env.Program('umbra_test', ['umbra_test.cpp'] + lib)
	)
if 'LIBDUDS' in env and 'LIBGPS' in env:
	targets.append(
		env_duds_gps.Program('umbra_lcd', ['umbra_lcd.cpp'] + lib + Glob('[DR]*.cpp'))
	)

Return('targets')
