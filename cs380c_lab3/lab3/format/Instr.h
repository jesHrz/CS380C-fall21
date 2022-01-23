#ifndef CS380C_LAB3_INSTR_H
#define CS380C_LAB3_INSTR_H

#include <string>

using namespace std;

enum OPTYPE {
    Err, GP, FP,
    R, // ()
    I, // []
    N, // 5
    B, // xx_base#<offset>
    O, // xx_offset#<offset>
    V, // xx#<offset>
};

extern const vector<string> RET_OPS;
extern const vector<string> BR_OPS;
extern const vector<string> PROC_START_OP;
extern const vector<string> CBR_OPS;  // conditional branch
extern const vector<string> UBR_OPS; // unconditional branch
extern const vector<string> LEADER_OPS;
extern const vector<string> INVALID_LEADER_OPS;
extern const vector<string> CMP_OPS;
extern const vector<string> CALC_OPS;
extern const vector<string> ARITH_OPS;
extern const map<string, string> ARITH_OP_MAP;

class Instr {
public:
    int id;
    string op;
    vector<string> operand;
    vector<OPTYPE> operand_type;
    int jump, block;

    Instr();

    explicit Instr(const string &s);

    friend ostream &operator<<(ostream &out, const Instr &ins);

private:
    static OPTYPE _parse_argtype(const string &arg);
};

class Def {
public:
    Instr *ref;
    int idx;

    Def() {};
    Def(Instr *ins, int _i) : ref(ins), idx(_i) {};
};

class Use {
public:
    Instr *ref;
    int idx;

    Use() {}
    Use(Instr *ins, int _i) : ref(ins), idx(_i) {};
};


#endif //CS380C_LAB3_INSTR_H
