/**
 * @file IRGenerator.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief AST遍历产生线性IR
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "AST.h"
#include "IRCode.h"
#include "IRGenerator.h"
#include "IRInst.h"
#include "SymbolTable.h"
#include "Value.h"
#include "ValueType.h"

/// @brief 构造函数
/// @param _root AST的根
/// @param _symtab 符号表
IRGenerator::IRGenerator(ast_node * _root, SymbolTable * _symtab) : root(_root), symtab(_symtab)
{
    /* 叶子节点 */
    ast2ir_handlers[ast_operator_type::AST_OP_LEAF_LITERAL_UINT] = &IRGenerator::ir_leaf_node_uint;
    ast2ir_handlers[ast_operator_type::AST_OP_LEAF_LITERAL_FLOAT] = &IRGenerator::ir_leaf_node_float;
    ast2ir_handlers[ast_operator_type::AST_OP_LEAF_VAR_ID] = &IRGenerator::ir_leaf_node_var_id;

    /* 表达式运算， 加减 */
    ast2ir_handlers[ast_operator_type::AST_OP_SUB] = &IRGenerator::ir_sub;
    ast2ir_handlers[ast_operator_type::AST_OP_ADD] = &IRGenerator::ir_add;
    ast2ir_handlers[ast_operator_type::AST_OP_MUL] = &IRGenerator::ir_mul;
    ast2ir_handlers[ast_operator_type::AST_OP_DIV] = &IRGenerator::ir_div;
    ast2ir_handlers[ast_operator_type::AST_OP_MOD] = &IRGenerator::ir_mod;

    /* 关系运算 */
    ast2ir_handlers[ast_operator_type::AST_OP_LE] = &IRGenerator::ir_le;
    ast2ir_handlers[ast_operator_type::AST_OP_LT] = &IRGenerator::ir_lt;
    ast2ir_handlers[ast_operator_type::AST_OP_GT] = &IRGenerator::ir_gt;
    ast2ir_handlers[ast_operator_type::AST_OP_GE] = &IRGenerator::ir_ge;
    ast2ir_handlers[ast_operator_type::AST_OP_NE] = &IRGenerator::ir_ne;
    ast2ir_handlers[ast_operator_type::AST_OP_EQ] = &IRGenerator::ir_eq;

    /* 语句 */
    ast2ir_handlers[ast_operator_type::AST_OP_EXPR] = &IRGenerator::ir_expr_noshow;
    ast2ir_handlers[ast_operator_type::AST_OP_EXPR_SHOW] = &IRGenerator::ir_expr_show;
    ast2ir_handlers[ast_operator_type::AST_OP_ASSIGN] = &IRGenerator::ir_assign;
    ast2ir_handlers[ast_operator_type::AST_OP_RETURN_STATEMENT] = &IRGenerator::ir_return;

    /* 函数调用 */
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_CALL] = &IRGenerator::ir_function_call;

    /* 函数定义 */
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_DEF] = &IRGenerator::ir_function_define;
    ast2ir_handlers[ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS] = &IRGenerator::ir_function_formal_params;

    /* 变量定义 */
    ast2ir_handlers[ast_operator_type::AST_OP_VAR_DECLS] = &IRGenerator::ir_var_list;
    ast2ir_handlers[ast_operator_type::AST_OP_VAR_DECL] = &IRGenerator::ir_var_define;
    ast2ir_handlers[ast_operator_type::AST_OP_TYPE_INT] = &IRGenerator::ir_type_int;

    /* 语句块 */
    ast2ir_handlers[ast_operator_type::AST_OP_BLOCK] = &IRGenerator::ir_block;

    /* 编译单元 */
    ast2ir_handlers[ast_operator_type::AST_OP_COMPILE_UNIT] = &IRGenerator::ir_compile_unit;
}

