#include <functional>
#include <iostream>
#include <list>
#include <tuple>
#include <fstream>

#include <Surelog/surelog.h>

// UHDM
#include <uhdm/ElaboratorListener.h>
#include <uhdm/uhdm.h>
#include <uhdm/VpiListener.h>

// functions declarations
std::string visitbit_sel(vpiHandle);
std::string visithier_path(vpiHandle);
std::string visitindexed_part_sel(vpiHandle);
std::string visitpart_sel(vpiHandle);
std::list <std::string> visitCond(vpiHandle);
std::list <std::string> visitLeaf(vpiHandle);
std::tuple <bool, std::list <std::string>> visitOperation(vpiHandle);
void findTernaryInOperation(vpiHandle);
void visitAssignment(vpiHandle);
void visitBlocks(vpiHandle);
void visitTernary(vpiHandle);
void visitTopModules(vpiHandle);

int evalOperation(vpiHandle);
int evalLeaf(vpiHandle);

// global variables
bool saveVariables = false; 
bool saveSubex = false;
bool expand = true;

// struct defines
// keeps track current conditional block's condition expr for iterative inserts
struct currentCond_s {
  bool v;
  std::string cond;
} currentCond = {false, ""};

// discovered variables
struct vars {
  int width[4]; //malloc will be slower, expensive; supports upto 4 dimensional arrays
  int dims; // for multi dimension arrays
  std::string name;
  std::string type; //reg/wire
};

// global data structures
std::list <vars> nets, netsCurrent; // for storing nets discovered
std::list <std::string> all, ternaries, cases, ifs; // for storing specific control expressions (see definition in main README.md)
std::list <int> nAll, nTernaries, nCases, nIfs, nSubexpressions; // numbers for quick print debug
std::map <std::string, int> paramsAll, params; // for params, needed for supplanting in expressions expansions

// ancillary functions
// prints out discovered control expressions to file or stdout
void print_list(std::list<std::string> &list, bool f = false, std::string fileName = "", bool std = false) {
    std::ofstream file;
    file.open(fileName, std::ios_base::app);
    for (auto const &i: list) {
      if(std)
        std::cout << i << std::endl;
      if(f)
        file << i << std::endl;
    }
    file.close();
    return;
}

// visitor functions for different node types

std::string visitref_obj(vpiHandle h) {
  std::string out = "";
  if(vpiHandle actual = vpi_handle(vpiActual, h)) {
    std::cout << "Actual type of ref_obj: " << 
      UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)actual)->type) << std::endl;
    switch(((const uhdm_handle *)actual)->type) {
      case UHDM::uhdmparameter : 
        out = (visitLeaf(actual)).front();
        break;
      case UHDM::uhdmconstant:
      case UHDM::uhdmenum_const :
      default :
        std::cout << "Default actual object\n";
        if (const char* s = vpi_get_str(vpiFullName, actual))
          out += s;
        else if(const char *s = vpi_get_str(vpiName, actual))
          out += s;
        else out += "UNKNOWN";
        std::cout << "(Full)Name: " << out << std::endl;
        break;
    }
  } else {
    std::cout << "Walking not actual reference object; type: " << 
      UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)h)->type) << std::endl;
    if (const char* s = vpi_get_str(vpiFullName, h)) {
      std::cout << "FullName available\n";
      out += s;
    } else if(const char *s = vpi_get_str(vpiName, h)) {
      std::cout << "FullName unavailable\n";
      out += s;
    } else std::cerr << "Neither FullName, nor Name available\n";

    if(const char *s = vpi_get_str(vpiDecompile, h)) {
      //Produces string literals like 16'h4fc0
      std::cout << "Decompilation available: " << s << std::endl;
      out += s;
    } else {
      s_vpi_value value;
      vpi_get_value(h, &value);
      if (value.format)
        out += std::to_string(value.value.integer);
      else std::cout << "Decompilation unavailable\n";
    }
  }
  return out;
}

std::string visitbit_sel(vpiHandle h) {
  std::string out = "";
  std::cout << "Walking bit select\n";
  if(const char *s = vpi_get_str(vpiFullName, h)) {
    out += s;
    std::cout << "FullName at bit_sel: " << s << std::endl;
  } else {
    std::cout << "Couldn't find FullName at bit_sel!\n";
    vpiHandle par = vpi_handle(vpiParent, h);
    if(!par) {
      std::cerr << "Couldn't find parent of bit_sel!\n";
      out += "";
    } else out += visitref_obj(par);
  }
  std::cout << "Parent: " << out << std::endl;
  out += "[";
  vpiHandle ind = vpi_handle(vpiIndex, h);
  if(ind) {
    std::list <std::string> current = visitLeaf(ind);
    out += current.front();
  }
  else std::cout << "Index not resolved\n";
  out += "]";
  std::cout << "Parent+Index: " << out << std::endl;
  return out;
}

std::string visitindexed_part_sel(vpiHandle h) {
  std::string out = "";
  std::cout << "Walking indexed part select\n";
  vpiHandle par = vpi_handle(vpiParent, h);
  if(!par) std::cerr << "Couldn't find parent\n";
  else out += visitref_obj(par);
  out += "[";
  if(vpiHandle b = vpi_handle(vpiBaseExpr, h)) {
    std::cout << "Base expression found\n";
    std::list <std::string> current = visitLeaf(b);
    out += current.front();
    vpi_release_handle(b);
  }
  out += ":";
  if(vpiHandle w = vpi_handle(vpiWidthExpr, h)) {
    std::cout << "Width expression found\n";
    std::list <std::string> current = visitLeaf(w);
    out += current.front();
    vpi_release_handle(w);
  }
  out += "]";
  return out;
}

