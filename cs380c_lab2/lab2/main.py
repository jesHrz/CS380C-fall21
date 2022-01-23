#!/usr/bin/python3
# coding=utf-8
import sys, getopt, os
from program import Program

default_args = {'-c':0, '-r':0, '-l':0, '-p': 0}

def usage():
    print( '\nUsage: python main.py [-c] [-r] [-l] [-p]')
    print( '''\nOptions:\n
    -c, print Control Flow Graph\n
    -p, print Optimization report\n
    -r, perform reaching definitions and use it to perform simple constant propagation\n
    -l, perform live variable analysis and use it to perform dead statement elimination\n
    -h, help\n
    ''')

def parse_args(args, default_args):
    try:
        optlist, args = getopt.getopt(args, 'crlp')
        opt_dict = {k:v for k, v in optlist}
        for k in default_args:
            opt_dict[k] = default_args[k] if k not in opt_dict else 1
        return opt_dict
    except Exception as e:
        print(e)
        usage()
        exit()

def main():
    if len(sys.argv) == 1 or '-h' in sys.argv:
        usage()
        exit()
    opt_dict = parse_args(sys.argv[1:], default_args)
    result=''
    data = sys.stdin.readlines()
    p = Program(data)

    if opt_dict['-c'] == 1:
        p._get_cfg()
        return
    if opt_dict['-r'] == 1:
        result+=p._program_scp()
        p._update()
    if opt_dict['-l'] == 1:
        result+=p._program_dse()
        p._update()

    if opt_dict['-p'] == 1:
        print(result)
    else:
        print(p)

if __name__ == '__main__':
    main()