/// @brief 编译单元AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_compile_unit(ast_node * node)
{
    // 新建main函数并默认设置当前函数为main函数
    symtab->mainFunc = symtab->newFunction("main", BasicType::TYPE_INT);
    symtab->currentFunc = symtab->mainFunc;

    // 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
    InterCode & irCode = symtab->currentFunc->getInterCode();

    // 创建并加入Entry入口指令
    irCode.addInst(new EntryIRInst());

    // 创建出口指令并不加入出口指令，等函数内的指令处理完毕后加入出口指令
    IRInst * exitLabelInst = new LabelIRInst();
    symtab->currentFunc->setExitLabel(exitLabelInst);

    // 新建一个Value，用于保存函数的返回值，如果没有返回值可不用申请，
    Value * retValue = symtab->currentFunc->newVarValue(BasicType::TYPE_INT);
    symtab->currentFunc->setReturnValue(retValue);

    for (auto son: node->sons) {

        // 遍历编译单元，要么是函数定义，要么是语句
        ast_node * son_node = ir_visit_ast_node(son);
        if (!son_node) {
            return false;
        }

        node->blockInsts.addInst(son_node->blockInsts);
    }

    // 除了函数定义的指令外都加入到main函数的指令当中
    irCode.addInst(node->blockInsts);

    // 添加函数出口Label指令，主要用于return语句跳转到这里进行函数的退出
    irCode.addInst(exitLabelInst);

    // 尾部追加一个return 0指令，使得main函数的格式正确
    irCode.addInst(new ExitIRInst(new ConstValue(0)));

    // main函数移动到列表的尾部，以便后续简便处理
    symtab->moveFunctionEnd(symtab->mainFunc);

    // 设置成空，使得后续访问该变量出错。
    symtab->currentFunc = nullptr;

    return true;
}

/// @brief 函数定义AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_define(ast_node * node)
{
    // 创建一个函数，用于当前函数处理
    if (symtab->currentFunc != symtab->mainFunc) {
        // 函数中嵌套定义函数，这是不允许的，错误退出
        // TODO 自行追加语义错误处理
        return false;
    }

    // 创建一个新的函数定义，函数的返回类型设置为VOID，待定，必须等return时才能确定，目前可以是VOID或者INT类型
    symtab->currentFunc = new Function(node->name, BasicType::TYPE_VOID);
    bool result = symtab->insertFunction(symtab->currentFunc);
    if (!result) {
        // 清理资源
        delete symtab->currentFunc;

        // 恢复当前函数指向main函数
        symtab->currentFunc = symtab->mainFunc;

        // 函数已经定义过了，不能重复定义，语义错误：出错返回。
        // TODO 自行追加语义错误处理

        return false;
    }

    // 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
    InterCode & irCode = symtab->currentFunc->getInterCode();

    // 这里也可增加一个函数入口Label指令，便于后续基本块划分

    // 创建并加入Entry入口指令
    irCode.addInst(new EntryIRInst());

    // 创建出口指令并不加入出口指令，等函数内的指令处理完毕后加入出口指令
    IRInst * exitLabelInst = new LabelIRInst();

    // 函数出口指令保存到函数信息中，因为在语义分析函数体时return语句需要跳转到函数尾部，需要这个label指令
    symtab->currentFunc->setExitLabel(exitLabelInst);

    // 新建一个Value，用于保存函数的返回值，如果没有返回值可不用申请，
    // 目前未知，先创建一个，不用后续可释放
    Value * retValue = symtab->currentFunc->newVarValue(BasicType::TYPE_INT);

    // 保存函数返回值变量到函数信息中，在return语句翻译时需要设置值到这个变量中
    symtab->currentFunc->setReturnValue(retValue);

    // 遍历函数体内的每个语句
    for (auto son: node->sons) {

        // 遍历函数定义，孩子要么是形式参数，要么是block
        ast_node * son_node = ir_visit_ast_node(son);
        if (!son_node) {

            // 对函数体内的语句进行语义分析时出现错误
            return false;
        }

        // IR指令追加到当前的节点中
        node->blockInsts.addInst(son_node->blockInsts);
    }

    // 此时，所有指令都加入到当前函数中，也就是node->blockInsts

    // node节点的指令移动到函数的IR指令列表中
    irCode.addInst(node->blockInsts);

    // 添加函数出口Label指令，主要用于return语句跳转到这里进行函数的退出
    irCode.addInst(exitLabelInst);

    // 检查函数是否有返回值类型，则需要设置返回值，否则不设置
    if (symtab->currentFunc->getReturnType().type != BasicType::TYPE_VOID) {
        // 函数出口指令
        irCode.addInst(new ExitIRInst(retValue));
    } else {
        // 清理资源恢复原状
        symtab->currentFunc->deleteVarValue(retValue);
        symtab->currentFunc->setReturnValue(nullptr);
        delete retValue;

        // 函数出口指令
        irCode.addInst(new ExitIRInst());
    }

    // 恢复成指向main函数
    symtab->currentFunc = symtab->mainFunc;

    return true;
}

