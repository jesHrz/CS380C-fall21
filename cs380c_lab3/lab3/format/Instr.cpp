#include <map>
#include <string>
#include <regex>

#include "Instr.h"
#include "misc.h"

using namespace std;

const vector<string> RET_OPS = {"ret"};
const vector<string> BR_OPS = {"br", "blbc", "blbs"};
const vector<string> PROC_START_OP = {"enter"};
const vector<string> CBR_OPS = {"blbc", "blbs"};  // conditional branch
const vector<string> UBR_OPS = {"br"}; // unconditional branch
const vector<string> LEADER_OPS = {"enter"};
const vector<string> INVALID_LEADER_OPS = {"entrypc", "nop"};
const vector<string> CMP_OPS = {"cmpeq", "cmple", "cmplt"};
const vector<string> CALC_OPS = {"add", "sub", "mul", "div", "mod", "neg"};
const vector<string> ARITH_OPS = {"add", "sub", "mul", "div", "mod", "neg", "cmpeq", "cmple", "cmplt"};
const map<string, string> ARITH_OP_MAP = {
        {"add", "+"},
        {"sub", "-"},
        {"mul", "*"},
        {"div", "/"},
        {"mod", "%"},
        {"cmple", "<="},
        {"cmpeq", "=="},
        {"cmplt", "<"}
};

static const string REGISTER_PATTERN = "\\((\\d+)\\)"; // e.g. (5)
static const string INSTR_PATTERN = "\\[(\\d+)\\]"; // e.g. [5]
static const string NUM_PATTERN = "(\\d+)"; // e.g. 5
static const string BASE_PATTERN = "[_a-zA-Z0-9]+_base#([-0-9]+)";  // e.g. global_array_base#32576
static const string OFFSET_PATTERN = "[_a-zA-Z0-9]+_offset#([-0-9]+)"; // e.g. y_offset#8
static const string VAR_PATTERN = "([_a-zA-Z0-9]+)#[-0-9]+"; // e.g. i#-8
static const map<OPTYPE, string> PATTERNS = {
        {R, REGISTER_PATTERN},
        {I, INSTR_PATTERN},
        {N, NUM_PATTERN},
        {B, BASE_PATTERN},
        {O, OFFSET_PATTERN},
        {V, VAR_PATTERN}
};

Instr::Instr(const string &s) {
    StrList line;

    unsigned long colon_pos = s.find(':');
    unsigned long start_pos = s.find("instr ");
    if (colon_pos == string::npos || start_pos == string::npos)
        ERROR("Invalid format instr:\n\t%s\n", s.c_str());
    split(s.substr(colon_pos + 2), line, ' ');
    this->id = str2int(SUBSTR(s, start_pos + 6, colon_pos));
    this->op = line[0];
    this->operand.insert(this->operand.end(), line.begin() + 1, line.end());
    while(this->operand.size() < 2) this->operand.emplace_back("");
    this->operand_type.resize(this->operand.size());
    for (int i = 0; i < this->operand.size(); ++i) {
        this->operand_type[i] = _parse_argtype(this->operand[i]);
    }

    if (CONTAIN_ITEM(UBR_OPS, this->op)) {
        this->jump = GET_REG(this->operand[0]);
    } else if (CONTAIN_ITEM(CBR_OPS, this->op)) {
        this->jump = GET_REG(this->operand[1]);
    } else {
        this->jump = 0;
    }

}

Instr::Instr() {
    this->id = 0;
    this->op = "";
    this->operand.emplace_back("");
    this->operand.emplace_back("");
    this->operand_type.emplace_back(Err);
    this->operand_type.emplace_back(Err);
    this->jump = 0;
}

OPTYPE Instr::_parse_argtype(const string &arg) {
    if (arg.empty())
        return Err;
    if (arg == "GP")
        return GP;
    if (arg == "FP")
        return FP;
    for (auto &it : PATTERNS) {
        regex e(it.second);
        if (regex_match(arg, e))
            return it.first;
    }
    return Err;
}

ostream &operator<<(ostream &out, const Instr &ins) {
    out << "    instr " << ins.id << ": " << ins.op;
    for (auto &o : ins.operand)
        if (ins.op != "phi" || !FIND(o, '*'))
            out << " " << o;
    return out;
}