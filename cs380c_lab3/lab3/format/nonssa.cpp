#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "misc.h"
#include "Instr.h"

using namespace std;

vector<Instr> instrs_ssa;
vector<Instr>::iterator ins_ssa;

vector<int> cfg_leader;

static void read_ssa(istream &in) {
    string one_instr;
    while (getline(in, one_instr)) {
        trim(one_instr);
        if (one_instr.empty())
            break;
        instrs_ssa.emplace_back(one_instr);
    }
}

static void write_3addr(ostream &out) {
    for (ins_ssa = instrs_ssa.begin(); ins_ssa != instrs_ssa.end(); ++ins_ssa) {
        out << *ins_ssa << endl;
    }
}

static void remove_phi() {
    auto start = ins_ssa;
    vector<int> jump_it;
    map<int, int> phi_next;
    for (; ins_ssa < instrs_ssa.end(); ++ins_ssa) {
        if (ins_ssa->op == "enter") {
            cfg_leader.push_back(ins_ssa->id);
        } else if (CONTAIN_ITEM(UBR_OPS, ins_ssa->op) || CONTAIN_ITEM(BR_OPS, ins_ssa->op)) {
            cfg_leader.push_back(ins_ssa->jump);
            cfg_leader.push_back(ins_ssa->id + 1);
            jump_it.push_back((int)(ins_ssa - instrs_ssa.begin()));
        } else if (ins_ssa->op == "ret") {
            break;
        }
    }

    if (cfg_leader.empty())
        return;

    sort(cfg_leader.begin(), cfg_leader.end(), [&](int i, int j) {
        int idx1, idx2;
        for (idx1 = 0; idx1 < instrs_ssa.size(); ++idx1) if (instrs_ssa[idx1].id == i) break;
        for (idx2 = 0; idx2 < instrs_ssa.size(); ++idx2) if (instrs_ssa[idx2].id == j) break;
        return idx1 < idx2;
    });
    cfg_leader.resize(unique(cfg_leader.begin(), cfg_leader.end()) - cfg_leader.begin());

    int next_bb = 0;
    for (auto _ins = start; _ins != instrs_ssa.end(); ++_ins) {
        if (next_bb + 1 < cfg_leader.size() && cfg_leader[next_bb + 1] == _ins->id)
            next_bb++;
        _ins->block = cfg_leader[next_bb];
        if (_ins->op == "ret") break;
    }

    for (auto _ins = start; _ins != instrs_ssa.end(); ++_ins) {
        if (_ins->id == _ins->block && _ins->op == "phi") {
            auto tmp = _ins;
            ++tmp; ++tmp;
            while (tmp->op == "phi") {
                ++tmp;
                ++tmp;
            }
            phi_next[_ins->id] = tmp->id;
        }
        if (_ins->op == "ret")  break;
    }

    for (auto &idx : jump_it) {
        auto _ins = instrs_ssa.begin() + idx;
        if (phi_next.count(_ins->jump)) {
            if (_ins->op == "br") {
                _ins->operand[0] = "[" + to_string(phi_next[_ins->jump]) + "]";
                _ins->jump = phi_next[_ins->jump];
            } else {
                _ins->operand[1] = "[" + to_string(phi_next[_ins->jump]) + "]";
                _ins->jump = phi_next[_ins->jump];
            }
        }
    }

    for (auto _ins = start; _ins != instrs_ssa.end();) {
        if (_ins->op == "phi") {
            _ins = instrs_ssa.erase(_ins);
            _ins = instrs_ssa.erase(_ins);
            continue;
        } else if (_ins->op == "ret") {
            break;
        }
        ++_ins;
    }
}

static void remove_name() {
    for (ins_ssa = instrs_ssa.begin(); ins_ssa < instrs_ssa.end(); ++ins_ssa) {
        for (auto &operand : ins_ssa->operand) {
            if (FIND(operand, '$')) {
                operand = SUBSTR(operand, 0, operand.rfind('$'));
            }
        }
    }
}

static void reorder() {
    map<int, vector<pair<vector<Instr>::iterator, int>>> refs;
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

static void translate_main() {
    for (ins_ssa = instrs_ssa.begin(); ins_ssa < instrs_ssa.end(); ++ins_ssa) {
        remove_phi();
    }
    remove_name();
    reorder();
}

int main() {
    read_ssa(cin);
    translate_main();
    write_3addr(cout);
    return EXIT_SUCCESS;
}