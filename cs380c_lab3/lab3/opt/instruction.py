import re, collections, copy

class Instruction(object):


    RET_OPS = ['ret']
    BR_OPS = ['br', 'blbc', 'blbs']
    PROC_OPS = ['call'] + RET_OPS
    PROC_START_OP = 'enter'
    JMP_OPS = BR_OPS + ['call'] # although 'ret' is also a jump, it does not have explicit dst addr in the instruction
    CBR_OPS = ['blbc', 'blbs'] # conditional branch
    CJMP_OPS = CBR_OPS
    UBR_OPS = ['br'] # unconditional branch
    UJMP_OPS = UBR_OPS + ['call'] # although 'ret' is also a jump, it does not have explicit dst addr in the instruction
    LEADER_OPS = ['enter']
    INVALID_LEADER_OPS = ['entrypc', 'nop']
    TERMINATOR_OPS = BR_OPS + PROC_OPS
    CMP_OPS = ['cmpeq', 'cmple', 'cmplt']
    CALC_OPS = ['add', 'sub', 'mul', 'div', 'mod', 'neg']
    ARITH_OPS = ['add', 'sub', 'mul', 'div', 'mod', 'neg', 'cmpeq', 'cmple', 'cmplt']
    EXCHANGEABLE_ARITH_OPS = ['add', 'mul', 'cmpeq']
    ARITH_OP_MAP = {'add':'+', 'sub':'-', 'mul':'*', 'div':'/', 'mod':'%', 'neg':'-', 'cmpeq':'==', 'cmple':'<=', 'cmplt':'<'}

    REGISTER_PATTERN = '\((\d+)\)' # e.g. (5)
    INSTR_PATTERN = '\[(\d+)\]' # e.g. [5]
    NUM_PATTERN = '(\d+)' # e.g. 5
    BASE_PATTERN = '[_a-zA-Z0-9]+_base#([-0-9]+)'  # e.g. global_array_base#32576
    OFFSET_PATTERN = '[_a-zA-Z0-9]+_offset#([-0-9]+)' # e.g. y_offset#8
    VAR_PATTERN = '([_a-zA-Z0-9]+)#[-0-9]+' # e.g. i#-8
    PATTERNS = {'r':REGISTER_PATTERN, 'i':INSTR_PATTERN, 'n':NUM_PATTERN, 'b':BASE_PATTERN,
    'o':OFFSET_PATTERN, 'v':VAR_PATTERN}
    EVALUABLE_PATTERN = '^[-.0-9]+( *[%s] *[-.0-9]+)?$' % ''.join(set(''.join(ARITH_OP_MAP.values())))

    JUMP_OPS=['br','blbc','blbs']

    def __init__(self,id,block_id,instr):
        self.instr_id=id
        self.block_id=block_id
        self.instr=instr
        t=instr.split(' ')
        t.append('')
        t.append('')
        self.op=t[0]
        self.operand1=t[1]
        self.operand2=t[2]
        #类型有rigester(r),instr(i),num(n),base(b),offset(o),var(v)
        self.operand1_type=''
        self.operand2_type=''
        self.jump=[]
        self._parse()

    def _parse_operand(self,operand):
        if operand=='':
            return None
        elif operand in ['GP','FP']:
            return operand
        for k,pat in Instruction.PATTERNS.items():
            m=re.match(pat,operand)
            if m!=None:
                return k

    def _parse(self):
        #判断每个operand类型
        self.operand1_type=self._parse_operand(self.operand1)
        self.operand2_type=self._parse_operand(self.operand2)
        #判断该语句是否是跳转语句，若是存储跳转目的地
        if self.op in self.UBR_OPS:
            self.jump=[self.block_id,int(self.operand1[1:-1])]
        elif self.op in self.CBR_OPS:
            self.jump=[self.block_id,int(self.operand2[1:-1])]

    def __repr__(self):
        return 'instr '+str(self.instr_id)+': '+self.op+' '+str(self.operand1)+' '+str(self.operand2)