/// @brief 形式参数AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_formal_params(ast_node * node)
{
    // 获取当前要保存函数的形式参数清单
    auto & params = symtab->currentFunc->getParams();

    // 遍历形式参数列表，孩子是叶子节点
    for (auto son: node->sons) {

        // 创建变量，默认整型
        Value * var = symtab->currentFunc->newVarValue(son->name, BasicType::TYPE_INT);

        // 默认是整数类型
        params.emplace_back(son->name, BasicType::TYPE_INT, var);
    }

    return true;
}

/// @brief 函数调用AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_call(ast_node * node)
{
    std::vector<Value *> realParams;

    // 根据函数名查找函数，看是否存在。若不存在则出错
    // 这里约定函数必须先定义后使用
    auto pFunction = symtab->findFunction(node->name);
    if (nullptr == pFunction) {
        // TODO 这里输出错误信息
        return false;
    }

    // 设置存在函数调用
    symtab->currentFunc->setExistFuncCall(true);

    // 如果没有孩子，也认为是没有参数
    if (!node->sons.empty()) {

        // 只有一个节点，实际参数列表
        auto paramsNode = node->sons[0];

        int argsCount = (int) paramsNode->sons.size();

        // 设置最大函数调用参数个数
        if (argsCount > symtab->currentFunc->getMaxFuncCallArgCnt()) {
            symtab->currentFunc->setMaxFuncCallArgCnt(argsCount);
        }

        // 遍历参数列表，孩子是表达式
        // 这里自左往右计算表达式
        for (auto son: paramsNode->sons) {

            // 遍历Block的每个语句，进行显示或者运算
            ast_node * temp = ir_visit_ast_node(son);
            if (!temp) {
                return false;
            }

            realParams.push_back(temp->val);
            node->blockInsts.addInst(temp->blockInsts);
        }
    }

    // 创建临时变量，用于保存函数调用的结果
    Value * resultVal = nullptr;

    // 返回调用有返回值，则需要分配临时变量
    if (pFunction->getReturnType().type == BasicType::TYPE_INT) {
        resultVal = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);
    }

    // 创建函数调用指令
    node->blockInsts.addInst(new FuncCallIRInst(node->name, realParams, resultVal));

    // 设置存在函数调用，后面要根据是否函数调用进行调整栈分配策略
    symtab->currentFunc->setExistFuncCall(true);

    // 函数调用结果保存到node中，用于外部使用
    node->val = resultVal;

    return true;
}

/// @brief 语句块（含函数体）AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_block(ast_node * node)
{
    std::vector<ast_node *>::iterator pIter;
    for (pIter = node->sons.begin(); pIter != node->sons.end(); ++pIter) {

        // 遍历Block的每个语句，进行显示或者运算
        ast_node * temp = ir_visit_ast_node(*pIter);
        if (!temp) {
            return false;
        }

        node->blockInsts.addInst(temp->blockInsts);
    }

    return true;
}

/// @brief 变量定义列表AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_var_list(ast_node * node)
{

    // 遍历变量定义列表，孩子是变量定义节点
    for (auto son: node->sons) {

        // 遍历变量定义列表，孩子是变量定义
        ast_node * son_node = ir_visit_ast_node(son);
        if (!son_node) {

            return false;
        }
    }
    return true;
}

/// @brief 变量定义AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_var_define(ast_node * node)
{

    // 孩子节点只有一个，是类型节点
    if (node->sons.size() == 1) {
        Value * val;

        val = symtab->currentFunc->findValue(node->name, false);
        if (!val) {

            // 变量不存在，则创建一个变量
            val = symtab->currentFunc->newVarValue(node->name);
        }

        node->val = val;
    } else if (node->sons.size() == 2) {
        // 遍历变量定义，孩子是类型节点，或赋值节点
        for (auto son: node->sons) {

            ast_node * son_node = ir_visit_ast_node(son);
            if (!son_node) {

                return false;
            }
        }
    }

    return true;
}

/// @brief int类型AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_type_int(ast_node * node)
{
    // 默认类型都是int，所以这里什么也不用做
    return true;
}

