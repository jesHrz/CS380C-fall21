#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include <queue>
#include <stack>
#include <list>

#include "Instr.h"
#include "misc.h"

using namespace std;

list<Instr> instrs_ssa;
list<Instr>::iterator instr_ssa;

vector<Instr> instrs_3addr;
vector<Instr>::iterator instr;
int instr_count;

vector<int> cfg_leader;
map<int, int> cfg_idom;
map<int, vector<int>> cfg, cfg_rev, cfg_dom, df;

map<string, vector<Def>> def_var;
map<string, vector<Use>> use_var;

static map<int, bool> visited;

int var_count;
stack<int> var_stack;


static void read_3addr(istream &in) {
    string one_instr;
    while (getline(in, one_instr)) {
        trim(one_instr);
        if (one_instr.empty())
            continue;
        instrs_3addr.emplace_back(Instr(one_instr));
    }
    instr_count = instrs_3addr.size() + 1;
}

static void write_ssa(ostream &out) {
    for (auto &it: instrs_ssa) {
        out << it << endl;
    }
}

static void cleanup() {
    cfg.clear();
    cfg_rev.clear();
    cfg_leader.clear();
    cfg_dom.clear();
    cfg_idom.clear();
    df.clear();
    def_var.clear();
    use_var.clear();
}

static bool build_cfg() {
    auto _instr = instr;
    for (; instr < instrs_3addr.end(); ++instr) {
        if (instr->op == "enter") {
            cfg_leader.push_back(instr->id);
        } else if (CONTAIN_ITEM(UBR_OPS, instr->op) || CONTAIN_ITEM(CBR_OPS, instr->op)) {
            cfg_leader.push_back(instr->jump);
            cfg_leader.push_back(instr->id + 1);
        } else if (instr->op == "call") {
            cfg_leader.push_back(instr->id + 1);
        } else if (instr->op == "ret") {
            break;
        }
    }

    sort(cfg_leader.begin(), cfg_leader.end());
    cfg_leader.resize(unique(cfg_leader.begin(), cfg_leader.end()) - cfg_leader.begin());

    for (int next_bb = 1; _instr < instrs_3addr.end(); ++_instr) {
        if (next_bb < cfg_leader.size() && cfg_leader[next_bb] <= _instr->id)
            next_bb++;
        _instr->block = cfg_leader[next_bb - 1];
        instrs_ssa.push_back(*_instr);
        if (_instr->op == "ret") break;
    }

    if (cfg_leader.empty())
        return false;

    if (instr_ssa == instrs_ssa.end())
        instr_ssa = instrs_ssa.begin();

    /* build cfg and its parent */
    for (int i = 0; i < (int)cfg_leader.size() - 1; ++i) {
        vector<int> to;
        Instr *block_last_instr = &instrs_3addr[(cfg_leader[i + 1] - 1) - 1]; // idx start from 0
        if (block_last_instr->op == "br") {
            to.push_back(block_last_instr->jump);
        } else if (block_last_instr->op == "blbc" || block_last_instr->op == "blbs") {
            to.push_back(cfg_leader[i + 1]);
            to.push_back(block_last_instr->jump);
            sort(to.begin(), to.end());
        } else {
            to.push_back(cfg_leader[i + 1]);
        }
        for (auto &s: to) cfg_rev[s].push_back(cfg_leader[i]);
        cfg.insert(MK(cfg_leader[i], to));
    }
    cfg.insert(MK(cfg_leader[cfg_leader.size() - 1], {}));
    return true;
}

static void dfs_walk(int r, vector<int> &rpo_seq) {
    visited[r] = true;
    for (auto &s: cfg[r]) {
        if (visited.count(s) && visited[s])
            continue;
        dfs_walk(s, rpo_seq);
    }
    rpo_seq.push_back(r);
}

static bool build_idom() {
    if (cfg_leader.empty())
        return false;

    int nentry = cfg_leader[0];
    vector<int> rpo_seq;

    visited.clear();
    dfs_walk(nentry, rpo_seq);
    reverse(rpo_seq.begin(), rpo_seq.end());

    cfg_idom[nentry] = nentry;

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto &b: rpo_seq) {
            if (b == nentry)
                continue;
            int new_idom = cfg_rev[b][0];
            for (int i = 1; i < cfg_rev[b].size(); ++i) {
                if (CONTAIN_KEY(cfg_idom, cfg_rev[b][i])) {
                    int p1 = cfg_rev[b][i];
                    int p2 = new_idom;
                    while (p1 != p2) {
                        while (p1 > p2) p1 = cfg_idom[p1];
                        while (p2 > p1) p2 = cfg_idom[p2];
                    }
                    new_idom = p1;
                }
            }
            if (cfg_idom[b] != new_idom) {
                cfg_idom[b] = new_idom;
                changed = true;
            }
        }
    }

    for (auto &it: cfg_idom) {
        cfg_dom[it.second].push_back(it.first);
    }
    return true;
}

