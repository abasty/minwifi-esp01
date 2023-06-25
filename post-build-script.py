Import("env", "projenv")

import subprocess
from platformio.builder.tools.pioupload import CheckUploadSize

def post_program_action(source, target, env):
    print("Generate status.txt file")

    from contextlib import redirect_stdout

    result = subprocess.run(['git', 'log', '--oneline', '-n', '1'], capture_output=True)
    with open('status.txt', 'a') as f:
        with redirect_stdout(f):
            print(result.stdout.decode().strip())
            CheckUploadSize("", target, source, env)
            print('---')

env.AddPostAction("checkprogsize", post_program_action)
