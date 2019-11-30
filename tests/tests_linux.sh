CXX=g++
CFLAGS="-I.. -Wall -Wextra -Wno-unused-parameter -Wno-reorder" 
LDFLAGS="-lX11 -lrt"
SKIP_RUN=$1

function run_test() {
  rm -f test.elf
  echo -e "\n\033[32;1m Building $1 \033[0m"
  ${CXX} ${CFLAGS} $1 -o test.elf ${LDFLAGS} $2
  RUN_RESULT=-1
  if [ -f test.elf ]; then
    if [ "${SKIP_RUN}" != "yes" ]; then
      ./test.elf
      if [ $? -eq 0 ]; then
        echo "OK"
        RUN_RESULT=0
      fi
    else
      RUN_RESULT=0
    fi
  fi
  echo "result: ${RUN_RESULT}"
  return ${RUN_RESULT}
}

let res=0
run_test test_2d.cpp
let res=res+$?
run_test test_gl.cpp "-lGL -lGLU"
let res=res+$?
run_test test_gl_legacy.cpp "-lGL -lGLU"
let res=res+$?
return ${res}