static bool build_df() {
    if (cfg_leader.empty())
        return false;
    for (auto &b: cfg_leader) df[b].clear();
    for (auto &b: cfg_leader) {
        if (cfg_rev[b].size() < 2)
            continue;
        for (auto &p: cfg_rev[b]) {
            int runner = p;
            while (runner != cfg_idom[b]) {
                if (find(df[runner].begin(), df[runner].end(), b) == df[runner].end())
                    df[runner].push_back(b);
                runner = cfg_idom[runner];
            }
        }
    }
    for (auto &_df: df) {
        sort(_df.second.begin(), _df.second.end());
    }
    return true;
}

static void gen_def_use() {
    for (; instr_ssa != instrs_ssa.end(); ++instr_ssa) {
        if (CONTAIN_ITEM(ARITH_OPS, instr_ssa->op) && !(instr_ssa->operand_type[1] == GP || instr_ssa->operand_type[1] == FP)) {
            if (instr_ssa->operand_type[0] == V/* || instr_ssa->operand_type[0] == B*/) {
                use_var[instr_ssa->operand[0]].emplace_back(Use(&*instr_ssa, 0));
            }
            if (instr_ssa->operand_type[1] == V/* || instr_ssa->operand_type[1] == B*/) {
                use_var[instr_ssa->operand[1]].emplace_back(Use(&*instr_ssa, 1));
            }
        } else if ((instr_ssa->op == "param" || instr_ssa->op == "write" || CONTAIN_ITEM(CBR_OPS, instr->op)) &&
                   (instr_ssa->operand_type[0] == V/* || instr_ssa->operand_type[0] == B*/)) {
            use_var[instr_ssa->operand[0]].emplace_back(Use(&*instr_ssa, 0));
        } else if (instr_ssa->op == "move" || instr_ssa->op == "store") {
            if (instr_ssa->operand_type[0] == V)
                use_var[instr_ssa->operand[0]].emplace_back(Use(&*instr_ssa, 0));
            if (instr_ssa->operand_type[1] == V)
                def_var[instr_ssa->operand[1]].emplace_back(Def(&*instr_ssa, 1));
        } else if (instr_ssa->op == "add" && (instr_ssa->operand_type[1] == GP || instr_ssa->operand_type[1] == FP)) {
            continue;
//            string var_name = instr_ssa->operand[0];
//            auto ins_cur = instr_ssa;
//            auto ins_last = instr_ssa;
//            auto ins_next = instr_ssa;
//            ++ins_next;
//            --ins_last;
//            int k = LNR - 1;
//
//            if (ins_last->op == "mul" && ins_last->operand_type[1] == N) {
//                var_name += "[" + ins_last->operand[0] + "]";
//                ins_next = ins_last;
//                ++ins_cur;
//                ++ins_last;
//                if (str2int(ins_next->operand[1]) > VSZ) {
//                    ins_next = ins_cur; ++ins_next;
//                    while (ins_next->op == "mul") {
//                        int off = str2int(ins_next->operand[1]);
//                        if (off < VSZ) break;
//                        var_name += "[" + ins_next->operand[0] + "]";
//                        ++ins_cur; ++ins_cur;
//                        ++ins_last; ++ins_last;
//                        ++ins_next; ++ins_next;
//                        if (off == 8) break;
//                    }
//                }
//            }
//
//            while (ins_next->operand_type[1] == O) {
//                var_name += ".";
//                var_name += ins_next->operand[1];
//                ++ins_next;
//                ++ins_cur;
//                ++ins_last;
//            }
//
//            stringstream ss;
//            ss << "(" << ins_cur->id << ")";
//            string tmp = ss.str();
//
//            auto shadow_ins = instr_ssa;
//            ++shadow_ins;
//            for (; shadow_ins != instrs_ssa.end(); ++shadow_ins) {
//                if ((shadow_ins->op == "load" || shadow_ins->op == "store") && shadow_ins->operand[0] == ss.str()) {
//                    use_var[var_name].emplace_back(Use(&*instr_ssa, 0));
//                    break;
//                }
//                if (shadow_ins->op == "store" && shadow_ins->operand[1] == ss.str()) {
//                    def_var[var_name].emplace_back(Def(&*instr_ssa, 0));
//                    break;
//                }
//            }
        }
    }
    for (auto &it : def_var) {
        sort(it.second.begin(), it.second.end(), [](Def &a, Def &b) -> bool {
            return a.ref->id < b.ref->id;
        });
    }
    for (auto &it : use_var) {
        sort(it.second.begin(), it.second.end(), [](Use &a, Use &b) -> bool {
            return a.ref->id < b.ref->id;
        });
    }
}

