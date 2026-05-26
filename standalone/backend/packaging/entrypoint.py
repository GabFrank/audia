import sys, os
def _log(m): sys.stderr.write(m + "\n"); sys.stderr.flush()
_log("[boot] pid=%d start" % os.getpid())
from auma_backend.__main__ import main
_log("[boot] calling main")
main()
