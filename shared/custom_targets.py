Import("env")

if env['PIOENV'] == "release":
    env.AddCustomTarget(
            "generate_control",
            "$BUILD_DIR/${PROGNAME}.bin",
            "misc/generate_control.py"
    )
else:
    env.AddCustomTarget(
            "generate_control",
            "$BUILD_DIR/${PROGNAME}.bin",
            "true"
    )