static void insert_phi() {
    for (auto &it: def_var) {
        const string &var_name = it.first;
        queue<int> worklist;
        map<int, bool> checklist, done;
        if (!FIND(var_name, '#'))
            continue;
        int offset = str2int(SUBSTR(var_name, var_name.rfind('#') + 1, var_name.size()));
        if (offset <= 0 && it.second.size() < 2)
            continue;
        for (const Def &def: it.second) {
            int b = def.ref->block;
            if (!checklist[b]) {
                checklist[b] = true;
                worklist.push(b);
            }
        }
        while (!worklist.empty()) {
            int b = worklist.front();
            worklist.pop();
            for (int d: df[b]) {
                if (done[d]) continue;

                auto to = find_if(instrs_ssa.begin(), instrs_ssa.end(), [&](const Instr &ins) { return ins.id == d; });
                ASSERT(to != instrs_ssa.end());


                stringstream ss_phi, ss_move;
                ss_phi << "    instr " << instr_count << ": phi";
                for (int i = 0; i < it.second.size() + (offset > 0); ++i)
                    ss_phi << " " << var_name << "$*";
                ss_move << "    instr " << instr_count + 1 << ": move (" << instr_count << ") " << var_name << "$*";
                instr_count += 2;

                to = instrs_ssa.insert(to, Instr(ss_move.str()));
                to->block = d;
                to = instrs_ssa.insert(to, Instr(ss_phi.str()));
                to->block = d;

                done[d] = true;
                if (!checklist[d]) {
                    checklist[d] = true;
                    worklist.push(d);
                }
            }
        }
    }
}

static string get_name() {
    if (var_stack.empty()) {
        var_stack.push(var_count++);
    }
    return to_string(var_stack.top());
}

static string alloc_name(bool *need_free = nullptr) {
    var_stack.push(var_count++);
    if (need_free)
        *need_free = true;
    return get_name();
}

static void free_name() {
    var_stack.pop();
}

static void find_block(list<Instr>::iterator &begin, list<Instr>::iterator &end, int r) {
    /* in block r, instruction start from begin_id to end_id */
    begin = instrs_ssa.begin();
    end = instrs_ssa.end();
    for (auto it = instrs_ssa.begin(); it != instrs_ssa.end(); ++it) {
        if (it->id == r - 1) {
            begin = it;
            ++begin;
        }
    }
    end = begin;
    for (++end; end != instrs_ssa.end(); ++end) {
        if (end->block != r)
            break;
    }
}

