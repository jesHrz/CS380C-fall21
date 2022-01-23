from instruction import Instruction
import collections


class Block(object):
    ARITHMETIC = ['add', 'sub', 'mul', 'div', 'mod', 'neg', 'cmpeq', 'cmple', 'cmplt']
    CBR_OPS = ['blbc', 'blbs']  # conditional branch
    IO = ['read', 'write', 'wrl']

    def __init__(self, st_id, func_id, instrs):
        self.block_id = st_id
        self.st_id = st_id
        self.ed_id = max(instrs.keys())
        self.func_id = func_id
        self.instrs = collections.OrderedDict()
        self.instr_num = len(instrs)
        # for live variable analysis
        self.def_var_live = {}
        self.use_var_live = {}
        self.global_var = []
        self._get_instr(instrs)
        self.call = []
        self._get_call()
        # 存储successor,self.jump=[[target_block_id,target_instr_id],...]
        # for reaching definitions
        self.use_var = {}
        self.jump = []
        self.gen_var = {}
        self._get_global_var()
        self.global_var = list(set(self.global_var))
        self._get_gen_var()
        self.phi_var=[]
        self.cons_propa = 0
        self._get_successor()
        self._get_def_use()


        # self._constant_propagation_in_block()

    def _get_instr(self, instrs):
        for ins in instrs:
            self.instrs[ins] = Instruction(ins, self.block_id, instrs[ins])

    def _get_successor(self):
        last_instr = max(self.instrs.keys())
        tmp = self.instrs[last_instr]
        if tmp.op in self.CBR_OPS:
            self.jump.append(tmp.jump)
            self.jump.append([self.block_id, self.ed_id + 1])
        elif tmp.op == 'br':
            self.jump.append(tmp.jump)
        else:
            self.jump.append([self.block_id, self.ed_id + 1])

    def _get_phi_var(self):
        for instr in self.blocks:
            if self.blocks[instr].op=='phi':
                var=self.blocks[instr].split(' ')[1:]
                self.phi_var+=var

    def _get_def_use(self):
        tmp1 = []
        for key in self.instrs:
            tmp1.append([key, self.instrs[key]])
        def_var = {}
        use_var = {}

        for i in range(len(tmp1)):
            t = tmp1[i][1]

            if t.op == 'move':
                if t.operand2_type in ['v', 'b']:
                    # self._add_var(def_var,t.operand2,[tmp1[i][0],t.operand1])
                    if t.operand2 not in use_var.keys():
                        self._add_var(def_var, t.operand2, [self.block_id, tmp1[i][0], t.operand1])
                if t.operand1_type in ['v', 'b']:
                    if t.operand1 not in def_var.keys():
                        self._add_var(use_var, t.operand1, tmp1[i][0])

            elif t.op in ['nop', 'wrl', 'entrypc', 'enter', 'ret', 'br', 'call']:
                continue
            elif t.op in ['param', 'write', 'blbc', 'blbs'] and t.operand1_type in ['v', 'b']:
                if t.operand1 not in def_var.keys():
                    self._add_var(use_var, t.operand1, tmp1[i][0])
            elif t.op in ['cmpeq', 'cmple', 'cmplt', 'add', 'sub', 'mul', 'div', 'mod', 'neg'] and t.operand2 not in [
                'GP', 'FP']:

                if t.operand1_type in ['v', 'b']:
                    if t.operand1 not in def_var.keys():
                        self._add_var(use_var, t.operand1, tmp1[i][0])
                if t.operand2_type in ['v', 'b']:
                    if t.operand2 not in def_var.keys():
                        self._add_var(use_var, t.operand2, tmp1[i][0])
            elif t.op == 'add' and t.operand2 in ['GP', 'FP']:
                var_name = t.operand1
                # array
                k = i
                flag = 0
                if tmp1[i - 1][1].op == 'mul' and tmp1[i - 1][1].operand2_type == 'n':
                    flag = 1
                    var_name += '['
                    var_name += str(tmp1[i - 1][1].operand1)
                    var_name += ']'
                    k += 1
                    if int(tmp1[k - 2][1].operand2) > 8:
                        while tmp1[k + 1][1].op == 'mul' and int(tmp1[k + 1][1].operand2) >= 8:
                            var_name += '['
                            var_name += str(tmp1[k + 1][1].operand1)
                            var_name += ']'
                            k += 2
                            if int(tmp1[k - 1][1].operand2) == 8:
                                break
                # struct
                if tmp1[k + 1][1].operand2_type == 'o':
                    flag = 1
                    while tmp1[k + 1][1].operand2_type == 'o':
                        var_name += '.'
                        var_name += str(tmp1[k + 1][1].operand2)
                        k += 1

                # local variable
                if flag == 0:
                    var_name = t.operand1

                address = tmp1[k][0]
                k = i + 1
                while True:
                    if k == len(tmp1):
                        break
                    if tmp1[k][1].op == 'load' and tmp1[k][1].operand1 == '(' + str(address) + ')':
                        if var_name not in def_var.keys():
                            self._add_var(use_var, var_name, tmp1[k][0])
                        break
                    elif tmp1[k][1].op == 'store':
                        if tmp1[k][1].operand1 == '(' + str(address) + ')':
                            if var_name not in def_var.keys():
                                self._add_var(use_var, var_name, tmp1[k][0])
                            break
                        elif tmp1[k][1].operand2 == '(' + str(address) + ')':
                            if var_name not in use_var.keys():
                                self._add_var(def_var, var_name, [self.block_id, tmp1[k][0], tmp1[k][1].operand1])
                            # self._add_var(def_var, var_name, [tmp1[k][0],tmp1[k][1].operand1])
                            break
                    k += 1

        for v in def_var:
            def_var[v].sort(key=lambda x: x[0])

        self.def_var_live = def_var
        self.use_var_live = use_var

    def _get_call(self):
        for instr in self.instrs:
            if self.instrs[instr].op == 'call':
                self.call.append([self.block_id, instr])

    def _get_global_var(self):
        for instr in self.instrs:
            t = self.instrs[instr]
            if t.operand2 == 'GP':
                self.global_var.append(t.operand1)

    @classmethod
    def _add_var(cls, a, b, c):
        if b not in a.keys():
            a[b] = [c]
        else:
            a[b].append(c)

    @classmethod
    def _add_var1(cls, a, b, c):
        if b not in a.keys():
            a[b] = {c[1]: [c[0], c[2]]}
        else:
            a[b][c[1]] = [c[0], c[2]]

    # for reaching definition:
    # 获得每个block生成和使用的变量
    # self.gen_var={'var1':[[instr_id,new_var]...],'var2':[[instr_id,new_var]...],...}
    # gen_var记录每个新定义的变量及相应的指令和对其的赋值
    # self.use_var={var1:[instr1,instr2],var2:[instr1,instr2],...}
    # use_var记录每个变量被使用的指令位置
    def _get_gen_var(self):
        tmp1 = []
        for key in self.instrs:
            tmp1.append([key, self.instrs[key]])
        def_var = {}
        use_var = {}

        for i in range(len(tmp1)):
            t = tmp1[i][1]

            if t.op == 'move' and t.operand2_type == 'v':
                # self._add_var(def_var,t.operand2,[tmp1[i][0],t.operand1])
                self._add_var(def_var, t.operand2, [self.block_id, tmp1[i][0], t.operand1])

            elif t.op in ['nop', 'wrl', 'entrypc', 'enter', 'ret', 'br', 'call']:
                continue
            elif t.op in ['param', 'write', 'blbc', 'blbs'] and t.operand1_type in ['v', 'b']:
                self._add_var(use_var, t.operand1, tmp1[i][0])
            elif t.op in ['cmpeq', 'cmple', 'cmplt', 'add', 'sub', 'mul', 'div', 'mod', 'neg'] and t.operand2 not in [
                'GP', 'FP']:

                if t.operand1_type in ['v', 'b']:
                    self._add_var(use_var, t.operand1, tmp1[i][0])
                if t.operand2_type in ['v', 'b']:
                    self._add_var(use_var, t.operand2, tmp1[i][0])
            elif t.op == 'add' and t.operand2 in ['GP', 'FP']:
                var_name = t.operand1
                # array
                k = i
                flag = 0
                if tmp1[i - 1][1].op == 'mul' and tmp1[i - 1][1].operand2_type == 'n':
                    flag = 1
                    var_name += '['
                    var_name += str(tmp1[i - 1][1].operand1)
                    var_name += ']'
                    k += 1
                    if int(tmp1[k - 2][1].operand2) > 8:
                        while tmp1[k + 1][1].op == 'mul' and int(tmp1[k + 1][1].operand2) >= 8:
                            var_name += '['
                            var_name += str(tmp1[k + 1][1].operand1)
                            var_name += ']'
                            k += 2
                            if int(tmp1[k - 1][1].operand2) == 8:
                                break
                # struct
                if tmp1[k + 1][1].operand2_type == 'o':
                    flag = 1
                    while tmp1[k + 1][1].operand2_type == 'o':
                        var_name += '.'
                        var_name += str(tmp1[k + 1][1].operand2)
                        k += 1

                # local variable
                if flag == 0:
                    var_name = t.operand1

                address = tmp1[k][0]
                k = i + 1
                while True:
                    if k == len(tmp1):
                        break
                    if tmp1[k][1].op == 'load' and tmp1[k][1].operand1 == '(' + str(address) + ')':
                        self._add_var(use_var, var_name, tmp1[k][0])
                        break
                    elif tmp1[k][1].op == 'store':
                        if tmp1[k][1].operand1 == '(' + str(address) + ')':
                            self._add_var(use_var, var_name, tmp1[k][0])
                            break
                        elif tmp1[k][1].operand2 == '(' + str(address) + ')':
                            self._add_var(def_var, var_name, [self.block_id, tmp1[k][0], tmp1[k][1].operand1])
                            break
                    k += 1
        self.use_var = use_var
        for v in def_var:
            def_var[v].sort(key=lambda x: x[0])
        self.gen_var = def_var

        # 单个block内的constant propagation

    @classmethod
    def can_replace(self, a, b):
        for i in range(len(a)):
            if i < len(a) - 1:
                if a[i][1] < b and a[i + 1][1] > b:
                    return a[i][2]
            elif i == len(a) - 1 and a[i][1] < b:
                return a[i][2]
            else:
                return 'undef'

    def _constant_propagation_in_block(self):
        # print('block_id:',str(self.block_id))
        replace = collections.OrderedDict()
        n = collections.OrderedDict()
        num = 0
        tmp1 = []

        del_instr = []

        for key in self.instrs:
            tmp1.append([key, self.instrs[key]])
        while True:
            l = len(replace)
            # update instr
            k = 0
            for var in self.gen_var:
                for i in range(len(self.gen_var[var])):
                    if self.gen_var[var][i][2] in list(replace.keys()):
                        self.gen_var[var][i][2] = replace[self.gen_var[var][i][2]]
            need_to_add = {}
            for instr in self.instrs:
                if self.instrs[instr].operand1 in replace.keys():
                    for key in replace:
                        if key == self.instrs[instr].operand1:
                            need_to_add[instr] = self.instrs[instr].operand1
                            self.instrs[instr].operand1 = str(replace[key])
                            self.instrs[instr].operand1_type = 'n'
                            num += 1
                elif self.instrs[instr].operand2 in replace.keys():
                    for key in replace:
                        if key == self.instrs[instr].operand2:
                            if tmp1[k - 2][1].op == 'mul' and tmp1[k - 1][1].operand1_type == 'b':
                                continue
                            else:
                                need_to_add[instr] = self.instrs[instr].operand2
                                self.instrs[instr].operand2 = str(replace[key])
                                self.instrs[instr].operand2_type = 'n'
                                num += 1
            for key in need_to_add:
                replace[key] = need_to_add[key]
            for instr in self.instrs:
                tmp = self.instrs[instr]
                if tmp.op in ['add', 'sub', 'mul', 'div', 'mod']:
                    if tmp.operand1_type == 'n' and tmp.operand2_type == 'n':
                        if tmp.op == 'add':
                            result = int(tmp.operand1) + int(tmp.operand2)
                        elif tmp.op == 'sub':
                            result = int(tmp.operand1) - int(tmp.operand2)
                        elif tmp.op == 'mul':
                            result = int(tmp.operand1) * int(tmp.operand2)
                        elif tmp.op == 'div':
                            result = int(int(tmp.operand1) / int(tmp.operand2))
                        elif tmp.op == 'mod':
                            result = int(tmp.operand1) % int(tmp.operand2)
                        if tmp.op == 'mul' and self.instrs[instr + 1].operand1_type == 'b':
                            continue
                        replace['(' + str(instr) + ')'] = str(result)
                        # update gen_var
                        self._get_gen_var()

                k += 1
            for v in self.use_var:
                tmp_use = self.use_var[v]
                if v not in self.gen_var.keys():
                    continue
                for i in range(len(tmp_use)):
                    tmp_def = ['', '', '']
                    if tmp_use[i] < self.gen_var[v][0][1]:
                        continue
                    for j in range(len(self.gen_var[v])):
                        if j < len(self.gen_var[v]) - 1:
                            if self.gen_var[v][j][1] < tmp_use[i] and self.gen_var[v][j + 1][1] > tmp_use[i]:
                                tmp_def = self.gen_var[v][j]
                        elif j == len(self.gen_var[v]) - 1 and self.gen_var[v][j][1] < tmp_use[i]:
                            tmp_def = self.gen_var[v][j]
                    if str(tmp_def[2]).isdigit():
                        replace['(' + str(tmp_use[i]) + ')'] = int(tmp_def[2])
            if l == len(replace):
                break
            l = len(replace)

        del_instr = list(set(del_instr))
        for instr in del_instr:
            del self.instrs[instr]

        register = []
        for instr in self.instrs:
            if self.instrs[instr].operand1_type == 'r':
                register.append(int(self.instrs[instr].operand1[1:-1]))
            if self.instrs[instr].operand2_type == 'r':
                register.append(int(self.instrs[instr].operand2[1:-1]))

        need_delete = []
        for instr in self.instrs:
            if self.instrs[instr].op in ['add', 'mul', 'sub', 'div', 'mod']:
                if self.instrs[instr].op == 'mul' and self.instrs[instr + 1].operand1_type == 'b':
                    continue
                if instr not in register:
                    need_delete.append(instr)
            if self.instrs[instr].op == 'load' and instr not in register:
                need_delete.append(instr)

        for instr in need_delete:
            t = self.instrs[instr]
            if t.operand1 in self.phi_var:
                del self.instrs[instr]
            elif t.operand2 in self.phi_var:
                del self.instrs[instr]

        for i in range(len(need_delete)):
            t = [need_delete[i]]
            while len(t) > 0:
                t1 = []
                for j in range(len(t)):
                    if self.instrs[t[j]].operand1_type == 'r':
                        need_delete.append(int(self.instrs[t[j]].operand1[1:-1]))
                        t1.append(int(self.instrs[t[j]].operand1[1:-1]))
                    if self.instrs[t[j]].operand2_type == 'r':
                        need_delete.append(int(self.instrs[t[j]].operand2[1:-1]))
                        t1.append(int(self.instrs[t[j]].operand2[1:-1]))
                t = t1

        need_delete = list(set(need_delete))
        for i in range(len(need_delete)):
            del self.instrs[need_delete[i]]

        self.cons_propa = num




