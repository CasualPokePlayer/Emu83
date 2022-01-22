global_cflags = ARGUMENTS.get('CFLAGS', '-Wall -Wextra -Wpedantic -O3 -flto -fvisibility=internal -fomit-frame-pointer')
global_defines = ' -DNDEBUG'
vars = Variables()
vars.Add('CC')

env = Environment(CFLAGS = global_cflags + global_defines, variables = vars)

sourceFiles = Split('''events.c memory.c ti83.c z80.c link.c savestate.c stream.c queue.c crc32.c''')

conf = env.Configure()

conf.Finish()

shlib = env.SharedLibrary('emu83', sourceFiles, LINKFLAGS = env['LINKFLAGS'] + ' -s', SHLIBPREFIX = "lib")

env.Default(shlib)
