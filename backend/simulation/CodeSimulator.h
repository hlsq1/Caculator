/**
 * @file CodeSimulator.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 计算器解释执行处理头文件
 * @version 0.1
 * @date 2023-09-26
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <unordered_map>

#include "CodeGenerator.h"

/// @brief 中间代码指令的模拟/解释运行
class CodeSimulator : public CodeGenerator {

public:
    /// @brief 构造函数
    /// @param tab 符号表
    CodeSimulator(SymbolTable & tab);

    /// @brief 析构函数
    ~CodeSimulator() override = default;

    /// @brief 中间代码指令的模拟运行
    /// @param fp 输出内容所在文件
    /// @return true：成功，false：失败
    bool run() override;

protected:
    /// @brief 函数入口指令计算
    /// @param inst IR中间指令
    bool calc_entry(IRInst * inst);

    /// @brief 函数出口指令计算
    /// @param inst IR中间指令
    bool calc_exit(IRInst * inst);

    /// @brief 赋值指令计算
    /// @param inst IR中间指令
    bool calc_assign(IRInst * inst);

    /// @brief Label指令计算
    /// @param inst IR中间指令
    bool calc_label(IRInst * inst);

    /// @brief 整数加法指令计算
    /// @param inst IR中间指令
    bool calc_add_int32(IRInst * inst);

    /// @brief 整数减法指令计算
    /// @param inst IR中间指令
    bool calc_sub_int32(IRInst * inst);

    /// @brief 整数乘法指令计算
    /// @param inst IR中间指令
    bool calc_mul_int32(IRInst * inst);

    /// @brief 整数除法指令计算
    /// @param inst IR中间指令
    bool calc_div_int32(IRInst * inst);

    /// @brief 整数取余指令计算
    /// @param inst IR中间指令
    bool calc_mod_int32(IRInst * inst);

    /// @brief 整数求负指令计算
    /// @param inst IR中间指令
    bool calc_neg_int32(IRInst * inst);

    /// @brief GOTO指令计算
    /// @param inst IR中间指令
    bool calc_goto(IRInst * inst);

    /// @brief 函数调用指令计算
    /// @param inst IR中间指令
    bool calc_call(IRInst * inst);

    /// @brief 指令解释执行，返回的结果报告到inst的destValue中
    /// @param inst IR指令
    bool IRInstCalc(IRInst * inst);

    /// @brief 中间指令动作执行函数原型
    typedef bool (CodeSimulator::*calc_handler)(IRInst *);

    /// @brief IR动作解释执行指令清单
    std::unordered_map<IRInstOperator, calc_handler> calc_handlers;

    /// @brief 保存自定义函数目的操作数
    Value * Result = nullptr;
};
