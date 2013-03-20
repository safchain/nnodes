"""
Next.
"""

def setupProject(project):
    # Command line
    TIMEOUT = 0.7

    # Create buggy input file
    orig_filename = project.application().getInputFilename("Xml Conf File")
    mangle = AutoMangle(project, orig_filename)
    mangle.max_size = 10*1024*1024
    mangle.config.max_op = 100
    mangle.config.change_size = True
    mangle.config.bit = True

    process = AppProcess(project,
        ['/home/safchain/dev/next/src/service/next', '--df', '--nb', '0', '--cf'] + ["<conf_filename>"],
        timeout=TIMEOUT)
    watch = WatchProcess(process, timeout_score=0, exitcode_score=0)
    watch.cpu.max_load = 0.50
    watch.cpu.max_duration = 1.0
    watch.cpu.max_score = 0.50

    stdout = WatchStdout(process)

    # Ignore input errors
    #stdout.ignoreRegex('^Failed to open')

    # Restore terminal state
    TerminalEcho(project)

from fusil.process.create import CreateProcess
from fusil.process.watch import WatchProcess
from fusil.process.stdout import WatchStdout
from fusil.auto_mangle import AutoMangle
from fusil.terminal_echo import TerminalEcho

class AppProcess(CreateProcess):
    def on_mangle_filenames(self, conf_filenames):
        self.cmdline.arguments[-1] = conf_filenames[0]
        self.createProcess()