std::string visitpart_sel(vpiHandle h) {
  std::string out = "";
  std::cout << "Walking part select\n";
  vpiHandle par = vpi_handle(vpiParent, h);
  if(!par) std::cout << "Couldn't find parent\n";
  else out += visitref_obj(par);
  out += "[";
  vpiHandle lrh = vpi_handle(vpiLeftRange, h);
  if(lrh) {
    std::list <std::string> current = visitLeaf(lrh);
    out += current.front();
  }
  else std::cerr << "Left range not found; type: " <<
    UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)(((const uhdm_handle *)lrh)->type)) << std::endl;
  out += ":";
  vpiHandle rrh = vpi_handle(vpiRightRange, h);
  if(rrh) {
    std::list <std::string> current = visitLeaf(rrh);
    out += current.front();
  }
  else std::cerr << "Right range not found; type: " <<
    UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)(((const uhdm_handle *)rrh)->type)) << std::endl;
  out += "]";
  vpi_release_handle(rrh);
  vpi_release_handle(lrh);
  return out;
}

std::list <std::string> visitLeaf(vpiHandle h) {
  std::list <std::string> out;
  switch(((const uhdm_handle *)h)->type) {
    case UHDM::uhdmoperation : {
      std::cout << "Operation at leaf\n";
      bool k;
      std::tie(k, out) = visitOperation(h);
      break;
    }
    case UHDM::uhdmconstant : {
      if(!saveVariables) {
        std::cout << "Constant at leaf; walking\n";
        std::string tmp = vpi_get_str(vpiDecompile, h);
        out.push_back(tmp);
        std::cout << "Constant: " << tmp << std::endl;
      } else {
        std::cout << "Ignoring constant at leaf\n";
        out.push_back("");
      }
      break;
    }
    case UHDM::uhdmparameter : {
      std::cout << "Parameter at leaf\n";
      std::map <std::string, int>::iterator it;
      std::string name = "";
      if(const char *s = vpi_get_str(vpiName, h)) {
        std::cout << "Finding param " << s << std::endl;
        name = s;
      }
      it = params.find(name);
      if(it == params.end()) {
        std::cout << "UNKNOWN_PARAM\n";
        out.push_back("0"); //TODO can do better
      } else std::cout << "Found param value: " << it->second << std::endl;
      out.push_back(std::to_string(it->second));
      break;
    }
    case UHDM::uhdmhier_path : { 
      std::string tmp = visithier_path(h);
      std::cout << "Struct at leaf: " << tmp << std::endl;
      out.push_back(tmp);
      break;
    }
    case UHDM::uhdmref_obj : {
      std::cout << "Ref object at leaf\n";
      std::string tmp = visitref_obj(h);
      out.push_back(tmp);
      break;
    }
    case UHDM::uhdmbit_select : {
      std::cout << "Bit select at leaf\n";
      std::string tmp = visitbit_sel(h);
      out.push_back(tmp);
      break;
    }
    case UHDM::uhdmpart_select : {
      std::cout << "Part select at leaf\n";
      std::string tmp = visitpart_sel(h);
      out.push_back(tmp);
      break;
    }
    case UHDM::uhdmindexed_part_select : {
      std::cout << "Indexed part select at leaf\n";
      std::string tmp = visitindexed_part_sel(h);
      out.push_back(tmp);
      break;
    }
    case UHDM::uhdmint_typespec : {
      std::cout << "Typespec at leaf\n";
      s_vpi_value value;
      vpi_get_value(h, &value);
      if (value.format)
        out.push_back(std::to_string(value.value.integer));
      else {
        if(const char *s = vpi_get_str(vpiFullName, h))
          out.push_back(s);
        else if (const char* s = vpi_get_str(vpiName, h))
          out.push_back(s);
        else out.push_back("UNKNOWN int_typespec");
      }
      break;
    }
    default: 
      std::cout << "UNKNOWN node at leaf; type: " << 
        UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)h)->type) << std::endl;
      out.push_back("UNKNOWN_NODE");
      break;
  }
  return out;
}

std::string visithier_path(vpiHandle soph) {
  std::string out = "";
  std::cout << "Walking hierarchical path\n";
  vpiHandle it = vpi_iterate(vpiActual, soph);
  if(it) {
    bool first = true;
    while(vpiHandle itx = vpi_scan(it)) {
      bool bitsel = ((const uhdm_handle *)itx)->type == UHDM::uhdmbit_select;
      std::cout << "Found ref object; type: " << 
        UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)itx)->type) << std::endl;
      if(first) {
        std::cout << "Walking struct; ignoring\n";
        std::list <std::string> tmp = visitLeaf(itx);
        out += tmp.front();
        std::cout << out << std::endl;
      }
      else {
        out += ".";
        std::cout << "Walking member hierarchy\n";
        if(bitsel) {
          std::cout << "Member is bit-select; handling locally\n";
          vpiHandle p = vpi_handle(vpiParent, itx);
          out = visitref_obj(p);
        } else {
          std::cout << "Member not bit-select\n";
          std::list <std::string> tmp = visitLeaf(itx);
          out += tmp.front();
        }
        std::cout << "Extracted at this point: " << out << std::endl;
      }
      first = false;
    }
  } else std::cerr << "Couldn't iterate through objs\n";
  return out;
}

