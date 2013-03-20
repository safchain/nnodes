#!/usr/bin/env python
"""
fuzzing the lookup section of nnodes
"""

MAX_FILESIZE = 1024*1024

from fusil.application import Application
from optparse import OptionGroup
from fusil.process.mangle import MangleProcess
from fusil.process.watch import WatchProcess
from fusil.process.stdout import WatchStdout
from fusil.auto_mangle import AutoMangle
from fusil.terminal_echo import TerminalEcho

class Fuzzer(Application):
    NAME = "config"
    USAGE = "%prog [options] config"
    NB_ARGUMENTS = 1

    def createFuzzerOptions(self, parser):
        options = OptionGroup(parser, "lookup")
        return options

    def setupProject(self):
	project = self.project

	conf = self.arguments[0]

        # Create buggy input file
        mangle = AutoMangle(project, conf)
        mangle.max_size = MAX_FILESIZE

        process = MangleProcess(project,
            ['./config', '--cf', "<conf>"],
	    "<conf>",
            timeout=2)
        watch = WatchProcess(process, timeout_score=0)
        if watch.cpu:
            watch.cpu.weight = 0.20
            watch.cpu.max_load = 0.50
            watch.cpu.max_duration = min(3, 2 - 0.5)
            watch.cpu.max_score = 0.50

        #stdout = WatchStdout(process)

        # Restore terminal state
        TerminalEcho(project)

if __name__ == "__main__":
    Fuzzer().main()

