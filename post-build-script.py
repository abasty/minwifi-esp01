Import("env", "projenv")

# import os
# import re

import subprocess
from platformio.builder.tools.pioupload import CheckUploadSize


# def CheckUploadSize(_, target, source, env):
#     check_conditions = [
#         env.get("BOARD"),
#         env.get("SIZETOOL") or env.get("SIZECHECKCMD"),
#     ]
#     if not all(check_conditions):
#         return
#     program_max_size = int(env.BoardConfig().get("upload.maximum_size", 0))
#     data_max_size = int(env.BoardConfig().get("upload.maximum_ram_size", 0))
#     if program_max_size == 0:
#         return

#     def _configure_defaults():
#         env.Replace(
#             SIZECHECKCMD="$SIZETOOL -B -d $SOURCES",
#             SIZEPROGREGEXP=r"^(\d+)\s+(\d+)\s+\d+\s",
#             SIZEDATAREGEXP=r"^\d+\s+(\d+)\s+(\d+)\s+\d+",
#         )

#     def _get_size_output():
#         cmd = env.get("SIZECHECKCMD")
#         if not cmd:
#             return None
#         if not isinstance(cmd, list):
#             cmd = cmd.split()
#         cmd = [arg.replace("$SOURCES", str(source[0])) for arg in cmd if arg]
#         sysenv = os.environ.copy()
#         sysenv["PATH"] = str(env["ENV"]["PATH"])
#         result = exec_command(env.subst(cmd), env=sysenv)
#         if result["returncode"] != 0:
#             return None
#         return result["out"].strip()

#     def _calculate_size(output, pattern):
#         if not output or not pattern:
#             return -1
#         size = 0
#         regexp = re.compile(pattern)
#         for line in output.split("\n"):
#             line = line.strip()
#             if not line:
#                 continue
#             match = regexp.search(line)
#             if not match:
#                 continue
#             size += sum(int(value) for value in match.groups())
#         return size

#     def _format_availale_bytes(value, total):
#         percent_raw = float(value) / float(total)
#         blocks_per_progress = 10
#         used_blocks = min(
#             int(round(blocks_per_progress * percent_raw)), blocks_per_progress
#         )
#         return "[{:{}}] {: 6.1%} (used {:d} bytes from {:d} bytes)".format(
#             "=" * used_blocks, blocks_per_progress, percent_raw, value, total
#         )

#     if not env.get("SIZECHECKCMD") and not env.get("SIZEPROGREGEXP"):
#         _configure_defaults()
#     output = _get_size_output()
#     program_size = _calculate_size(output, env.get("SIZEPROGREGEXP"))
#     data_size = _calculate_size(output, env.get("SIZEDATAREGEXP"))

#     print('Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"')
#     if data_max_size and data_size > -1:
#         print("RAM:   %s" % _format_availale_bytes(data_size, data_max_size))
#     if program_size > -1:
#         print("Flash: %s" % _format_availale_bytes(program_size, program_max_size))
#     if int(ARGUMENTS.get("PIOVERBOSE", 0)):
#         print(output)

#     # raise error
#     # if data_max_size and data_size > data_max_size:
#     #     sys.stderr.write(
#     #         "Error: The data size (%d bytes) is greater "
#     #         "than maximum allowed (%s bytes)\n" % (data_size, data_max_size))
#     #     env.Exit(1)
#     if program_size > program_max_size:
#         sys.stderr.write(
#             "Error: The program size (%d bytes) is greater "
#             "than maximum allowed (%s bytes)\n" % (program_size, program_max_size)
#         )
#         env.Exit(1)

def post_program_action(source, target, env):
    print("Generate status.txt file")

    from contextlib import redirect_stdout

    result = subprocess.run(['git', 'log', '--oneline', '-n', '1'], capture_output=True)
    with open('status.txt', 'a') as f:
        with redirect_stdout(f):
            print(result.stdout.decode().strip())
            CheckUploadSize("", target, source, env)
            print('')

env.AddPostAction("checkprogsize", post_program_action)