static void do_rename(int r, const string &var) {
    if (visited[r])
        return;

    visited[r] = true;

    bool need_free = false;
    list<Instr>::iterator begin, end;

    find_block(begin, end, r);

    /* rewrite uses of names with current version */
    for (auto ins = begin; ins != end; ++ins) {
        if (ins->op == "phi") {
            string phi_var = SUBSTR(ins->operand[0], 0, ins->operand[0].rfind('$'));
            if (phi_var == var) {
                ++ins;
                ins->operand[1] = phi_var + '$' + alloc_name(&need_free);
            }
        }
    }

    /* rewrite uses of global names with current version */
    vector<pair<int, void *>> order;
    for (Use &use : use_var[var]) {
        order.emplace_back(0, (void *)&use);
    }
    for (Def &def : def_var[var]) {
        order.emplace_back(1, (void *)&def);
    }
    sort(order.begin(), order.end(), [](pair<int, void*> &a, pair<int, void*> &b){
        int id_a, id_b;
        if (a.first == 0) {
            id_a = ((Use *) a.second)->ref->id;
        } else {
            id_a = ((Def *) a.second)->ref->id;
        }
        if (b.first == 0) {
            id_b = ((Use *) b.second)->ref->id;
        } else {
            id_b = ((Def *) b.second)->ref->id;
        }
        return id_a < id_b;
    });

    for (auto &it : order) {
        Instr *ins;
        int idx;
        if (it.first == 0) {
            ins = ((Use *)it.second)->ref;
            idx = ((Use *)it.second)->idx;
            if (ins->block != r) continue;
            ins->operand[idx] += "$" + get_name();
        } else {
            ins = ((Use *)it.second)->ref;
            idx = ((Use *)it.second)->idx;
            if (ins->block != r) continue;
            ins->operand[idx] += "$" + alloc_name(&need_free);
        }

    }

    /* fill in phi parameters of successor blocks */
    for (auto &succ: cfg[r]) {
        find_block(begin, end, succ);
        for (auto ins = begin; ins != end; ++ins) {
            if (ins->op == "phi") {
                for (int i = 0; i < ins->operand.size(); ++i) {
                    string phi_var = SUBSTR(ins->operand[i], 0, ins->operand[i].rfind('$'));
                    if (phi_var == var && ins->operand[i].back() == '*') {
                        ins->operand[i] = phi_var + '$' + get_name();
                        break;
                    } else if (phi_var != var) {
                        break;
                    }
                }
            }
        }
    }

    /* recurse on r's children in the dominator tree */
    for (auto &succ: cfg_dom[r]) {
        do_rename(succ, var);
    }

    if (need_free) free_name();
}

static void rename() {
    for (auto &it: def_var) {
        var_count = 0;
        while (!var_stack.empty()) var_stack.pop();
        visited.clear();
        do_rename(cfg_leader[0], it.first);
    }
}

static void rewrite_jump() {
    for (auto it = instrs_ssa.begin(); it != instrs_ssa.end(); ++it) {
        if (CONTAIN_ITEM(CBR_OPS, it->op)) {
            auto target = find_if(instrs_ssa.begin(), instrs_ssa.end(), [&](Instr &ins){return ins.id == it->jump;});
            int b = target->block;
            while(target != instrs_ssa.begin()) {
                if (target->block == b)
                    --target;
                else
                    break;
            }
            ++target;
            it->operand[1] = "[" + to_string(target->id) + "]";
        } else if (CONTAIN_ITEM(UBR_OPS, it->op)) {
            auto target = find_if(instrs_ssa.begin(), instrs_ssa.end(), [&](Instr &ins){return ins.id == it->jump;});
            int b = target->block;
            while(target != instrs_ssa.begin()) {
                if (target->block == b)
                    --target;
                else
                    break;
            }
            ++target;
            it->operand[0] = "[" + to_string(target->id) + "]";
        }
    }
}

static void reorder() {
    map<int, vector<pair<list<Instr>::iterator, int>>> refs;
    for (auto ins = instrs_ssa.begin(); ins != instrs_ssa.end(); ++ins) {
        for (int i = 0; i < ins->operand.size(); ++i) {
            if (ins->operand_type[i] == R || ins->operand_type[i] == I) {
                int ref_id = GET_REG(ins->operand[i]);
                refs[ref_id].push_back(MK(ins, i));
            }
        }
    }

    int recount = 1;
    for (auto ins = instrs_ssa.begin(); ins != instrs_ssa.end(); ++ins) {
        for (auto &ref : refs[ins->id]) {
            auto _ins = ref.first;
            int i = ref.second;
            if (_ins->operand_type[i] == R) {
                _ins->operand[i] = "(" + to_string(recount) + ")";
            } else if (_ins->operand_type[i] == I) {
                _ins->operand[i] = "[" + to_string(recount) + "]";
            }
        }
        ins->id = recount++;
    }
}

void translate_main() {
    for (instr = instrs_3addr.begin(), instr_ssa = instrs_ssa.end(); instr < instrs_3addr.end(); ++instr) {
        if (build_cfg() && build_idom() && build_df()) {
            gen_def_use();
            insert_phi();
            rename();
            rewrite_jump();
            cleanup();
        } else {
            break;
        }
    }
    reorder();
}

int main(int argc, char *argv[]) {
    read_3addr(cin);

    translate_main();

    write_ssa(cout);

    return EXIT_SUCCESS;
}