int search_width(vpiHandle h) {
  switch(((const uhdm_handle *)h)->type) {
    case UHDM::uhdmbit_select: return 1;
    case UHDM::uhdmpart_select: {
      int left=0, right=0;
      vpiHandle lrh = vpi_handle(vpiLeftRange, h);
      if(lrh) {
        left = evalLeaf(lrh);
      } else std::cerr << "Left range UNKNOWN; type: " <<
        UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)(((const uhdm_handle *)lrh)->type)) << std::endl;
      vpiHandle rrh = vpi_handle(vpiRightRange, h);
      if(rrh) {
        right = evalLeaf(rrh);
      } else std::cerr << "Right range UNKNOWN; type: " <<
        UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)(((const uhdm_handle *)rrh)->type)) << std::endl;
      vpi_release_handle(rrh);
      vpi_release_handle(lrh);
      std::cout << "Operand width: " << std::to_string(right-left);
      return right - left;
    }
    case UHDM::uhdmhier_path:
    case UHDM::uhdmref_obj: {
      std::string name = visitref_obj(h);
      std::cout << "Finding width of " << name << std::endl;
      auto match = std::find_if(netsCurrent.cbegin(), netsCurrent.cend(),
        [&] (const vars& s) {
          return s.name == name;
        });
      if(match != netsCurrent.cend()) {
        std::cout << "Operand width: " << *(match->width) << std::endl;
        return *(match->width);
      } else {
        std::cerr << "Couldn't find the width of: " << name << std::endl;
        return -1;
      }
    }
    case UHDM::uhdmconstant:
    case UHDM::uhdmparameter:
      return -1;
    default: 
      std::cout << "Operand width: UNKNOWN\n";
      std::cout << "Operand type: " <<  UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)(((const uhdm_handle *)h)->type)) << std::endl;
      return -1;
  }
  return -1;
}


std::tuple <bool, std::list <std::string>> visitOperation(vpiHandle h) {
  vpiHandle ops = vpi_iterate(vpiOperand, h);
  std::list <std::string> current;
  std::string out = "";
  bool constantsOnly = true;
  bool chooseVars = true;

  if(!saveVariables) {
    std::list <std::string> variables;
    std::cout << "Handle type: " << UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)(vpi_get(vpiType, h))) << "(" << std::to_string(vpi_get(vpiType, h)) << ")" << std::endl;
    const int type = vpi_get(vpiOpType, h);
    std::cout << "Operation type: " << UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)type) << "(" << std::to_string(type) << ")" << std::endl;
    std::string symbol = "";
    switch(type) {
      // some of the operations cannot appear
      // within control expressions; and so are not included here
      case 3  : symbol += " !  "; break;
      case 4  : symbol += " ~  "; break;
      case 5  : symbol += " &  "; break;
      case 7  : symbol += " |  "; break;
      case 11 : symbol += " -  "; break;
      case 12 : symbol += " /  "; break;
      case 14 : symbol += " == "; break;
      case 15 : symbol += " != "; break;
      case 16 : symbol += " === "; break;
      case 17 : symbol += " !== "; break;
      case 18 : symbol += " >  "; break;
      case 19 : symbol += " >= "; break;
      case 20 : symbol += " <  "; break;
      case 21 : symbol += " <= "; break;
      case 22 : symbol += " << "; break;
      case 23 : symbol += " >> "; break;
      case 24 : symbol += " +  "; break;
      case 25 : symbol += " *  "; break;
      case 26 : symbol += " && "; break;
      case 27 : symbol += " || "; break;
      case 28 : symbol += " &  "; break;
      case 29 : symbol += " |  "; break;
      case 30 : symbol += " ^  "; break;
      case 32 : symbol += " :  "; break; 
      case 33 : symbol += " ,  "; break; //concat
      case 34 : symbol += "  { "; break;
      case 41 : symbol += " <<< "; break;
      case 42 : symbol += " >>> "; break;
      case 67 : symbol += " '( "; break;
      case 71 : symbol += " ,  "; break; //streaming left to right
      case 72 : symbol += " ,  "; break; //streaming right to left
      case 95 : symbol += " ,  "; break; 
      default : symbol += " UNKNOWN_OP(" + std::to_string(type) + ") " ; break;
    }

    std::cout << "Found symbol\n";
    if(ops) {
      int opCnt = 0;
      //opening operand, if any
      if(type == 33)
        out += "{";
      else if(type == 71)
        out += "{>>{";
      else if(type == 72)
        out += "{<<{";
      else if(type == 3 || type == 4 || type == 5 || type == 7)
        out += symbol;

      //operation body
      while(vpiHandle oph = vpi_scan(ops)) {
        std::cout << "Walking on operands\n";
        if(opCnt == 0) {
          if(type == 67) {
            std::cout << "Finding typespec\n";
            vpiHandle num = vpi_handle(vpiTypespec, h);
            out += (visitLeaf(num)).front();
            out += symbol;
          } 

          if(((const uhdm_handle *)oph)->type == UHDM::uhdmhier_path) {
            int w = search_width(oph);
            if(w<0 && w>5)
              chooseVars &= false;
            std::string tmp = visithier_path(oph);
            out += tmp;
            variables.push_back(tmp);
            constantsOnly &= false;
          }
          else if(((const uhdm_handle *)oph)->type == UHDM::uhdmoperation) {
            out += "(";
            std::list <std::string> tmp;
            bool k_tmp;
            std::tie(k_tmp, tmp) = visitOperation(oph);
            out += tmp.front();
            out += ")";
            if(saveSubex)
              current.insert(current.end(), tmp.begin(), tmp.end());
            constantsOnly &= k_tmp; //Depends on whether the operation is ignoreable
          }
          else {
            std::string tmp = (visitLeaf(oph)).front();
            out += tmp;
            variables.push_back(tmp);
            if(vpiHandle actual = vpi_handle(vpiActual, oph))
              if(((const uhdm_handle *)actual)->type != UHDM::uhdmparameter &&
                  ((const uhdm_handle *)actual)->type != UHDM::uhdmconstant)
                constantsOnly &= false;
            if(((const uhdm_handle *)oph)->type != UHDM::uhdmparameter &&
              ((const uhdm_handle *)oph)->type != UHDM::uhdmconstant) {
              constantsOnly &= false;
            }
            if(!constantsOnly) {
              int w = search_width(oph);
              if(w<0 && w>5)
                chooseVars &= false;
            }
          }
          opCnt++;
        } else {
          if(opCnt == 1) 
            if(type == 32)
              out += " ? ";
            else if(type == 95)
              out += " inside { ";
            else out += symbol;
          else    
            out += symbol;
          opCnt++;
          if(((const uhdm_handle *)oph)->type == UHDM::uhdmhier_path) {
            int w = search_width(oph);
            std::string tmp = visithier_path(oph);
            out += tmp;
            variables.push_back(tmp);
            constantsOnly &= false;
            if(w<0 && w>5)
              chooseVars &= false;
          }
          else if(((const uhdm_handle *)oph)->type == UHDM::uhdmoperation) {
            out += "(";
            std::list <std::string> tmp;
            bool k_tmp;
            std::tie(k_tmp, tmp) = visitOperation(oph);
            out += tmp.front();
            out += ")";
            if(saveSubex)
              current.insert(current.end(), tmp.begin(), tmp.end());
            constantsOnly &= k_tmp;
          }
          else {
            std::string tmp = (visitLeaf(oph)).front();
            out += tmp;
            variables.push_back(tmp);
            if(vpiHandle actual = vpi_handle(vpiActual, oph))
              if(((const uhdm_handle *)actual)->type != UHDM::uhdmparameter &&
                  ((const uhdm_handle *)actual)->type != UHDM::uhdmconstant)
                constantsOnly &= false;
          
            if(((const uhdm_handle *)oph)->type != UHDM::uhdmparameter &&
              ((const uhdm_handle *)oph)->type != UHDM::uhdmconstant) {
              constantsOnly &= false;
            }
            if(constantsOnly) {
              int w = search_width(oph);
              if(w<0 && w>5)
                chooseVars &= false;
            }
          }
        }

        vpi_release_handle(oph);
      }
      //closing operand
      if(type == 33 ||
         type == 34 || 
         type == 95 || 
         type == 71 || 
         type == 72)
        out += " }";
      else if(type == 67) 
        out += " )";

      if(constantsOnly) 
        std::cout << "Operation is constants-only: " << out << std::endl;
      //if(!chooseVars) {
        std::cout << "Inserting Operation\n";
        current.push_front(out);
      //}
      if(chooseVars) {
        std::cout << "Inserting Variables\n";
        current.insert(current.end(), variables.begin(), variables.end());
      }

    } else {
      std::cerr << "Couldn't iterate on operands! Iterator type: " << 
        UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)ops)->type) << std::endl;
    }
  } 

  //Control Variables
  else {
    if(ops)
      while(vpiHandle op = vpi_scan(ops)) {
        std::cout << "Walking on opearands\n";
        std::list <std::string> tmp = visitLeaf(op);
        current.insert(current.end(), tmp.begin(), tmp.end());
        vpi_release_handle(op);
      }
  }
  vpi_release_handle(ops);
  return std::make_tuple(constantsOnly, current);
}

