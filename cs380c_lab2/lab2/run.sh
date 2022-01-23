#!/usr/bin/env bash
OPTIMIZER=./main.py
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

if [ $OPT = "scp" ]
then
    OPT="-r"
elif [ $OPT = "dse" ]
then
    OPT="-l"
else
    echo "Unknown -opt=${OPT}"
    exit 1
fi

if [ $BACKEND = "c" ]
then
    OPT=${OPT}
elif [ $BACKEND = "cfg" ]
then
    OPT="${OPT} -c"
elif [ $BACKEND = "3addr" ]
then
    OPT=${OPT}
elif [ $BACKEND = "rep" ]
then
    OPT="${OPT} -p"
else
    echo "Unknown -backend=${BACKEND}"
    exit 1
fi
${OPTIMIZER} ${OPT} |
if [ $BACKEND = "c" ]; then ${THREE_ADDR_TO_C_TRANSLATOR}; else cat; fi
