import subprocess
import sys
MODIFIED_FILES = [
    './src/libcpu/context_gcc.S',
    './src/bsp/rtconfig.h',
    './src/libcpu/cpuport.h',  
    './src/bsp/board.c',  
    './src/libcpu/cpuport.c',  
    './src/bsp/startup.S', 
    './src/libcpu/psp_vect_instrumented.S',
    './src/bsp/link.lds'  
]
command = ['git', 'diff', 'PortBase', '--diff-algorithm=minimal', '--']
for file_name in MODIFIED_FILES:
    command.append(file_name)
res = subprocess.run(command, capture_output=True)
res.check_returncode()
with open('branch_diff.patch', 'w') as f:
    f.write(res.stdout.decode(sys.getdefaultencoding()))