std::list <std::string> visitCond(vpiHandle h) {

  std::cout << "Walking condition; type: " << 
    UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)h)->type) << std::endl;
  std::list <std::string> current;
  switch(((const uhdm_handle *)h)->type) {
    case UHDM::uhdmpart_select :
    case UHDM::uhdmindexed_part_select :
    case UHDM::uhdmbit_select :
    case UHDM::uhdmref_obj :
    case UHDM::uhdmexpr :
      std::cout << "Leafs found\n";
      current = visitLeaf(h);
      break;
    case UHDM::uhdmhier_path :
      std::cout << "Struct found\n";
      current.push_back(visithier_path(h));
      break;
    case UHDM::uhdmoperation :
      std::cout << "Operation found\n";
      bool k;
      std::tie(k, current) = visitOperation(h);
      break;
    case UHDM::uhdmconstant :
    case UHDM::uhdmparameter : 
      std::cout << "Const/Param found at cond; ignored\n";
			//current.push_back(visitLeaf(h));
    default: 
      std::cout << "UNKNOWN type found\n";
      break;
  }
  return current;
}

void visitIfElse(vpiHandle h) {
  std::list <std::string> out;
  std::cout << "Found IfElse/If\n";
  if(vpiHandle c = vpi_handle(vpiCondition, h)) {
    std::cout << "Found condition\n";
    out = visitCond(c);
    vpi_release_handle(c);
  } else std::cerr << "No condition found\n";
  std::cout << "Saving to list: \n";
  print_list(out);
  
  std::list <std::string> tmp(out);
  ifs.insert(ifs.end(), out.begin(), out.end());
  all.insert(all.end(), tmp.begin(), tmp.end());

  if(vpiHandle s = vpi_handle(vpiStmt, h)) {
    std::cout << "Found statements\n";
    visitBlocks(s);
    vpi_release_handle(s);
  } else std::cout << "Statements not found\n";
  return;
}

void visitCase(vpiHandle h) {
  std::list <std::string> out;
  if(vpiHandle c = vpi_handle(vpiCondition, h)) {
    std::cout << "Found condition\n";
    out = visitCond(c);
    vpi_release_handle(c);
  } else std::cout << "No condition found!\n";
  cases.insert(cases.end(), out.begin(), out.end());
  all.insert(all.end(), out.begin(), out.end());
  std::cout << "Parsing case item; type: " << 
    UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)h)->type) << std::endl;
  vpiHandle newh = vpi_iterate(vpiCaseItem, h);
  if(newh) {
    while(vpiHandle sh = vpi_scan(newh)) {
      std::cout << "Found case item; type: " << 
        UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)sh)->type) << std::endl;
      visitBlocks(sh);
      vpi_release_handle(sh);
    }
    vpi_release_handle(newh);
  } else std::cout << "Statements not found\n";
  return;
}