/// @brief 表达式语句ST节点翻译成线性中间IR的共同函数
/// @param node AST节点
/// @param show 是否显示值，true：显示，false：不显示
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_show_internal(ast_node * node, bool show)
{
    // TODO real number print

    ast_node * src1_node = node->sons[0];

    ast_node * result = ir_visit_ast_node(src1_node);
    if (!result) {
        // 解析错误
        return false;
    }

    node->blockInsts.addInst(result->blockInsts);

    if (show && (result->val != nullptr)) {

        // 创建函数调用指令
        node->blockInsts.addInst(new FuncCallIRInst("putint", result->val));

        // 设置存在函数调用，后面要根据是否函数调用进行调整栈分配策略
        symtab->currentFunc->setExistFuncCall(true);
    }

    node->val = nullptr;

    return true;
}

/// @brief 不显示表达式AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_expr_noshow(ast_node * node)
{
    return ir_show_internal(node, false);
}

/// @brief 显示表达式AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_expr_show(ast_node * node)
{
    return ir_show_internal(node, true);
}

/// @brief 整数加法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_add(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 加法节点，左结合，先计算左节点，后计算右节点

    // 加法的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 加法的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    // TODO real number add

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new BinaryIRInst(IRInstOperator::IRINST_OP_ADD_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 整数减法和单目求负AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_sub(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 减法节点，左结合，先计算左节点，后计算右节点

    //有第二个操作数进行减法，否则进行单目运算求负
    if (src2_node) {
        // 减法的左边操作数
        ast_node * left = ir_visit_ast_node(src1_node);
        if (!left) {
            // 某个变量没有定值
            return false;
        }

        // 减法的右边操作数
        ast_node * right = ir_visit_ast_node(src2_node);
        if (!right) {
            // 某个变量没有定值
            return false;
        }

        // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
        // TODO real number add

        Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);

        // 创建临时变量保存IR的值，以及线性IR指令
        node->blockInsts.addInst(left->blockInsts);
        node->blockInsts.addInst(right->blockInsts);
        node->blockInsts.addInst(new BinaryIRInst(IRInstOperator::IRINST_OP_SUB_I, resultValue, left->val, right->val));
        node->val = resultValue;

        return true;
    } else {
        ast_node * son = ir_visit_ast_node(src1_node);
        if (!son)
            return false;
        Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);

        // 创建临时变量保存IR的值，以及线性IR指令
        node->blockInsts.addInst(son->blockInsts);
        node->blockInsts.addInst(new BinaryIRInst(IRInstOperator::IRINST_OP_NEG_I, resultValue, son->val));
        node->val = resultValue;

        return true;
    }
}

