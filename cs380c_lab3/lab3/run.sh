#!/usr/bin/env bash
SSA=./ssa
NOSSA=./nonssa
OPTIMIZER=./opt/main.py
THREE_ADDR_TO_C_TRANSLATOR=../../cs380c_lab1/lab1/convert

for i in "$@"; do
  case $i in
    -opt=*)
      OPT="${i#*=}"
      shift # past argument=value
      ;;
    -backend=*)
      BACKEND="${i#*=}"
      shift # past argument=value
      ;;
    -*|--*)
      echo "Unknown option $i"
      exit 1
      ;;
    *)
      ;;
  esac
done

IFS=, read -a opts <<< "${OPT}"
for i in ${opts[@]};
do
    if [ $i = "scp" ]
    then
        _OPT="scp"
    elif [ $i = "dse" ]
    then
        _OPT="dse"
    elif [ $i = "ssa" ]
    then
        DOSSA=1
    else
        echo "Unknown -opt=${OPT}"
        exit 1
    fi
done

DONOSSA=1
IFS=, read -a backends <<< "${BACKEND}"
for i in ${backends[@]};
do
    if [ $i = "c" ]
    then
        _BACKEND="c"
    elif [ $i = "cfg" ]
    then
        _BACKEND="cfg"
    elif [ $i = "3addr" ]
    then
        _BACKEND="3addr"
    elif [ $i = "rep" ]
    then
        _BACKEND="rep"
    elif [ $i = "ssa" ] && [ $DOSSA -eq 1 ]
    then
        DONOSSA=0
    else
        echo "Unknown -backend=${BACKEND}"
        exit 1
    fi
done

if [ $DOSSA -eq 1 ]
then
    if [ $_OPT = "scp" ]
    then
        OPT="-r"
    elif [ $_OPT = "dse" ]
    then
        OPT="-l"
    fi

    if [ $_BACKEND = "c" ]
    then
        OPT=${OPT}
    elif [ $_BACKEND = "cfg" ]
    then
        OPT="${OPT} -c"
    elif [ $_BACKEND = "3addr" ]
    then
        OPT=${OPT}
    elif [ $_BACKEND = "rep" ]
    then
        OPT="${OPT} -rep"
    fi

    ${SSA} | ${OPTIMIZER} ${OPT} |
    if [ $DONOSSA -eq 1 ]; then ${NOSSA}; else cat; fi |
    if [ $_BACKEND = "c" ]; then ${THREE_ADDR_TO_C_TRANSLATOR}; else cat; fi
else
    cd ../../cs380c_lab2/lab2
    ./run.sh -opt=${OPT} -backend=${BACKEND}
fi