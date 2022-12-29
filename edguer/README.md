# My KB

## Build

Run `build_debug.sh` from source root folder. Here is what i does:
- Set variable JOBS to 8 - change to the number of cores in your machine
- Set BUILDVARS, adding the debug flag
- Set USR_SIZE and ROOT_SIZE so the image accommodates the new sizes after the debug compilation - the build script considers the release size for those sections.

## Run

Just run the `run.sh` command, it will start qemu using the Minix image.