void visitAssignment(vpiHandle h) {
  std::cout << "Walking assignment\n";
  if(vpiHandle rhs = vpi_handle(vpiRhs, h)) {
    if(((uhdm_handle *)rhs)->type == UHDM::uhdmoperation) {
      std::cout << "Operation found in RHS\n";
      const int n = vpi_get(vpiOpType, rhs);
      if (n == 32) {
        visitTernary(rhs);
      }
      else {
        if(vpiHandle operands = vpi_iterate(vpiOperand, rhs)) {
          while(vpiHandle operand = vpi_scan(operands)) {
            std::cout << "Operand type: "
              << UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((uhdm_handle *)operand)->type) << std::endl;
            if(((uhdm_handle *)operand)->type == UHDM::uhdmoperation) {
              std::cout << "\nOperand is operation; checking if ternary\n";
              findTernaryInOperation(operand);
              vpi_release_handle(operand);
            }
          }
          vpi_release_handle(operands);
        }
      }
    } else
      std::cout << "Not an operation on the RHS; ignoring\n";
    vpi_release_handle(rhs);
  } else
    std::cerr << "Assignment without RHS handle\n";
  return;
}

void visitBlocks(vpiHandle h) {
  std::cout << "Block type: " 
    << UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)h)->type) << std::endl;
  switch(((const uhdm_handle *)h)->type) {
    case UHDM::uhdmcase_items : 
    case UHDM::uhdmbegin : {
      vpiHandle i;
      i = vpi_iterate(vpiStmt,h);
      while (vpiHandle s = vpi_scan(i) ) {
        visitBlocks(s);
        vpi_release_handle(s);
      }
      vpi_release_handle(i);
      break;
    }
    case UHDM::uhdmstmt :
      if(((const uhdm_handle *)h)->type == UHDM::uhdmevent_control) 
        visitBlocks(h);
      break;
    case UHDM::uhdmcase_stmt :
      std::cout << "Case statement found\n";
      visitCase(h);
      break;
    case UHDM::uhdmif_stmt :
    case UHDM::uhdmelse_stmt : 
    case UHDM::uhdmif_else :
      std::cout << "If/IfElse statement found\n";
      visitIfElse(h);
      if(vpiHandle el = vpi_handle(vpiElseStmt, h)) {
        std::cout << "Else statement found\n";
        visitIfElse(el);
      } else std::cout << "Didn't find else statement\n";
      break;
    case UHDM::uhdmalways : {
      vpiHandle newh = vpi_handle(vpiStmt, h);
      visitBlocks(newh);
      vpi_release_handle(newh);
      break;
    }
    case UHDM::uhdmassignment : {
      std::cout << "Assignment found; checking for ternaries\n";
      visitAssignment(h);
      break;
    }
    default :
      if(vpiHandle newh = vpi_handle(vpiStmt, h)) {
        std::cout << "UNKNOWN type; but statement found inside\n";
        visitBlocks(newh);
      } else {
        std::cerr << "UNKNOWN type; skipping processing this node\n";
        //Accommodate all cases eventually
      }
      break;
  }
  return;
}

void findTernaryInOperation(vpiHandle h) {
  std::string out = "";
  std::cout << "Checking if operand is ternary\n";
  if(((uhdm_handle *)h)->type == UHDM::uhdmoperation) {
    const int nk = vpi_get(vpiOpType, h);
    //std::cout << "Type " << UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)nk) << "\n";
    if(nk == 32) {
      std::cout << "An operand is ternary\n";
      visitTernary(h);
    }

  }
  return;
}

void visitTernary(vpiHandle h) {
  std::list <std::string> current;
  std::cout << "Analysing ternary operation\n";
  vpiHandle i = vpi_iterate(vpiOperand, h);
  bool first = true;
  if(i) {
    while (vpiHandle op = vpi_scan(i)) {
      std::cout << "Operand type: " 
        << ((const uhdm_handle *)op)->type << std::endl;
      switch(((const uhdm_handle *)op)->type) {
        case UHDM::uhdmoperation :
          {
            std::cout << "Operation found in ternary\n";
            findTernaryInOperation(op);
            if(first) {
              std::list <std::string> out;
              bool k;
              std::tie(k, out) = visitOperation(op);
              current.insert(current.end(), out.begin(), out.end());
              first = false;
            }
            break;
          }
        case UHDM::uhdmref_obj :
        case UHDM::uhdmpart_select :
        case UHDM::uhdmbit_select :
        case UHDM::uhdmconstant : 
        case UHDM::uhdmparameter :
        case UHDM::uhdmexpr : //look into this again!
          if(first) {
            std::cout << "Leaf found in ternary\n";
            std::list <std::string> tmp = visitLeaf(op);
            current.insert(current.end(), tmp.begin(), tmp.end());
            first = false;
          }
          break;
        case UHDM::uhdmhier_path :
          if(first) {
            std::cout << "Struct found in ternary\n";
            std::string out = visithier_path(op);
            current.push_back(out);
            first = false;
          }
          break;
        default: 
          if(first) {
            std::cerr << "UNKNOWN type in ternary";
          }
          break;
      }
      vpi_release_handle(op);
    }
    vpi_release_handle(i);
  } else
    std::cout << "Couldn't iterate through statements!!\n";
  std::cout << "Saving ternaries...\n";
  print_list(current);
  ternaries.insert(ternaries.end(), current.begin(), current.end());
  all.insert(all.end(), current.begin(), current.end());
  return;
}

int evalLeaf(vpiHandle h) {
  switch(((const uhdm_handle *)h)->type) {
    case UHDM::uhdmconstant : {
      std::cout << "Evaluating Constant\n";
      s_vpi_value value;
      vpi_get_value(h, &value);
      if(value.format)
        return value.value.integer;
      else return 0;
    }
    case UHDM::uhdmparameter : {
      std::cout << "Evaluating Param\n";
      std::map <std::string, int>::iterator it;
      std::string name = "";
      if(const char *s = vpi_get_str(vpiName, h)) {
        std::cout << "Finding param " << s << std::endl;
        name = s;
      }
      it = params.find(name);
      if(it == params.end()) {
        std::cout << "UNKNOWN_PARAM\n";
        return 0;
      } else return it->second;
      break;
    }
    case UHDM::uhdmoperation:
      std::cout << "Evaluating operation\n";
      return evalOperation(h);
    default:
      std::cout << "Expression cannot be evaluated!!\n";
      return 0;
  }
  return 0;
}

