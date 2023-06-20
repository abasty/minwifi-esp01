Import("env", "projenv")

def post_program_action(source, target, env):
    print("Program has been built!")
    program_path = target[0].get_abspath()
    print("Program path", program_path)
    print(projenv.Dump())

env.AddPostAction("checkprogsize", post_program_action)
