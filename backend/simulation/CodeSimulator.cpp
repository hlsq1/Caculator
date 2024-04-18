/**
 * @file CodeSimulator.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 计算器解释执行处理的源文件
 * @version 0.1
 * @date 2023-09-26
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <cstdio>

#include "CodeGenerator.h"
#include "CodeSimulator.h"
#include "IRCode.h"
#include "IRInst.h"
#include "SymbolTable.h"
#include "Value.h"

/// @brief 构造函数
/// @param tab 符号表
CodeSimulator::CodeSimulator(SymbolTable & tab) : CodeGenerator(tab)
{
    calc_handlers[IRInstOperator::IRINST_OP_ENTRY] = &CodeSimulator::calc_entry;
    calc_handlers[IRInstOperator::IRINST_OP_EXIT] = &CodeSimulator::calc_exit;

    calc_handlers[IRInstOperator::IRINST_OP_LABEL] = &CodeSimulator::calc_label;
    calc_handlers[IRInstOperator::IRINST_OP_GOTO] = &CodeSimulator::calc_goto;

    calc_handlers[IRInstOperator::IRINST_OP_ASSIGN] = &CodeSimulator::calc_assign;

    calc_handlers[IRInstOperator::IRINST_OP_ADD_I] = &CodeSimulator::calc_add_int32;
    calc_handlers[IRInstOperator::IRINST_OP_SUB_I] = &CodeSimulator::calc_sub_int32;
    calc_handlers[IRInstOperator::IRINST_OP_MUL_I] = &CodeSimulator::calc_mul_int32;
    calc_handlers[IRInstOperator::IRINST_OP_DIV_I] = &CodeSimulator::calc_div_int32;
    calc_handlers[IRInstOperator::IRINST_OP_MOD_I] = &CodeSimulator::calc_mod_int32;
    calc_handlers[IRInstOperator::IRINST_OP_NEG_I] = &CodeSimulator::calc_neg_int32;

    calc_handlers[IRInstOperator::IRINST_OP_FUNC_CALL] = &CodeSimulator::calc_call;
}

/// @brief 函数入口指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_entry(IRInst * inst)
{
    // 目前什么都不做
    return true;
}

/// @brief 函数出口指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_exit(IRInst * inst)
{
    // TODO 函数调用支持时这里必须进行值的返回

    return true;
}

/// @brief 赋值指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_assign(IRInst * inst)
{
    // 目前只支持整数运算
    Value * destVal = inst->getDst();
    Value * srcVal = inst->getSrc1();

    destVal->intVal = srcVal->intVal;
    destVal->type = srcVal->type;

    return true;
}

/// @brief Label指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_label(IRInst * inst)
{
    // 目前可忽略
    // TODO 如果支持自定义的函数调用等功能时需要自己实现

    return true;
}

/// @brief 整数加法指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_add_int32(IRInst * inst)
{
    Value * destVal = inst->getDst();
    Value * srcVal1 = inst->getSrc1();
    Value * srcVal2 = inst->getSrc2();

    destVal->intVal = srcVal1->intVal + srcVal2->intVal;
    destVal->type.type = BasicType::TYPE_INT;

    return true;
}

/// @brief 整数减法指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_sub_int32(IRInst * inst)
{
    Value * destVal = inst->getDst();
    Value * srcVal1 = inst->getSrc1();
    Value * srcVal2 = inst->getSrc2();

    destVal->intVal = srcVal1->intVal - srcVal2->intVal;
    destVal->type.type = BasicType::TYPE_INT;

    return true;
}

/// @brief 整数乘法指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_mul_int32(IRInst * inst)
{
    Value * destVal = inst->getDst();
    Value * srcVal1 = inst->getSrc1();
    Value * srcVal2 = inst->getSrc2();

    destVal->intVal = srcVal1->intVal * srcVal2->intVal;
    destVal->type.type = BasicType::TYPE_INT;

    return true;
}

/// @brief 整数除法指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_div_int32(IRInst * inst)
{
    Value * destVal = inst->getDst();
    Value * srcVal1 = inst->getSrc1();
    Value * srcVal2 = inst->getSrc2();

    destVal->intVal = srcVal1->intVal / srcVal2->intVal;
    destVal->type.type = BasicType::TYPE_INT;

    return true;
}

/// @brief 整数取余指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_mod_int32(IRInst * inst)
{
    Value * destVal = inst->getDst();
    Value * srcVal1 = inst->getSrc1();
    Value * srcVal2 = inst->getSrc2();

    destVal->intVal = srcVal1->intVal % srcVal2->intVal;
    destVal->type.type = BasicType::TYPE_INT;

    return true;
}

/// @brief 整数求负指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_neg_int32(IRInst * inst)
{
    Value * destVal = inst->getDst();
    Value * srcVal1 = inst->getSrc1();

    destVal->intVal = -srcVal1->intVal;
    destVal->type.type = BasicType::TYPE_INT;

    return true;
}

/// @brief GOTO指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_goto(IRInst * inst)
{
    // 无条件跳转指令

    // TODO 需要自己实现
    printf("goto not support");

    return false;
}

/// @brief 函数调用指令计算
/// @param inst IR中间指令
bool CodeSimulator::calc_call(IRInst * inst)
{
    // TODO 自定义函数需要自己实现
    FuncCallIRInst * funcCallInst = dynamic_cast<FuncCallIRInst *>(inst);

    // 根据函数名查找函数信息
    Function * func = symtab.findFunction(funcCallInst->name);
    if (!func) {
        // 函数没有找到，则错误返回
        printf("Function(%s) not found\n", funcCallInst->name.c_str());
        return false;
    }

    // 目前支持内置函数
    if (!func->isBuiltin()) {
        // TODO 目前不支持，需要自己实现
        printf("User self-defined function(%s) not support to be called\n", funcCallInst->name.c_str());
        return false;
    }

    // 内置函数只支持putint函数
    if (funcCallInst->name != "putint") {
        // TODO 其它内置函数需要自己支持
        printf("Builtin function(%s) not support to be called\n", funcCallInst->name.c_str());
        return false;
    }

    printf("%d", funcCallInst->getSrc1()->intVal);

    return true;
}

/// @brief 指令解释执行，返回的结果报告到inst的destValue中
/// @param inst IR指令
bool CodeSimulator::IRInstCalc(IRInst * inst)
{
    // 操作符
    const IRInstOperator instOp = inst->getOp();

    std::unordered_map<IRInstOperator, calc_handler>::const_iterator pIter;
    pIter = calc_handlers.find(instOp);
    if (pIter == calc_handlers.end()) {
        // 没有找到，则说明当前不支持
        printf("IRInstCalc: Operator(%d) not support", (int) instOp);
        return false;
    }

    bool result = (this->*(pIter->second))(inst);

    return result;
}

/// @brief 中间代码指令的模拟运行
/// @param outFileName 输出内容所在文件
/// @return true：成功，false：失败
bool CodeSimulator::run()
{
    bool result = true;

    // 查找main函数进行执行
    Function * mainFunc = symtab.findFunction("main");
    if (!mainFunc) {
        // main函数查找不到，则出错
        printf("main function not found\n");
        return false;
    }

    // 开始遍历函数内的指令，开始解释执行，得出结果
    InterCode & irCode = mainFunc->getInterCode();
    for (auto & inst: irCode.getInsts()) {

        // 单条指令解释执行
        result = IRInstCalc(inst);
        if (!result) {
            break;
        }
    }

    return result;
}