int evalOperation(vpiHandle h) {
  //Some supported evaluatable operations we support
  int ops[2];
  int *op = ops;
  if(vpiHandle opi = vpi_iterate(vpiOperand, h)) {
    while(vpiHandle oph = vpi_scan(opi)) {
      std::cout << "Evaluating operand\n";
      switch(((const uhdm_handle *)oph)->type) {
        case UHDM::uhdmoperation: 
          *op = evalOperation(oph);
          op++;
          break;
        case UHDM::uhdmparameter :
        case UHDM::uhdmconstant : 
          *op = evalLeaf(oph);
          op++;
          break;
        default:
          std::cout << "Unable to evaluate operation\n";
          return 0;
      }
    }
    vpi_release_handle(opi);
  } else
    std::cerr << "Couldn't iterate on operands\n";

  const int type = vpi_get(vpiOpType, h);
  std::cout << "Operation type in eval: " << UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)type) << "(" << std::to_string(type) << ")" << std::endl;
  switch(type) {
    case 11 : return ops[0] - ops[1];
    case 12 : return ops[0] / ops[1];
    case 13 : return ops[0] % ops[1];
    case 22 : return ops[0] << ops[1];
    case 23 : return ops[0] >> ops[1];
    case 24 : return ops[0] + ops[1];
    case 25 : return ops[0] * ops[1];
    default : return 0;
  }
  std::cout << "Done evaluating operation\n";
  return 0;
}

int width(vpiHandle h, int *ptr) {
  std::cout << "Calculating width\n";
  vpiHandle ranges;
  std::string out = "";
  int dims=0;
  int *w;
  w = ptr;
  if((ranges = vpi_iterate(vpiRange, h))) {
    std::cout << "Range found\n";
    while (vpiHandle range = vpi_scan(ranges) ) {
      if(dims < 4) {
        std::cout << "New dimension\n";
        dims++;
        vpiHandle lh = vpi_handle(vpiLeftRange, range);
        vpiHandle rh = vpi_handle(vpiRightRange, range);
        *w = evalLeaf(lh) - evalLeaf(rh) + 1;
        std::cout << "One range is: " << *w << std::endl;
        w++;
        vpi_release_handle(lh);
        vpi_release_handle(range);
      } else std::cout << "Dimension overflow!\n";
    }
  } else {
    std::cout << "Single bit value\n";
    *w = 1;
    dims++;
    //meaning either a bit or an unknown range
  }
  vpi_release_handle(ranges);
  return dims;
}

void visitVariables(vpiHandle i) {
  std::cout << "Walking variables\n";
  while (vpiHandle h = vpi_scan(i)) {
    std::string out = "";
    switch(((const uhdm_handle *)h)->type) {
      case UHDM::uhdmstruct_var :
      case UHDM::uhdmstruct_net : {
        std::cout << "Finding width of struct\n";
        std::string base = vpi_get_str(vpiFullName, h);
        if(vpiHandle ts = vpi_handle(vpiTypespec, h)) {
          std::cout << "Finding Typespec\n";
          if(vpiHandle tsi = vpi_iterate(vpiTypespecMember, ts)) {
            std::cout << "Found TypespecMember\n";
            while(vpiHandle tsm = vpi_scan(tsi)) {
              std::cout << "Iterating\n";
              vpiHandle tsmts = vpi_handle(vpiTypespec, tsm);
              int t = vpi_get(vpiNetType, tsmts);
              struct vars tmp;
              tmp.type = t == 48 ? "Reg" :
                "Wire(" + UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)tsmts)->type) + ")";
              tmp.name = base + ".";
              tmp.name += vpi_get_str(vpiName, tsm);
              tmp.dims = width(tsmts, tmp.width);
              netsCurrent.push_back(tmp);
              std::cout << "Found TypespecMember names: " << tmp.name << std::endl;
            }
          }
        }
        break;
      }
      default: {
          int t = vpi_get(vpiNetType, h);
          struct vars tmp;
          tmp.type = t == 48 ? "Reg" :
            "Wire(" + UHDM::UhdmName((UHDM::UHDM_OBJECT_TYPE)((const uhdm_handle *)h)->type) + ")";
          tmp.name = vpi_get_str(vpiFullName, h);
          tmp.dims = width(h, tmp.width);
          netsCurrent.push_back(tmp);
          break;
      }
    }
    vpi_release_handle(h);
  }
  std::cout << "No more nets\n";
  vpi_release_handle(i);
  return;
}

void visitParams(vpiHandle p) {
  while(vpiHandle h = vpi_scan(p)) {
    std::cout << "Found a handle " << ((const uhdm_handle *)h)->type << "\n";
    std::string name = "";
    if(vpiHandle l = vpi_handle(vpiLhs, h)) {
      name = vpi_get_str(vpiName, l);
      std::cout << "LHS: " << name << std::endl;
      vpi_release_handle(l);
    } else {
      std::cout << "Unable to find name of param\n";
      name = "UNKNOWN";
    }
    if(vpiHandle r = vpi_handle(vpiRhs, h)) {
      std::cout << "Found a handle " << ((const uhdm_handle *)r)->type << "\n";
      switch(((const uhdm_handle *)r)->type) {
        case UHDM::uhdmconstant: {
          s_vpi_value value;
          vpi_get_value(r, &value);
          if(value.format) {
            std::cout << "Found const assignment: " << std::to_string(value.value.integer) << std::endl;
            params.insert(std::pair<std::string, int>(name, value.value.integer));
          } else
            std::cout << "Unable to resolve constant\n";
          break;
        } 
        case UHDM::uhdmparameter: {
          std::map <std::string, int>::iterator it;
          it = params.find(name);
          if(it == params.end()) {
            std::cout << "Can't find definition of param: " << name << std::endl;
            params.insert(std::pair<std::string, int>(name, 0));
          } else {
            std::cout << "Found existing param: " << it->second << std::endl;
            params.insert(std::pair<std::string, int>(name, it->second));
          }
          break;
        }
        case UHDM::uhdmoperation: 
          std::cout << "Unexpected operation in parameter assignment\n";
        default:
          std::cout << "Didn't find a constant of param in param assignment\n";
          break;
      }
      vpi_release_handle(r);
    } else {
      std::cout << "Didn't find RHS of param assignment!!\n";
    }
  }
  return;
}


