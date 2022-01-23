#include <iostream>
#include <sstream>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <fstream>

#include "misc.h"


#define LOG(args...) do { std::cout << #args<< " -> "; err(args); } while (0)

void err() { std::cout << std::endl; }

template<typename T, typename... Args>
void err(T a, Args... args) {
    std::cout << a << ' ';
    err(args...);
}

#define ASSERT(eval) \
    do {             \
        if (!(eval)) \
            TRANSLATOR_ERR("Assertion failed:\n\t" #eval "\nat %s, %s, line %d.\nCurrent: %s\n", __FILE__, __FUNCTION__, __LINE__, instr->c_str());\
    } while(0)
#define TRANSLATOR_ERR(fmt, args...) \
    do {                             \
        fprintf(stderr, fmt, ##args);  \
        exit(EXIT_FAILED);           \
    } while(0)
#define GET(m, ik) \
    ({            \
        ASSERT(m.count(ik));\
        m[ik];     \
    })
#define FIND_VALUE(m, v) (std::find_if(m.begin(), m.end(), [v](const auto& i){return i.second==v;}))
#define CONTAIN_VALUE(m, v) (FIND_VALUE(m, v) != m.end())
#define CONTAIN_KEY(m, k) (m.count(k) != 0)

#define GET_REG(s) (str2int((s).substr(1, (s).length() - 1)))
#define LNR ((int)(instr - instrs_3addr.begin()) + 1)

#define FIND(s, p) ((s).find((p)) != std::string::npos)
#define SUBSTR(s, l, r) (s).substr((l), ((r) - (l)))

#define OFFSET_LABEL "_offset"
#define OFFSET_LABEL_SZ 7
#define BASE_LABEL   "_base"
#define BASE_LABEL_SZ 5

#define INDENT_SPACE "  "

StrList instrs_3addr, program;
StrList::iterator instr;
StrMap globals, functions;
std::map<std::string, bool> garrays; // treat all structs as array
std::map<int, int> array_address_complete;

struct OneState {
    std::string prefix;
    std::string body;
    char suffix;
    explicit OneState(const std::string &b){
        prefix = "";
        body = b;
        suffix = 0;
    };
    OneState(const std::string &_pre, const std::string &_body, char _suf): prefix(_pre), body(_body), suffix(_suf) {};
    OneState(){
        prefix = "";
        body = "";
        suffix = 0;
    };
    std::ostream & operator << (std::ostream &out) const {
       out << body;
       return out;
    }
};
typedef std::map<int, OneState> Statement;

int FP = 0;
int GP = 32768; // downgrade to zero


/*
 * print final translated C program
 */
static void output_prog(std::ostream &out) {
    for (const std::string &statement: program) {
        out << statement << "\n";
    }
}

static std::string output_vars(StrMap &variables, std::map<std::string, bool> &arrays, int base, bool indent = true) {
    int pre = base, size;
    std::stringstream desc;
    for (auto &var : variables) {
        size = abs(var.first - pre) / VSZ;
        if (indent) desc << INDENT_SPACE;
        desc << "long " << var.second;
        if (size > 1 && CONTAIN_KEY(arrays, var.second)) {
            desc << '[' << size << ']';
        }
        desc << ";\n";
    }
    return desc.str();
}

static std::string output_func(std::string &func_name, int nb_param, StrMap &variables, StrMap &params, std::map<std::string, bool> &arrays, Statement &states) {
    std::stringstream ss;
    ss << "void " << func_name << "(";

    // params is descendingly ordered in stack
    for (int i = nb_param, off; i > 0; i--) {
        off = (i + 1) << 3; // (i+1)*8 LINK
        ss << "long ";
        if (CONTAIN_KEY(params, off)) {
            // this param appears
            ss << params[off];
        } else {
            ss << "_param_" << i;
        }
        if (i != 1) ss << ',';
    }
    ss << "){\n";

    // local variables
    ss << output_vars(variables, arrays, FP);

    // function states
    int indent_count = 1;
    for (auto &s : states) {
        OneState &p = s.second;
        for (char c: p.prefix) {
            ASSERT(c == '}');
            indent_count--;
            for (int j = 0; j < indent_count; ++j) ss << INDENT_SPACE;
            ss << "}\n";
        }

        if (p.body.empty())
            continue;

        for (int j = 0; j < indent_count; ++j)  ss << INDENT_SPACE;
        ss << p.body;

        if (p.suffix) {
            ASSERT(p.suffix == '{');
            indent_count++;
            ss << "{\n";
        } else {
            ss << ";\n";
        }
    }
    ss << "}\n";
    return ss.str();
}

static void read_3addr(std::istream &in) {
    std::string one_instr;
    while (std::getline(in, one_instr)) {
        trim(one_instr);
        if (one_instr.empty())
            break;
        instrs_3addr.emplace_back(one_instr);
    }
}

/*
 * parse instr like `Instr <num>: op arg1 arg2 ...`
 * save `op arg1 arg2` into line in order
 */
static void parse_instr(StrList &line) {
    const char delim = ' ';
    unsigned long split_pos = instr->find(": ");
    if (split_pos == std::string::npos)
        TRANSLATOR_ERR("Invalid format at line %d:\n\t%s\n", (int) (instr - instrs_3addr.begin()), instr->c_str());
    split(instr->substr(split_pos + 2), line, ' ');
}

static std::string parse_instr_arg(std::string arg, StrMap &params, StrMap &locals, std::map<std::string, bool> &arrays, Statement &states) {
    if (arg[0] == '(') {
        if (arg[arg.length() - 1] == ')') {
            int ln = GET_REG(arg);
            std::string s = GET(states, ln).body;
            if (states[ln].prefix.empty() && states[ln].suffix == 0)
                states.erase(ln);
            else
                states[ln].body.clear();
            return s;
        }
        TRANSLATOR_ERR("Invalid argument of %s\n", arg.c_str());
    }

    StrList v;
    split(arg, v, '#');

    if (v.size() > 1 && !FIND(v[0], OFFSET_LABEL)) {
        int offset = str2int(v[1]);
        if (offset >= 0) {
            params.insert(MK(offset, v[0]));
        } else if (FIND(v[0], BASE_LABEL)) {
            std::string array_name = SUBSTR(v[0], 0, v[0].length() - BASE_LABEL_SZ);
            arrays.insert(MK(array_name, true));
            locals.insert(MK(offset, array_name));
        } else {
            locals.insert(MK(offset, v[0]));
        }
    }

    return v[0];
}

/*
 * parse each instr until meet `ret`
 * and then generate corresponding C function code
 */
static std::string parse_function() {
    StrList line, call_params; // function call
    StrMap params, locals;
    Statement states;
    std::map<std::string, bool> larrays;
    std::string func_name;
    bool is_main_func;
    int nb_params_self;

    is_main_func = false;
    for (; instr != instrs_3addr.end(); ++instr) {
        parse_instr(line);
        if (line[0] == "enter") {
            functions.insert(MK(LNR, is_main_func ? "main" : std::string("function_") + std::to_string(LNR)));
            func_name = functions[LNR];
        } else if (line[0] == "ret") {
            nb_params_self = str2int(line[1]) / VSZ;
            break;
        } else if (line[0] == "entrypc") {
            is_main_func = true;
        } else if (line[0] == "param") {
            call_params.emplace_back(parse_instr_arg(line[1], params, locals, larrays, states));
        } else if (line[0] == "call") {
            std::stringstream ss;
            ss << GET(functions, str2int(SUBSTR(line[1], 1, line[1].length() - 1))) << "(";
            for (int i = 0; i < call_params.size(); ++i) {
                ss << call_params[i];
                if (i != call_params.size() - 1)
                    ss << ",";
            }
            ss << ")";
            call_params.clear();
            states[LNR].body = ss.str();
        } else if (line[0] == "read") {
            states[LNR].body = "$read$";
        } else if (line[0] == "write") {
            std::stringstream ss;
            ss << "WriteLong(" << parse_instr_arg(line[1], params, locals, larrays, states) << ")";
            states[LNR].body = ss.str();
        } else if (line[0] == "wrl") {
            states[LNR].body = "WriteLine()";
        } else if (line[0] == "load") {
            if (FIND(line[1], '(')) {
                int ln = GET_REG(line[1]);
                states.insert(MK(LNR, GET(states, ln)));
                states.erase(ln);
            } else {
                states[LNR].body = line[1];
            }
        } else if (line[0] == "store") {
            std::string lval = parse_instr_arg(line[2], params, locals, larrays, states);
            std::string rval = parse_instr_arg(line[1], params, locals, larrays, states);

            std::stringstream ss;
            ss << lval << '=' << rval;
            states[LNR].body = ss.str();
        } else if (line[0] == "move") {
            std::string lval = parse_instr_arg(line[2], params, locals, larrays, states);
            std::string rval = parse_instr_arg(line[1], params, locals, larrays, states);

            std::stringstream ss;
            if (FIND(rval, "$read$")) {
                ss << "ReadLong(" << lval << ')';
            } else {
                ss << lval << "=" << rval;
            }
            states[LNR].body = ss.str();
        } else if (line[0] == "add") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            if (CONTAIN_VALUE(globals, arg1) && FIND(arg2, '*')) {
                /*  Global Naive Array
                 *  instr 20: mul 0 40
                 *  instr 21: add b_base#32608 GP
                 *  instr 22: add (21) (20) <--
                 *  arg1: xxx
                 *  arg2: (exp*SIZE)
                 */
                std::string array_name = arg1;

                unsigned long lst_star_pos = arg2.rfind('*');
                int array_ele_size = str2int(SUBSTR(arg2, lst_star_pos + 1, arg2.length() - 1)) / VSZ;
                std::string idx = SUBSTR(arg2, 1, lst_star_pos);

                ASSERT(array_ele_size > 0);

                array_address_complete[LNR] = array_ele_size;

                garrays.insert(MK(array_name, true));
                larrays.erase(array_name);

                std::stringstream ss;
                ss << array_name << "[(" << idx << '*' << array_ele_size << ")]";
                states[LNR].body = ss.str();
            } else if (CONTAIN_KEY(larrays, arg1) && FIND(arg2, '*')) {
                /* Local Naive Array
                 * instr 76: mul 3 8
                 * instr 77: add d_base#-40 FP
                 * instr 78: add (77) (76) <--
                 * arg1: xxx
                 * arg2: (exp*SIZE)
                 */
                std::string array_name = arg1;

                unsigned long lst_star_pos = arg2.rfind('*');
                int array_ele_size = str2int(SUBSTR(arg2, lst_star_pos + 1, arg2.length() - 1)) / VSZ;
                std::string idx = SUBSTR(arg2, 1, lst_star_pos);

                ASSERT(array_ele_size > 0);

                int target_line = str2int(SUBSTR(line[1], 1, line[1].length() - 1));
                ASSERT(target_line != 0);
                array_address_complete[LNR] = std::min(array_address_complete[target_line], array_ele_size);

                std::stringstream ss;
                ss << array_name << "[(" << idx << '*' << array_ele_size << ")]";
                states[LNR].body = ss.str();
            } else if (FIND(arg1, '[') && arg1.rfind(']') == arg1.length() - 1 &&
                    array_address_complete[str2int(SUBSTR(line[1], 1, line[1].length() - 1))] > 1 &&
                    FIND(arg2, '*') && str2int(SUBSTR(arg2, arg2.rfind('*') + 1, arg2.length() - 1)) != 0) {
                /* Multidimensional array
                 * instr 24: mul 3 24
                 * instr 25: add e_base#-160 FP
                 * instr 26: add (25) (24)
                 * instr 27: mul 1 8
                 * instr 28: add (26) (27) <--
                 * arg1: array[orig_idx]
                 * arg2: (exp*SIZE)
                 */
                unsigned long left_bracket_pos = arg1.find('[');
                unsigned long right_bracket_pos = arg1.rfind(']');
                std::string array_name = SUBSTR(arg1, 0, left_bracket_pos);
                std::string orig_idx = SUBSTR(arg1, left_bracket_pos + 1, right_bracket_pos);

                std::string idx_str = SUBSTR(arg2, 1, arg2.length() - 1);
                unsigned long lst_star_pos = arg2.rfind('*');
                int array_ele_size = str2int(SUBSTR(arg2, lst_star_pos + 1, arg2.length() - 1)) / VSZ;
                std::string idx = SUBSTR(arg2, 1, lst_star_pos);

                ASSERT(array_ele_size > 0);
                ASSERT(CONTAIN_KEY(garrays, array_name) || CONTAIN_KEY(larrays, array_name));

                int target_line = str2int(SUBSTR(line[1], 1, line[1].length() - 1));
                ASSERT(target_line != 0);
                array_address_complete[LNR] = std::min(array_address_complete[target_line], array_ele_size);

                std::stringstream ss;
                ss << array_name << "[(" << orig_idx << "+(" << idx << '*' << array_ele_size << "))]";
                states[LNR].body = ss.str();
            } else if (CONTAIN_VALUE(globals, arg1) && FIND(arg2, OFFSET_LABEL)) {
                /*  Global Struct
                 * 	instr 11: add p_base#32560 GP
                 * 	instr 12: add (11) x_offset#0 <--
                 * 	arg1: xxx
                 * 	arg2: y_offset#<off>
                 */
                StrList v;
                split(line[2], v, '#');
                ASSERT(v.size() == 2);
                int offset = str2int(v[1]) / VSZ;

                int target_line = str2int(SUBSTR(line[1], 1, line[1].length() - 1));
                ASSERT(target_line != 0);
                array_address_complete[LNR] = std::min(array_address_complete[target_line], 1);

                garrays.insert(MK(arg1, true));
                larrays.erase(arg1);

                std::stringstream ss;
                ss << arg1 << "[(" << offset << ")]";
                states[LNR].body = ss.str();
            } else if (CONTAIN_KEY(larrays, arg1) && FIND(arg2, OFFSET_LABEL)) {
                /*  Local Struct
                 *  instr 14: add c_base#-80 FP
                 *  instr 15: add (14) x_offset#0 <--
                 */
                StrList v;
                split(line[2], v, '#');
                ASSERT(v.size() == 2);
                int offset = str2int(v[1]) / VSZ;

                ASSERT(CONTAIN_KEY(larrays, arg1));
                int target_line = str2int(SUBSTR(line[1], 1, line[1].length() - 1));
                ASSERT(target_line != 0);
                array_address_complete[LNR] = std::min(array_address_complete[target_line], 1);

                std::stringstream ss;
                ss << arg1 << "[(" << offset << ")]";
                states[LNR].body = ss.str();
            } else if (FIND(arg1, '[') && arg1.rfind(']') == arg1.length() - 1 && FIND(arg2, OFFSET_LABEL)) {
                /* Nested Struct / Array of Structs
                 * instr 4: add a_base#32576 GP
                 * instr 5: add (4) z_offset#16
                 * instr 6: add (5) r_offset#8 <---
                 * arg1: xxx[yyy]
                 * arg2: zzz_offset#<off>
                 */
                StrList v;
                split(line[2], v, '#');
                ASSERT(v.size() == 2);
                int offset = str2int(v[1]) / VSZ;

                unsigned long first_left_bracket_pos = arg1.find('[');
                unsigned long lst_right_bracket_pos = arg1.rfind(']');

                std::string idx_str = SUBSTR(arg1, first_left_bracket_pos + 1, lst_right_bracket_pos);
                std::string array_name = SUBSTR(arg1, 0, first_left_bracket_pos);

                ASSERT(CONTAIN_KEY(garrays, array_name) || CONTAIN_KEY(larrays, array_name));
                int target_line = str2int(SUBSTR(line[1], 1, line[1].length() - 1));
                ASSERT(target_line != 0);
                array_address_complete[LNR] = std::min(array_address_complete[target_line], 1);

                std::stringstream ss;
                ss << array_name << "[(" << idx_str << '+' << offset <<")]";
                states[LNR].body = ss.str();
            } else if (arg2 == "GP") { // Define Global Symbol
                StrList v;
                split(line[1], v, '#');
                ASSERT(v.size() == 2);
                int base = str2int(v[1]);

                ASSERT(FIND(v[0], BASE_LABEL));
                std::string symbol = SUBSTR(v[0], 0, v[0].length() - BASE_LABEL_SZ);

                globals.insert(MK(base, symbol));
                locals.erase(base);
                states[LNR].body = symbol;
            } else if (arg2 == "FP") { // Define Local Symbol
                StrList v;
                split(line[1], v, '#');
                ASSERT(v.size() == 2);

                ASSERT(FIND(v[0], BASE_LABEL));
                std::string symbol = SUBSTR(v[0], 0, v[0].length() - BASE_LABEL_SZ);

                array_address_complete[LNR] = INT_MAX;
                states[LNR].body = symbol;
            } else {
                std::stringstream ss;
                ss << '(' << arg1 << '+' << arg2 << ')';
                states[LNR].body = ss.str();
            }
        } else if (line[0] == "sub") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            std::stringstream ss;
            ss << '(' << arg1 << '-' << arg2 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "mul") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            std::stringstream ss;
            ss << '(' << arg1 << '*' << arg2 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "div") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            std::stringstream ss;
            ss << '(' << arg1 << '/' << arg2 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "mod") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            std::stringstream ss;
            ss << '(' << arg1 << '%' << arg2 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "neg") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::stringstream ss;
            ss << "(-" << arg1 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "cmpeq") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            std::stringstream ss;
            ss << '(' << arg1 << "==" << arg2 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "cmple") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            std::stringstream ss;
            ss << '(' << arg1 << "<=" << arg2 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "cmplt") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string arg2 = parse_instr_arg(line[2], params, locals, larrays, states);
            std::stringstream ss;
            ss << '(' << arg1 << '<' << arg2 << ')';
            states[LNR].body = ss.str();
        } else if (line[0] == "br") {
            std::string label_str = SUBSTR(line[1], 1, line[1].length() - 1);
            int label_line = str2int(label_str);

            if (label_line > LNR) { // else
                states[LNR + 1].prefix.pop_back();
                states.insert(MK(LNR, OneState("}", "else", '{')));
                states[label_line].prefix.push_back('}');
            } else { // while
                while(!CONTAIN_KEY(states, label_line) || (CONTAIN_KEY(states, label_line) && states[label_line].body.empty()))
                    label_line++;
                std::string orig_s = states[label_line].body;
                std::string exp = SUBSTR(orig_s, 3, orig_s.length() - 1);
                std::stringstream ss;
                ss << "while(" << exp <<")";
                states[label_line].body = ss.str();
            }
        } else if (line[0] == "blbc") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string label_str = SUBSTR(line[2], 1, line[2].length() - 1);
            int label_line = str2int(label_str);
            std::stringstream ss;
            ss << "if(" << arg1 << "!=0)";
            states[LNR].body = ss.str();
            states[LNR].suffix = '{';
            states[label_line].prefix.push_back('}');
        } else if (line[0] == "blbs") {
            std::string arg1 = parse_instr_arg(line[1], params, locals, larrays, states);
            std::string label_str = SUBSTR(line[2], 1, line[2].length() - 1);
            int label_line = str2int(label_str);
            std::stringstream ss;
            ss << "if(" << arg1 << "==0)";
            states[LNR].body = ss.str();
            states[LNR].suffix = '{';
            states[label_line].prefix.push_back('}');
        } else if (line[0] == "nop") {
            continue;
        } else {
            TRANSLATOR_ERR("Unknown 3addr code: %s\n", instr->c_str());
        }
    }

    return output_func(func_name, nb_params_self, locals, params, larrays, states);
}

void translate() {
    std::stringstream body;
    for (instr = instrs_3addr.begin(); instr < instrs_3addr.end(); ++instr) {
        if (!FIND(*instr, "nop"))
            body << parse_function();
    }
    program.emplace_back("#include <stdio.h>");
    program.emplace_back("#define WriteLong(x) printf(\" %lld\", (long)x);");
    program.emplace_back(R"(#define WriteLine(x) printf("\n");)");
    program.emplace_back("#define ReadLong(x)  if (fscanf(stdin, \"%lld\", &x) != 1) x = 0;");
    program.emplace_back("#define long long long");
    program.push_back(output_vars(globals, garrays, GP, false));
    program.push_back(body.str());
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        std::ifstream ifile(argv[1]);
        if (ifile) {
            read_3addr(ifile);
            ifile.close();
        } else {
            TRANSLATOR_ERR("Cannot open file: %s\n", argv[1]);
        }
    } else {
        read_3addr(std::cin);
    }

    translate();

    if (argc > 2) {
        std::ofstream ofile(argv[2]);
        if (ofile) {
            output_prog(ofile);
        } else {
            output_prog(std::cout);
        }
    } else {
        output_prog(std::cout);
    }
    return EXIT_SUCCESS;
}