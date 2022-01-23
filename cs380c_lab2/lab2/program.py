from function import Function
import os

class Program(object):

    def __init__(self,data):

        self.functions = []
        t = []
        i=0
        while True:
            if i>=len(data)-1:
                break
            if 'nop' in data[i]:
                i+=1
                continue
            while 'ret' not in data[i]:
                t.append(data[i])
                i+=1
            t.append(data[i])
            name = int(t[0][t[0].find('instr ') + 6:t[0].find(':')])
            ed_id = int(t[-1][t[-1].find('instr ') + 6:t[-1].find(':')])
            self.functions.append(Function(name, ed_id, t))
            t=[]
            i+=1

        # self._update()
        self.jump_instr={}
        self._get_jump_instr()

    def _get_cfg(self):
        result = ''
        for f in self.functions:
            result+='Function:'
            result+=str(f.func_id)
            result+='\n'
            result+='Basic blocks:'
            result+=f._return_basic_blocks()
            result+='\n'
            result+='CFG:\n'
            result+=f._return_cfg()
            result+='\n'
        print(result)


    def _get_jump_instr(self):
        for f in range(len(self.functions)):
            for b in self.functions[f].blocks:
                for instr in self.functions[f].blocks[b].instrs:
                    t = self.functions[f].blocks[b].instrs[instr]
                    if t.operand1_type=='i':
                        self.jump_instr[int(t.operand1[1:-1])]=[b,instr,1]
                    if t.operand2_type == 'i':
                        self.jump_instr[int(t.operand2[1:-1])] =[b,instr,2]

    def _program_scp(self):
        result=''
        for i in range(len(self.functions)):
            t=self.functions[i]
            t._scp()

            result+='Function:'
            result+=str(t.func_id)
            result+='\n'
            result+='Number of constants propagated:'
            result+=str(t.propa_num)
            result+='\n'
        return result
    def _program_dse(self):
        result=''
        for i in range(len(self.functions)):
            t=self.functions[i]
            t._dead_code_elimination()
            result += 'Function:'
            result += str(t.func_id)
            result += '\n'
            result+='Number of statements eliminated in SCR:'
            result+=str(t.num_in_scr)
            result+='\n'
            result+='Number of statements eliminated not in SCR:'
            result+=str(t.num_not_in_scr)
            result+='\n'
        return result

    def __repr__(self):
        result=''
        for f in self.functions:
            for b in f.blocks:
                for instr in f.blocks[b].instrs:
                    result+=str(f.blocks[b].instrs[instr])
                    result+='\n'
        return result


    def _update(self):
        i=1
        replace={}
        instr_id=[]

        for f in self.functions:
            for b in f.blocks:
                instr_id += list(f.blocks[b].instrs.keys())
        for f in self.functions:
            for b in f.blocks:
                for instr in f.blocks[b].instrs:
                    replace[instr]=i
                    f.blocks[b].instrs[instr].instr_id=i
                    i+=1
                    t=f.blocks[b].instrs[instr]
                    if t.operand1_type=='i':
                        if int(t.operand1[1:-1]) not in instr_id:
                            t1=int(t.operand1[1:-1])
                            while t1 not in instr_id:
                                t1+=1
                            f.blocks[b].instrs[instr].operand1='['+str(t1)+']'
                    if t.operand2_type=='i':
                        if int(t.operand2[1:-1]) not in instr_id:
                            t1=int(t.operand2[1:-1])
                            # print(t1)
                            while t1 not in instr_id:
                                t1+=1
                            f.blocks[b].instrs[instr].operand2='['+str(t1)+']'

        for f in range(len(self.functions)):
            for b in self.functions[f].blocks:
                for instr in self.functions[f].blocks[b].instrs:
                    t=self.functions[f].blocks[b].instrs[instr]
                    if t.operand1_type=='r':
                        t1 = int(t.operand1[1:-1])
                        self.functions[f].blocks[b].instrs[instr].operand1='('+str(replace[t1])+')'
                    if t.operand2_type == 'r':
                        t2 = int(t.operand2[1:-1])
                        self.functions[f].blocks[b].instrs[instr].operand2 ='('+str(replace[t2])+')'
                    if t.operand1_type=='i':
                        t1 = int(t.operand1[1:-1])
                        self.functions[f].blocks[b].instrs[instr].operand1='['+str(replace[t1])+']'
                    if t.operand2_type == 'i':
                        t2 = int(t.operand2[1:-1])
                        self.functions[f].blocks[b].instrs[instr].operand2 ='['+str(replace[t2])+']'