void visitTopModules(vpiHandle ti) {
  std::cout << "Exercising iterator\n";
  while(vpiHandle th = vpi_scan(ti)) {
    std::cout << "Top module handle obtained\n";
    if (vpi_get(vpiType, th) != vpiModule) {
      std::cerr << "Not a module\n";
      return;
    }

    std::cout << "Proceeding\n";
    //lambda for module visit
    std::function<void(vpiHandle, std::string)> visit =
      [&visit](vpiHandle mh, std::string depth) {

        std::string out_f;
        std::string defName;
        std::string objectName;
        if (const char* s = vpi_get_str(vpiDefName, mh)) {
          defName = s;
        }
        if (const char* s = vpi_get_str(vpiName, mh)) {
          if (!defName.empty()) {
            defName += " ";
          }
          objectName = std::string("(") + s + std::string(")");
        }
        std::string file = "";
        if (const char* s = vpi_get_str(vpiFile, mh))
          file = s;
        std::cout << "Walking module: " + defName + objectName + "\n";// +
        ", file:" + file +
        ", line:" + std::to_string(vpi_get(vpiLineNo, mh)) + "\n";

        // Params
        std::cout << "****************************************\n";
        std::cout << "      ***  Now finding ports         ***\n";
        std::cout << "****************************************\n";
        if(vpiHandle ports = vpi_iterate(vpiPorts, mh)) {
          std::cout << "Found ports\n";
          visitVariables(ports);
        } else std::cout << "No ports found in current module\n";
        std::cout << "Done with ports\n";

        std::cout << "****************************************\n";
        std::cout << "      ***  Now finding params        ***\n";
        std::cout << "****************************************\n";
        if(vpiHandle pi = vpi_iterate(vpiParamAssign, mh)) { //do vpiParameter
          std::cout << "Found params\n";
          visitParams(pi);
        } else std::cout << "No params found in current module/genscope\n";
        std::cout << "Found below params:\n";
        std::map<std::string, int>::iterator pitr;
        for (pitr = params.begin(); pitr != params.end(); ++pitr)
          std::cout << pitr->first << " = " << pitr->second << std::endl;

        // Variables
        std::cout << "****************************************\n";
        std::cout << "      ***  Now finding variables     ***\n";
        std::cout << "****************************************\n";
        if(vpiHandle vi = vpi_iterate(vpiVariables, mh)) {
          std::cout << "Found variables\n"; 
          visitVariables(vi);
        } else std::cout << "No variables found in current module\n";
        std::cout << "Done with vars\n";

        // Nets
        std::cout << "****************************************\n";
        std::cout << "      ***     Now finding nets       ***\n";
        std::cout << "****************************************\n";
        if(vpiHandle ni = vpi_iterate(vpiNet, mh)) {
          std::cout << "Found nets\n";
          visitVariables(ni);
        } else std::cout << "No nets found in current module\n";

        // ContAssigns:
        std::cout << "****************************************\n";
        std::cout << "      *** Now finding cont. assigns  ***\n";
        std::cout << "****************************************\n";
        vpiHandle ci = vpi_iterate(vpiContAssign, mh);
        if(ci) {
          std::cout << "Found continuous assign statements\n";
            while (vpiHandle ch = vpi_scan(ci)) {
              std::cout << "ContAssign Info -> " <<
                std::string(vpi_get_str(vpiFile, ch)) <<
                ", line:" << std::to_string(vpi_get(vpiLineNo, ch)) << std::endl;
              visitAssignment(ch);
              vpi_release_handle(ch);
            }
          vpi_release_handle(ci);
        } else std::cout << "No continuous assign statements found in current module\n";

        //ProcessStmts:
        std::cout << "****************************************\n";
        std::cout << "      *** Now finding process blocks ***\n";
        std::cout << "****************************************\n";
        vpiHandle ai = vpi_iterate(vpiProcess, mh);
        if(ai) {
          std::cout << "Found always block\n";
            while(vpiHandle ah = vpi_scan(ai)) {
              visitBlocks(ah);
              vpi_release_handle(ah);
            }
          vpi_release_handle(ai);
        } else std::cout << "No always blocks in current module\n";

        //Accumulate variables:
        nets.insert(nets.end(), netsCurrent.begin(), netsCurrent.end());
        netsCurrent.clear();
        paramsAll.insert(params.begin(), params.end());
        params.clear();

        //Statistics:
        static int numTernaries, numIfs, numCases;
        nTernaries.push_back(0);//ternaries.size() - numTernaries);
        nIfs.push_back(ifs.size() - numIfs);
        nCases.push_back(cases.size() - numCases);
        std::cout << "Block: " << defName + objectName << " | numTernaries: " << ternaries.size() - numTernaries
          << " | numCases: " << cases.size() - numCases << " | numIfs: " << ifs.size() - numIfs << std::endl; 
        numTernaries = ternaries.size();
        numIfs       = ifs.size();
        numCases     = cases.size();

        // Recursive tree traversal
        if (vpi_get(vpiType, mh) == vpiModule ||
            vpi_get(vpiType, mh) == vpiGenScope) {
          vpiHandle i = vpi_iterate(vpiModule, mh);
          while (vpiHandle h = vpi_scan(i)) {
            std::cout << "Iterating next module\n";
            depth = depth + "  ";
            visit(h, depth);
            vpi_release_handle(h);
          }
          vpi_release_handle(i);
        } else
          std::cout << "No children modules found\n";

        if (vpi_get(vpiType, mh) == vpiModule ||
            vpi_get(vpiType, mh) == vpiGenScope) {
          vpiHandle i = vpi_iterate(vpiGenScopeArray, mh);
          while (vpiHandle h = vpi_scan(i)) {
            std::cout << "Iterating genScopeArray\n";
            depth = depth + "  ";
            visit(h, depth);
            vpi_release_handle(h);
          }
          vpi_release_handle(i);
        } else
         std::cout << "No children genScopeArray found\n";

        if (vpi_get(vpiType, mh) == vpiGenScopeArray) {
          vpiHandle i = vpi_iterate(vpiGenScope, mh);
          while (vpiHandle h = vpi_scan(i)) {
            std::cout << "Iterating genScope\n";
            visit(h, depth);
            vpi_release_handle(h);
          }
          vpi_release_handle(i);
        } else
          std::cout << "No children genScope found\n";
        return;
      };
    visit(th, "");
    vpi_release_handle(th);
  }
  vpi_release_handle(ti);
  return;
}

