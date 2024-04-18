/**
 * @file InstSelectorArm32.cpp
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 指令选择器-ARM32
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <cstdio>

#include "Function.h"
#include "ILocArm32.h"
#include "IRInst.h"
#include "InstSelectorArm32.h"
#include "PlatformArm32.h"
#include "Value.h"

/// @brief 构造函数
/// @param _irCode 指令
/// @param _iloc ILoc
/// @param _func 函数
InstSelectorArm32::InstSelectorArm32(vector<IRInst *> & _irCode, ILocArm32 & _iloc, Function * _func)
    : ir(_irCode), iloc(_iloc), func(_func)
{
    translator_handlers[IRInstOperator::IRINST_OP_ENTRY] = &InstSelectorArm32::translate_entry;
    translator_handlers[IRInstOperator::IRINST_OP_EXIT] = &InstSelectorArm32::translate_exit;

    translator_handlers[IRInstOperator::IRINST_OP_LABEL] = &InstSelectorArm32::translate_label;
    translator_handlers[IRInstOperator::IRINST_OP_GOTO] = &InstSelectorArm32::translate_goto;

    translator_handlers[IRInstOperator::IRINST_OP_ASSIGN] = &InstSelectorArm32::translate_assign;

    translator_handlers[IRInstOperator::IRINST_OP_ADD_I] = &InstSelectorArm32::translate_add_int32;
    translator_handlers[IRInstOperator::IRINST_OP_SUB_I] = &InstSelectorArm32::translate_sub_int32;
    translator_handlers[IRInstOperator::IRINST_OP_MUL_I] = &InstSelectorArm32::translate_mul_int32;
    translator_handlers[IRInstOperator::IRINST_OP_DIV_I] = &InstSelectorArm32::translate_div_int32;
    translator_handlers[IRInstOperator::IRINST_OP_MOD_I] = &InstSelectorArm32::translate_mod_int32;
    translator_handlers[IRInstOperator::IRINST_OP_NEG_I] = &InstSelectorArm32::translate_neg_int32;

    translator_handlers[IRInstOperator::IRINST_OP_FUNC_CALL] = &InstSelectorArm32::translate_call;
}

/// @brief 指令选择执行
void InstSelectorArm32::run()
{
    for (auto inst: ir) {

        // 逐个指令进行翻译
        if (!inst->isDead()) {
            translate(inst);
        }
    }
}

/// @brief NOP翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_nop(IRInst * inst)
{
    (void) inst;
    iloc.nop();
}

/// @brief Label指令指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_label(IRInst * inst)
{
    // iloc.label(inst->getLabel());
}

/// @brief goto指令指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_goto(IRInst * inst)
{}

/// @brief 函数入口指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_entry(IRInst * inst)
{
    // 查看保护的寄存器
    auto & protectedRegNo = func->getProtectedReg();
    auto & protectedRegStr = func->getProtectedRegStr();

    bool first = true;
    for (auto regno: protectedRegNo) {
        if (first) {
            protectedRegStr = PlatformArm32::regName[regno];
            first = false;
        } else if (!first) {
            protectedRegStr += "," + PlatformArm32::regName[regno];
        }
    }

    if (!protectedRegStr.empty()) {
        iloc.inst("push", "{" + protectedRegStr + "}");
    }

    // 为fun分配栈帧，含局部变量、函数调用值传递的空间等
    iloc.allocStack(func, REG_ALLOC_SIMPLE_TMP_REG_NO);
}

/// @brief 函数出口指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_exit(IRInst * inst)
{
    if (!inst->getSrc().empty()) {
        // 存在返回值
        Value * retVal = inst->getSrc1();

        // 赋值给寄存器R0
        iloc.load_var(0, retVal);
    }

    auto & protectedRegStr = func->getProtectedRegStr();

    // 恢复栈空间
    iloc.inst("add",
              PlatformArm32::regName[REG_ALLOC_SIMPLE_FP_REG_NO],
              PlatformArm32::regName[REG_ALLOC_SIMPLE_FP_REG_NO],
              iloc.toStr(func->getMaxDep()));

    iloc.inst("mov", "sp", PlatformArm32::regName[REG_ALLOC_SIMPLE_FP_REG_NO]);

    // 保护寄存器的恢复
    if (!protectedRegStr.empty()) {
        iloc.inst("pop", "{" + protectedRegStr + "}");
    }

    iloc.inst("bx", "lr");
}

/// @brief 赋值指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_assign(IRInst * inst)
{
    Value * rs = inst->getDst();
    Value * arg1 = inst->getSrc1();

    if (arg1->regId != -1) {
        // 寄存器 => 内存
        // 寄存器 => 寄存器

        // r8 -> rs 可能用到r9
        iloc.store_var(arg1->regId, rs, 9);

    } else if (rs->regId != -1) {
        // 内存变量 => 寄存器

        iloc.load_var(rs->regId, arg1);

    } else {
        // 内存变量 => 内存变量

        // arg1 -> r8
        iloc.load_var(REG_ALLOC_SIMPLE_SRC1_REG_NO, arg1);

        // r8 -> rs 可能用到r9
        iloc.store_var(REG_ALLOC_SIMPLE_SRC1_REG_NO, rs, 9);
    }
}

/// @brief 二元操作指令翻译成ARM32汇编
/// @param inst IR指令
/// @param operator_name 操作码
/// @param rs_reg_no 结果寄存器号
/// @param op1_reg_no 源操作数1寄存器号
/// @param op2_reg_no 源操作数2寄存器号
void InstSelectorArm32::translate_two_operator(IRInst * inst,
                                               string operator_name,
                                               int rs_reg_no,
                                               int op1_reg_no,
                                               int op2_reg_no)
{
    Value * rs = inst->getDst();
    Value * arg1 = inst->getSrc1();
    Value * arg2 = inst->getSrc2();

    std::string arg1_reg_name, arg2_reg_name;
    int arg1_reg_no = arg1->regId, arg2_reg_no = arg2->regId;

    // 看arg1是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg1_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op1_reg_no, arg1);
    } else if (arg1_reg_no != op1_reg_no) {
        // 已分配的操作数1的寄存器和操作数2的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  3    -1   有问题
        // 实际寄存器  3    3    有问题
        // 实际寄存器  3    4    无问题
        if ((arg1_reg_no == op2_reg_no) && ((arg2_reg_no == -1) || (arg2_reg_no == op2_reg_no))) {
            iloc.mov_reg(op1_reg_no, arg1_reg_no);
        } else {
            op1_reg_no = arg1_reg_no;
        }
    }

    arg1_reg_name = PlatformArm32::regName[op1_reg_no];

    // 看arg2是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg2_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op2_reg_no, arg2);
    } else if (arg2_reg_no != op2_reg_no) {
        // 已分配的操作数2的寄存器和操作数1的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  -1   2   有问题
        // 实际寄存器  2    2    有问题
        // 实际寄存器  4    2    无问题
        if ((arg2_reg_no == op1_reg_no) && ((arg1_reg_no == -1) || (arg1_reg_no == op1_reg_no))) {
            iloc.mov_reg(op2_reg_no, arg2_reg_no);
        } else {
            op2_reg_no = arg2_reg_no;
        }
    }

    arg2_reg_name = PlatformArm32::regName[op2_reg_no];

    // 看结果变量是否是寄存器，若不是则采用参数指定的寄存器rs_reg_name
    if (rs->regId != -1) {
        rs_reg_no = rs->regId;
    } else if (rs->isTemp()) {
        // 临时变量
        rs->regId = rs_reg_no;
    }

    std::string rs_reg_name = PlatformArm32::regName[rs_reg_no];

    iloc.inst(operator_name, rs_reg_name, arg1_reg_name, arg2_reg_name);

    // 结果不是寄存器，则需要把rs_reg_name保存到结果变量中
    if (rs->regId == -1) {
        // r8 -> rs 可能用到r9
        iloc.store_var(rs_reg_no, rs, op2_reg_no);
    }
}

/// @brief 一元操作指令翻译成ARM32汇编
/// @param inst IR指令
/// @param operator_name 操作码
/// @param rs_reg_no 结果寄存器号
/// @param op1_reg_no 源操作数1寄存器号
void InstSelectorArm32::translate_one_operator(IRInst * inst, string operator_name, int rs_reg_no, int op1_reg_no)
{
    Value * rs = inst->getDst();
    Value * arg1 = inst->getSrc1();

    std::string arg1_reg_name;
    int arg1_reg_no = arg1->regId;

    // 看arg1是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg1_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op1_reg_no, arg1);
    } else if (arg1_reg_no != op1_reg_no) {
        // 已分配的操作数1的寄存器和操作数2的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  3    -1   有问题
        // 实际寄存器  3    3    有问题
        // 实际寄存器  3    4    无问题
        iloc.mov_reg(op1_reg_no, arg1_reg_no);
    }

    arg1_reg_name = PlatformArm32::regName[op1_reg_no];

    // 如果结果寄存器不是-1，那么使用指定的寄存器
    if (rs->regId != -1) {
        rs_reg_no = rs->regId;
    } else if (rs->isTemp()) {
        // 如果结果是临时变量，则使用指定的寄存器
        rs->regId = rs_reg_no;
    }

    std::string rs_reg_name = PlatformArm32::regName[rs_reg_no];

    // 执行单目运算
    iloc.inst(operator_name, rs_reg_name, arg1_reg_name);

    // 如果结果不在寄存器中，则保存到结果变量中
    if (rs->regId == -1) {
        iloc.store_var(rs_reg_no, rs, op1_reg_no);
    }
}

/// @brief 整数加法指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_add_int32(IRInst * inst)
{
    translate_two_operator(inst, "add");
}

/// @brief 整数减法指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_sub_int32(IRInst * inst)
{
    translate_two_operator(inst, "sub");
}

/// @brief 整数乘法指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_mul_int32(IRInst * inst)
{
    translate_two_operator(inst, "mul");
}

/// @brief 整数除法指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_div_int32(IRInst * inst)
{
    translate_two_operator(inst, "div");
}

/// @brief 整数取余指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_mod_int32(IRInst * inst)
{
    translate_two_operator(inst, "mod");
}

/// @brief 整数求负指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_neg_int32(IRInst * inst)
{
    translate_one_operator(inst, "neg");
}

/// @brief 函数调用指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate_call(IRInst * inst)
{
    FuncCallIRInst * callInst = dynamic_cast<FuncCallIRInst *>(inst);
    iloc.call_fun(callInst->name);
}

/// @brief 指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm32::translate(IRInst * inst)
{
    // 操作符
    IRInstOperator op = inst->getOp();

    map<IRInstOperator, translate_handler>::const_iterator pIter;
    pIter = translator_handlers.find(op);
    if (pIter == translator_handlers.end()) {
        // 没有找到，则说明当前不支持
        printf("Translate: Operator(%d) not support", (int) op);
        return;
    }

    (this->*(pIter->second))(inst);
}
