# version

version := '2.0.1'

# variables

cc := 'gcc'
cd := 'gdb'
ct := 'valgrind'
c-standard := 'c11'
c-common-flags := '-D_POSIX_C_SOURCE=200809L -std=' + c-standard + ' -pedantic -W -Wall -Wextra -Werror -I./src/headers -Wno-unused-parameter -Wno-unused-variable'
c-release-flags := c-common-flags + ' -O3 ' + c-extra-flags
c-debug-flags := c-common-flags + ' -g -g' + cd + ' ' + c-extra-flags
c-extra-flags := ''

# rules

os-build-dir := './build'
project-name := 'lab02'
just-self := just_executable() + ' --justfile ' + justfile()

_validate mode:
    @ if [ '{{ mode }}' != 'debug' ] && [ '{{ mode }}' != 'release' ]; then echo '`mode` must be: debug or release, not `{{ mode }}`'; exit 1; fi

# build project (`mode` must be: debug or `release`)
build mode: (_validate mode)
    @ mkdir -p '{{ os-build-dir }}'
    @ {{ just-self }} '_build_{{ mode }}'

_build_debug:
    {{ cc }} {{ c-debug-flags }} parent.c --output '{{ os-build-dir }}/parent'

_build_release:
    {{ cc }} {{ c-release-flags }} parent.c --output '{{ os-build-dir }}/parent'

# execute project's binary (`mode` must be: debug or `release`)
run mode *args: (build mode)
    '{{ os-build-dir }}/parent' {{ args }}

# start debugger
debug: (build 'debug')
    {{ cd }} '{{ os-build-dir / "debug" }}'

# clean project's build directory
clean:
    rm -rf '{{ os-build-dir }}'

# run a memory error detector valgrind
test mode *args: (build mode)
    {{ ct }} --leak-check=full --show-leak-kinds=all --track-origins=yes '{{ os-build-dir }}/parent' {{ args }}