int main(int argc, const char** argv) {
  // Read command line, compile a design, use -parse argument
  unsigned int code = 0;
  SURELOG::SymbolTable* symbolTable = new SURELOG::SymbolTable();
  SURELOG::ErrorContainer* errors = new SURELOG::ErrorContainer(symbolTable);

  SURELOG::CommandLineParser* clp =
    new SURELOG::CommandLineParser(errors, symbolTable, false, false);
  clp->noPython();
  clp->setParse(true);
  clp->setwritePpOutput(true);
  clp->setCompile(true);
  clp->setElaborate(true);  // Request Surelog instance tree Elaboration
  // clp->setElabUhdm(true);  // Request UHDM Uniquification/Elaboration

  bool success = clp->parseCommandLine(argc, argv);
  errors->printMessages(clp->muteStdout());
  vpiHandle the_design = 0;
  SURELOG::scompiler* compiler = nullptr;
  if (success && (!clp->help())) {
    compiler = SURELOG::start_compiler(clp);
    the_design = SURELOG::get_uhdm_design(compiler);
    auto stats = errors->getErrorStats();
    code = (!success) | stats.nbFatal | stats.nbSyntax | stats.nbError;
  }

  std::string out = "";

  std::cout << "UHDM Elaboration...\n";
  UHDM::Serializer serializer;
  UHDM::ElaboratorListener* listener =
    new UHDM::ElaboratorListener(&serializer, false);
  listener->listenDesigns({the_design});
  delete listener;
  std::cout << "Listener in place\n";

  // Browse the UHDM Data Model using the IEEE VPI API.
  // See third_party/Verilog_Object_Model.pdf

  // Either use the
  // - C IEEE API, (See third_party/UHDM/tests/test_helper.h)
  // - or C++ UHDM API (See third_party/UHDM/headers/*.h)
  // - Listener design pattern (See third_party/UHDM/tests/test_listener.cpp)
  // - Walker design pattern (See third_party/UHDM/src/vpi_visitor.cpp)

  if (the_design) {
    UHDM::design* udesign = nullptr;
    if (vpi_get(vpiType, the_design) == vpiDesign) {
      // C++ top handle from which the entire design can be traversed using the
      // C++ API
      udesign = UhdmDesignFromVpiHandle(the_design);
      std::cout << "Design name (C++): " + udesign->VpiName() + "\n";
    }
    // Example demonstrating the classic VPI API traversal of the folded model
    // of the design Flat non-elaborated module/interface/packages/classes list
    // contains ports/nets/statements (No ranges or sizes here, see elaborated
    // section below)
    std::cout <<
      "Design name (VPI): " + std::string(vpi_get_str(vpiName, the_design)) +
      "\n";
    // Flat Module list:
    std::cout << "Module List:\n";
    //      topmodule -- instance scope
    //        allmodules -- assign (ternares), always (if, case, ternaries)

    vpiHandle ti = vpi_iterate(UHDM::uhdmtopModules, the_design);
    if(ti) {
      std::cout << "Walking uhdmtopModules\n";
      // The walk
      visitTopModules(ti);
    } else std::cout << "No uhdmtopModules found!";
  }

  
  std::cout << "\n\n\n*** Printing all conditions ***\n\n\n";
  print_list(all, true, "allCtrlSigs");
  std::cout << "\n\n\n*** Printing case conditions ***\n\n\n";
  print_list(cases, true, "sigsFromCaseStmts");
  std::cout << "\n\n\n*** Printing if/if-else conditions ***\n\n\n";
  print_list(ifs, true, "sigsFromIfStmts");
  std::cout << "\n\n\n*** Printing ternary conditions ***\n\n\n";
  print_list(ternaries, true, "sigsFromTerns");
  std::cout << "\n\n\n*** Printing variables ***\n\n\n";
  std::ofstream file;
  file.open("nets", std::ios_base::app);
  for (auto const &i: nets) {
      file << i.name << " ";
      int k=0;
      while(k<i.dims) {
        file << i.width[k] << " ";
        k++;
      }
      file << std::endl;
  }
  file.close();
  std::cout << "\n\n\n*** Printing params ***\n\n\n"; //why?
  file.open("params", std::ios_base::app);
  std::map<std::string, int>::iterator itr;
  for (itr = paramsAll.begin(); itr != paramsAll.end(); ++itr)
    file << itr->first << " = " << itr->second << std::endl;
  file.close();


  std::cout << "\n\n\n*** Parsing Complete!!! ***\n\n\n";


  // Do not delete these objects until you are done with UHDM
  SURELOG::shutdown_compiler(compiler);
  delete clp;
  delete symbolTable;
  delete errors;
  return code;
}
