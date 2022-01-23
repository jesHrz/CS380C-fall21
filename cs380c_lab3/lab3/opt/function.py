from block import Block
import sys
import collections
from scc import Graph
class Function(object):
    ARITHMETIC = ['add', 'sub', 'mul', 'div', 'mod', 'neg']
    def __init__(self,func_id,ed_id,instrs):
        self.func_id=func_id
        self.ed_id=ed_id
        self.blocks={}
        self.cfgs={}
        self.reverse_cfg={}
        self.block_num=0
        self._get_blocks(instrs)
        self._get_cfg()
        #reaching definitions IN and OUT
        self.gen_instr={}
        self.kill_instr={}
        self.rd_in={}
        self.rd_out={}
        self.var={}
        self.propa_num=0
        self.scr=[]
        self._get_scr()
        #live variable
        self.num_in_scr=0
        self.num_not_in_scr=0
        self.def_var={}
        self.use_var={}
        self.use_var1={}
        self.lv_in={}
        self.lv_out={}
        self.call=[]
        self.get_call()
        self.global_var=[]
        self.get_global_var()

    def get_global_var(self):
        for b in self.blocks:
            self.global_var+=self.blocks[b].global_var

    @classmethod
    def get_r(cls,instr):
        return int(instr[instr.find('instr ') + 6:instr.find(':')])

    def get_call(self):
        for b in self.blocks:
            self.call+=self.blocks[b].call

    def _return_basic_blocks(self):
        return str(list(self.blocks.keys())).replace(',','').replace('[','').replace(']','')

    def _return_cfg(self):
        str1=''
        for cfg in self.cfgs:
            str1+=str(cfg)
            str1+=' -> '
            tmp=str(self.cfgs[cfg]).replace(',','').replace('[','').replace(']','')
            str1+=tmp
            str1+='\n'
        return str1[:-2]

    def _get_blocks(self,instrs):
        basic_block_st = []
        exprs={}
        for instr in instrs:
            exprs[Function.get_r(instr)]=instr[instr.find(':')+1:].replace('\n','').strip()
        for expr in exprs:
            t=exprs[expr]
            if 'enter' in t or 'entrypc' in t:
                basic_block_st.append(expr)
            elif 'call' in t:
                basic_block_st.append(expr + 1)
            elif 'br' in t or 'blbc' in t or 'blbs' in t:
                t1 = int(t[t.find('[') + 1:t.find(']')])
                basic_block_st.append(t1)
                basic_block_st.append(expr+1)

        basic_block_st = list(set(basic_block_st))
        basic_block_st.sort()
        self.block_num=len(basic_block_st)
        blocks_instr=[]
        for i in range(self.block_num):
            block_instr={}
            t=basic_block_st[i]
            if i<self.block_num-1:
                t1=basic_block_st[i+1]
                while t<t1:
                    block_instr[t]=exprs[t]
                    t+=1
            else:
                while t<=max(exprs.keys()):
                    block_instr[t]=exprs[t]
                    t+=1
            blocks_instr.append(block_instr)
            self.blocks[basic_block_st[i]]=Block(basic_block_st[i],self.func_id,block_instr)

    def _get_cfg(self):

        for block in self.blocks:
            self.cfgs[block]=[]
            self.reverse_cfg[block]=[]
        for block in self.blocks:
            tmp=self.blocks[block].jump
            for t in tmp:
                if t[1]>self.ed_id:
                    self.blocks[block].jump.remove(t)
                else:
                    self.cfgs[t[0]].append(t[1])
        for c in self.cfgs:
            self.cfgs[c].sort()
            for next in self.cfgs[c]:
                self.reverse_cfg[next].append(c)

    @classmethod
    def _add_var(cls,a,b,c):
        if b not in a.keys():
            a[b]=[c]
        else:
            a[b].append(c)

    @classmethod
    def _add_var1(cls,a,b,c):
        if b not in a.keys():
            a[b]=c
        elif a[b][0]<c[0]:
            a[b]=c

    def _get_gen_kill(self):

        var={}
        gen_instr={}
        kill_instr={}
        for b in self.blocks:
            gen_var=self.blocks[b].gen_var
            gen_instr[b] = {}
            kill_instr[b] = {}
            for v in gen_var:
                if v in var.keys():
                    var[v]+=gen_var[v]
                else:
                    var[v]=gen_var[v]
        for v in var:
            t=var[v]
            for i in range(len(t)):
                self._add_var(gen_instr[t[i][0]],v,[t[i][1],t[i][2]])
                # gen_instr[t[i][0]][v].append([t[i][1], t[i][2]])
                for j in range(len(t)):
                    if i!=j:
                        self._add_var(kill_instr[t[i][0]], v, [t[i][1], t[i][2]])
                        # kill_instr[t[j][0]][v].append([t[j][1],t[j][2]])
            var[v]=[]
        self.var=var
        self.gen_instr=gen_instr
        self.kill_instr=kill_instr

    @classmethod
    def _get_difference(cls,a,b):
        result={}
        for key in a:
            if key in b.keys():
                for i in range(len(a[key])):
                    if a[key][i] not in b[key]:
                        if key in result.keys():
                            result[key].append(a[key][i])
                        else:
                            result[key]=[a[key][i]]
            else:
                result[key]=a[key]
        return result

    @classmethod
    def _get_union(cls,a,b):
        result=a
        for key in b:
            if key not in result.keys():
                 result[key]=b[key]
            else:
                for i in range(len(b[key])):
                    if b[key][i] not in result[key]:
                        result[key].append(b[key][i])
        return result

    def _get_reaching_definitions(self):
        self._get_gen_kill()
        IN = {}
        OUT = {}
        for b in self.cfgs:
            IN[b] = {}
            OUT[b] ={}
        flag = 1
        while flag:
            flag = 0
            for b in self.cfgs:
                tmp={}
                for s in self.reverse_cfg[b]:
                    tmp = self._get_union(tmp,OUT[s])
                IN[b] = tmp

                tmp1=self._get_union(self.gen_instr[b],self._get_difference(IN[b],self.kill_instr[b]))
                # tmp1.sort()
                if tmp1 != OUT[b]:
                    OUT[b] = tmp1
                    flag = 1
        self.rd_in=IN
        # print(IN)
        self.rd_out=OUT

    def _constant_propagation(self):
        constant = {}
        need_remove = []

        for b in self.blocks:
            delete_use = {}
            self.blocks[b]._constant_propagation_in_block()
            self.propa_num += self.blocks[b].cons_propa
            self.blocks[b]._get_gen_var()
            use_var = self.blocks[b].use_var
            for v in use_var:

                if v in self.rd_in[b]:
                    if len(self.rd_in[b][v]) == 1:
                        if self.rd_in[b][v][0][1].isdigit():
                            for i in range(len(use_var[v])):
                                if 'is_prime_base' in v:
                                    continue
                                if len(self.call)>0:
                                    if v in self.blocks[b].global_var and use_var[v][i]>min(self.call[0]):
                                        continue
                                flag = 0
                                t = self.blocks[b].instrs[use_var[v][i]]
                                if t.op=='load' and t.operand1_type=='r':
                                    self._add_var(delete_use, t.instr_id, int(t.operand1[1:-1]))
                                    self.blocks[b].instrs[use_var[v][i]].operand1 = int(self.rd_in[b][v][0][1])
                                    self.blocks[b].instrs[use_var[v][i]].operand1_type = 'n'
                                    self.propa_num += 1
                                    constant[use_var[v][i]] =[b, int(self.rd_in[b][v][0][1])]
                                    flag += 1
                                if t.operand1_type in ['r', 'v'] and t.operand1 == v:
                                    if t.operand1_type == 'r':
                                        self._add_var(delete_use, t.instr_id, int(t.operand1[1:-1]))
                                    # if t.operand1==self.rd_in[b][v]
                                    self.blocks[b].instrs[use_var[v][i]].operand1 = int(self.rd_in[b][v][0][1])
                                    self.blocks[b].instrs[use_var[v][i]].operand1_type = 'n'
                                    self.propa_num += 1
                                    flag += 1

                                if t.operand2_type in ['r', 'v'] and t.operand2 == v:
                                    if t.operand2_type == 'r':
                                        self._add_var(delete_use, t.instr_id, int(t.operand2[1:-1]))
                                    self.blocks[b].instrs[use_var[v][i]].operand2 = int(self.rd_in[b][v][0][1])
                                    self.blocks[b].instrs[use_var[v][i]].operand2_type = 'n'
                                    self.propa_num += 1
                                    flag += 1
                                if flag == 2:
                                    need_remove.append([b, use_var[v][i]])
                                if flag >= 1:
                                    if t.op in ['add', 'sub', 'mul', 'div', 'mod']:
                                        if t.operand1_type == 'n' and t.operand2_type == 'n':
                                            if t.op == 'add':
                                                result = int(t.operand1) + int(t.operand2)
                                            elif t.op == 'sub':
                                                result = int(t.operand1) - int(t.operand2)
                                            elif t.op == 'mul':
                                                result = int(t.operand1) * int(t.operand2)
                                            elif t.op == 'div':
                                                result = int(int(t.operand1) / int(t.operand2))
                                            elif t.op == 'mod':
                                                result = int(t.operand1) % int(t.operand2)
                                            constant[use_var[v][i]] = [b,result]

            # print(delete_use)
            l=len(delete_use)
            need_delete=[]
            while True:
                tmp=[]
                for instr in delete_use:
                    tmp+=delete_use[instr]
                for i in range(len(tmp)):
                    delete_use[tmp[i]]=[]
                    if self.blocks[b].instrs[tmp[i]].operand1_type=='r':
                        delete_use[tmp[i]].append(int(self.blocks[b].instrs[tmp[i]].operand1[1:-1]))
                    if self.blocks[b].instrs[tmp[i]].operand2_type=='r':
                        delete_use[tmp[i]].append(int(self.blocks[b].instrs[tmp[i]].operand2[1:-1]))
                if l==len(delete_use):
                    break
                l=len(delete_use)
            for instr in delete_use:
                del self.blocks[b].instrs[instr]
        # print(constant)
        n = 0
        l = len(constant)
        # print(constant)
        while True:
            for b in self.blocks:
                for instr in self.blocks[b].instrs:
                    flag = 0
                    t = self.blocks[b].instrs[instr]
                    if t.operand1_type == 'r':
                        if int(t.operand1[1:-1]) in constant.keys():
                            self.blocks[b].instrs[instr].operand1 = constant[int(t.operand1[1:-1])][1]
                            self.blocks[b].instrs[instr].operand1_type = 'n'
                            n += 1
                            flag += 1
                    if t.operand2_type == 'r':
                        if int(t.operand2[1:-1]) in constant.keys():
                            self.blocks[b].instrs[instr].operand2 = constant[int(t.operand2[1:-1])][1]
                            self.blocks[b].instrs[instr].operand2_type = 'n'
                            n += 1
                            flag += 1
                    if flag == 2:
                        need_remove.append([b, instr])
                        if t.op in ['add', 'sub', 'mul', 'div', 'mod']:
                            if t.operand1_type == 'n' and t.operand2_type == 'n':
                                if t.op == 'add':
                                    result = int(t.operand1) + int(t.operand2)
                                elif t.op == 'sub':
                                    result = int(t.operand1) - int(t.operand2)
                                elif t.op == 'mul':
                                    result = int(t.operand1) * int(t.operand2)
                                elif t.op == 'div':
                                    result =int(int(t.operand1) / int(t.operand2))
                                elif t.op == 'mod':
                                    result = int(t.operand1) % int(t.operand2)
                                constant[t.instr_id] = [b,result]
            if len(constant) == l:
                break
            l = len(constant)
        self.propa_num += n
        for instr in constant:
            if instr in self.blocks[constant[instr][0]].instrs.keys():
                del self.blocks[constant[instr][0]].instrs[instr]

    def _scp(self):
        self._get_reaching_definitions()
        self._constant_propagation()

    def _get_def_use(self):
        for b in self.blocks:
            self.def_var[b]=self.blocks[b].def_var_live
            self.use_var1[b]=self.blocks[b].use_var_live
    def _live_variable_analysis(self):
        self._get_def_use()
        IN = {}
        OUT = {}
        for b in self.cfgs:
            IN[b] = []
            OUT[b] = []
        flag = 1
        while flag:
            flag = 0
            for b in self.cfgs:
                tmp = set()
                for s in self.cfgs[b]:
                    tmp = tmp.union(set(IN[s]))
                OUT[b] = list(tmp)
                tmp1 = list(set(self.use_var1[b]).union(set(OUT[b]).difference(set(self.def_var[b]))))
                if tmp1 != IN[b]:
                    IN[b] = list(tmp1)
                    flag = 1
        self.lv_in = IN
        self.lv_out = OUT

    def _dead_code_elimination(self):
        self._live_variable_analysis()
        delete_block = []
        for b in self.blocks:
            buchong = []
            for out in self.lv_out[b]:
                if 'base' in out:
                    buchong.append(out[:out.find('#')])
            self.lv_out[b] += buchong
            self.lv_out[b] = list(set(self.lv_out[b]))

            need_to_delete = {}
            for def_var in self.def_var[b]:

                if def_var in self.global_var:
                    continue
                if def_var in self.blocks[b].use_var.keys():
                    continue
                if 'temp' in def_var:
                    continue
                if def_var not in self.lv_out[b]:
                    if 'base' in def_var:
                        if def_var[:def_var.find('#')] not in self.lv_out[b]:
                            need_to_delete[def_var] = self.def_var[b][def_var]
                    else:
                        need_to_delete[def_var] = self.def_var[b][def_var]
            for var in need_to_delete:
                t = need_to_delete[var]
                for i in range(len(t)):
                    if 'base' not in var:

                        del self.blocks[t[i][0]].instrs[t[i][1]]
                        delete_block.append(t[i][0])
                    else:
                        del self.blocks[t[i][0]].instrs[t[i][1]]
                        delete_instr = t[i][1] - 1
                        delete_block.append(t[i][0])
                        while 'base' not in self.blocks[t[i][0]].instrs[delete_instr].operand1:
                            del self.blocks[t[i][0]].instrs[delete_instr]
                            delete_instr -= 1
                            delete_block.append(t[i][0])
                        del self.blocks[t[i][0]].instrs[delete_instr]
                        delete_block.append(t[i][0])
                        if delete_instr in self.blocks[t[i][0]].instrs.keys():
                            if 'mul' == self.blocks[t[i][0]].instrs[delete_instr - 1].op:
                                del self.blocks[t[i][0]].instrs[delete_instr - 1]
                                delete_block.append(t[i][0])
        register_used = []
        instr_id = []
        for b in self.blocks:

            for instr in self.blocks[b].instrs:
                t = self.blocks[b].instrs[instr]
                if t.op in self.ARITHMETIC:
                    if t.operand1_type == 'n' and t.operand2_type == 'n':
                        instr_id.append([b, instr])
                if t.operand1_type == 'r':
                    register_used.append(int(t.operand1[1:-1]))
                if t.operand2_type == 'r':
                    register_used.append(int(t.operand2[1:-1]))

        for i in range(len(instr_id)):
            if instr_id[i][1] not in register_used:
                del self.blocks[instr_id[i][0]].instrs[instr_id[i][1]]
                delete_block.append(instr_id[i][0])
        for i in range(len(delete_block)):
            if delete_block[i] in self.scr:
                self.num_in_scr += 1
            else:
                self.num_not_in_scr += 1

    # Depth first search with postorder append to stack
    def dfs(self,g, u, stack):
        g.explored[u] = 1
        if u in g.graph:
            for v in g.graph[u]:
                if not g.explored[v]:
                    self.dfs(g, v, stack)
        stack.append(u)

    def scc(self,g):
        results = []
        # Initial DFS on G
        search_stack = []
        for v, explored in enumerate(g.explored):
            if not explored and v > 0:
               self.dfs(g, v, search_stack)

        # Reverse Graph
        gr = g.reverse()

        # DFS ordered by search_stack
        while len(search_stack) > 0:
            u = search_stack[-1]
            scc_stack = []
            self.dfs(gr, u, scc_stack)
            for v in scc_stack:
                if v in gr.graph:
                    del gr.graph[v]
                search_stack.remove(v)
            if len(scc_stack)>1:
                results.append(scc_stack)
        return results

    def _get_scr(self):
        e=0
        for f in self.cfgs:
            e+=len(self.cfgs[f])
        replace1={}
        replace2={}
        i=1
        for n in self.cfgs:
            replace1[i]=n
            replace2[n]=i
            i+=1
        new_cfg={}
        for n in self.cfgs:
            new_cfg[replace2[n]]=[]
            for i in range(len(self.cfgs[n])):
                new_cfg[replace2[n]].append(replace2[self.cfgs[n][i]])
        G=Graph(new_cfg,self.block_num,e)
        result=self.scc(G)
        scr=[]
        for i in range(len(result)):
            t=[]
            for j in range(len(result[i])):
                t.append(replace1[result[i][j]])
            scr.append(t)
        scr_id=[]
        for i in range(len(scr)):
            for j in range(len(scr[i])):
                scr_id.append(scr[i][j])
        self.scr=list(set(scr_id))


