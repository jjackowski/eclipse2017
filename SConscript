Import('*')

if 'LIBEVDEV' in env:
	lib = env.StaticLibrary('eclipse2017', Glob('[FUPI]*.cpp'))
else:
	lib = env.StaticLibrary('eclipse2017', Glob('[FUP]*.cpp'))

env_duds = env.Clone()
env_duds.AppendUnique(
	LIBS = 'duds'
)

env_lcd = env_duds.Clone()
env_lcd.AppendUnique(
	LIBS = [
		'duds',
		'gps',
		'evdev'
	]
)

env_evdev = env.Clone()
env_evdev.AppendUnique(
	LIBS = 'evdev'
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
if 'LIBDUDS' in env and 'LIBGPS' in env and 'LIBEVDEV' in env:
	targets.append(
		env_lcd.Program('umbra_lcd', ['umbra_lcd.cpp'] + Glob('[DR]*.cpp') + lib)
	)

Return('targets')