/// @brief 整数乘法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_mul(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 乘法节点，左结合，先计算左节点，后计算右节点

    // 乘法的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 乘法的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    // TODO real number add

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new BinaryIRInst(IRInstOperator::IRINST_OP_MUL_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 整数除法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_div(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 除法节点，左结合，先计算左节点，后计算右节点

    // 除法的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 除法的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    // TODO real number add

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new BinaryIRInst(IRInstOperator::IRINST_OP_DIV_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 整数取余AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_mod(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 取余节点，左结合，先计算左节点，后计算右节点

    // 取余的左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 取余的右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    // TODO real number add

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_INT);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new BinaryIRInst(IRInstOperator::IRINST_OP_MOD_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 小于等于AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_le(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new CompareIRInst(IRInstOperator::IRINST_OP_LE_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 小于AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_lt(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new CompareIRInst(IRInstOperator::IRINST_OP_LT_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 大于AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_gt(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new CompareIRInst(IRInstOperator::IRINST_OP_GT_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 大于等于AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_ge(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new CompareIRInst(IRInstOperator::IRINST_OP_GE_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 不等于AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_ne(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new CompareIRInst(IRInstOperator::IRINST_OP_NE_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 等于AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_eq(ast_node * node)
{
    ast_node * src1_node = node->sons[0];
    ast_node * src2_node = node->sons[1];

    // 左边操作数
    ast_node * left = ir_visit_ast_node(src1_node);
    if (!left) {
        // 某个变量没有定值
        return false;
    }

    // 右边操作数
    ast_node * right = ir_visit_ast_node(src2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    Value * resultValue = symtab->currentFunc->newTempValue(BasicType::TYPE_BOOL);

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(new CompareIRInst(IRInstOperator::IRINST_OP_EQ_I, resultValue, left->val, right->val));
    node->val = resultValue;

    return true;
}

/// @brief 赋值AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_assign(ast_node * node)
{
    ast_node * son1_node = node->sons[0];
    ast_node * son2_node = node->sons[1];

    // 赋值节点，自右往左运算

    // 赋值运算符的左侧操作数
    ast_node * left = ir_visit_ast_node(son1_node);
    if (!left) {
        // 某个变量没有定值
        // 这里缺省设置变量不存在则创建，因此这里不会错误
        return false;
    }

    // 赋值运算符的右侧操作数
    ast_node * right = ir_visit_ast_node(son2_node);
    if (!right) {
        // 某个变量没有定值
        return false;
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理
    // TODO real number add

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);
    node->blockInsts.addInst(left->blockInsts);
    node->blockInsts.addInst(new AssignIRInst(left->val, right->val));

    // 这里假定赋值的类型是一致的
    left->val->type = right->val->type;
    node->val = left->val;

    return true;
}

/// @brief return节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_return(ast_node * node)
{
    ast_node * right = nullptr;

    // return语句可能没有没有表达式，也可能有，因此这里必须进行区分判断
    if (!node->sons.empty()) {

        ast_node * son_node = node->sons[0];

        // 返回的表达式的指令保存在right节点中
        right = ir_visit_ast_node(son_node);
        if (!right) {

            // 某个变量没有定值
            return false;
        }
    }

    // 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

    // 创建临时变量保存IR的值，以及线性IR指令
    node->blockInsts.addInst(right->blockInsts);

    // 返回值赋值到函数返回值变量上，然后跳转到函数的尾部
    node->blockInsts.addInst(new AssignIRInst(symtab->currentFunc->getReturnValue(), right->val));

    // 跳转到函数的尾部出口指令上
    node->blockInsts.addInst(new GotoIRInst(symtab->currentFunc->getExitLabel()));

    node->val = right->val;

    // 这里设置返回值类型
    ValueType & returnType = symtab->currentFunc->getReturnType();
    if (returnType.type == BasicType::TYPE_VOID) {
        // 设置类型
        returnType.type = right->val->type.type;
    } else if (returnType.type != right->val->type.type) {
        // 两者类型不一致，要出错显示
        // 或者隐式转换成更高的类型
        // TODO 这里目前什么都不做
    }

    return true;
}

/// @brief 标识符叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_var_id(ast_node * node)
{
    Value * val;

    // 新建一个ID型Value

    // 变量，则需要在符号表中查找对应的值
    // 若变量之前没有有定值，则采用默认的值为0

    val = symtab->currentFunc->findValue(node->name, false);
    if (!val) {

        // 变量不存在，则创建一个变量
        val = symtab->currentFunc->newVarValue(node->name);
    }

    node->val = val;

    return true;
}

/// @brief 无符号整数字面量叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_uint(ast_node * node)
{
    Value * val;

    // 新建一个整数常量Value
    val = symtab->newConstValue((int32_t) node->integer_val);

    node->val = val;

    return true;
}

/// @brief float数字面量叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_float(ast_node * node)
{
    Value * val = nullptr;

    // 新建一个实数型常量Value
    val = symtab->newConstValue(node->float_val);

    node->val = val;

    return true;
}

/// @brief 未知节点类型的节点处理
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_default(ast_node * node)
{
    // 未知的节点
    printf("Unkown node(%d)\n", (int) node->node_type);
    return true;
}

/// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
/// @param node AST节点
/// @return 成功返回node节点，否则返回nullptr
ast_node * IRGenerator::ir_visit_ast_node(ast_node * node)
{
    // 空节点
    if (nullptr == node) {
        return nullptr;
    }

    bool result;

    std::unordered_map<ast_operator_type, ast2ir_handler_t>::const_iterator pIter;
    pIter = ast2ir_handlers.find(node->node_type);
    if (pIter == ast2ir_handlers.end()) {
        // 没有找到，则说明当前不支持
        result = (this->ir_default)(node);
    } else {
        result = (this->*(pIter->second))(node);
    }

    if (!result) {
        // 语义解析错误，则出错返回
        node = nullptr;
    }

    return node;
}

/// @brief 遍历抽象语法树产生线性IR，保存到IRCode中
/// @param root 抽象语法树
/// @param IRCode 线性IR
/// @return true: 成功 false: 失败
bool IRGenerator::run()
{
    ast_node * node;

    // 从根节点进行遍历
    node = ir_visit_ast_node(root);

    return node != nullptr;
}
