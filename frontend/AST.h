/**
 * @file ast.h
 * @author zenglj (zenglj@nwpu.edu.cn)
 * @brief 抽象语法树AST管理的源文件
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "AttrType.h"
#include "IRCode.h"
#include "Value.h"

/// @brief AST节点的类型
enum class ast_operator_type : int {

    /* 以下为AST的叶子节点 */

    /// @brief 无符号整数字面量叶子节点
    AST_OP_LEAF_LITERAL_UINT,

    /// @brief  无符号整数字面量叶子节点
    AST_OP_LEAF_LITERAL_FLOAT,

    /// @brief 变量ID叶子节点
    AST_OP_LEAF_VAR_ID,

    /// @brief 复杂类型的节点
    AST_OP_LEAF_TYPE,

    /* 以下为AST的内部节点，含根节点 */
    /// @brief 二元运算符+
    AST_OP_ADD,

    /// @brief 二元运算符*
    AST_OP_SUB, //

    /// @brief 多个语句组成的块运算符
    AST_OP_BLOCK,

    /// @brief 赋值语句运算符
    AST_OP_ASSIGN,

    /// @brief 表达式语句运算符，不显示表达式的值
    AST_OP_EXPR,

    /// @brief 表达式显示语句运算符，需要显示表达式的值
    AST_OP_EXPR_SHOW,

    /// @brief return语句运算符
    AST_OP_RETURN_STATEMENT,

    /// @brief 函数定义运算符，函数名和返回值类型作为节点的属性，自左到右孩子：AST_OP_FUNC_FORMAL_PARAMS、AST_OP_BLOCK
    AST_OP_FUNC_DEF,

    /// @brief 形式参数列表运算符，可包含多个孩子：AST_OP_FUNC_FORMAL_PARAM
    AST_OP_FUNC_FORMAL_PARAMS,

    /// @brief 形参运算符，属性包含名字与类型，复杂类型时可能要包含孩子
    AST_OP_FUNC_FORMAL_PARAM,

    /// @brief 函数调用运算符，函数名作为节点属性，孩子包含AST_OP_FUNC_REAL_PARAMS
    AST_OP_FUNC_CALL,

    /// @brief 实际参数列表运算符，可包含多个表达式AST_OP_EXPR
    AST_OP_FUNC_REAL_PARAMS,

    /// @brief 文件编译单元运算符，可包含函数定义、语句块等孩子
    AST_OP_COMPILE_UNIT,

    /// @brief 二元运算符*
    AST_OP_MUL,

    /// @brief 二元运算符/
    AST_OP_DIV,

    /// @brief 二元运算符%
    AST_OP_MOD,

    /// @brief 小于等于运算符
    AST_OP_LE,

    /// @brief 小于运算符
    AST_OP_LT,

    /// @brief 大于运算符
    AST_OP_GT,

    /// @brief 大于等于运算符
    AST_OP_GE,

    /// @brief 不等于运算符
    AST_OP_NE,

    /// @brief 等于运算符
    AST_OP_EQ,

    /// @brief int类型
    AST_OP_TYPE_INT,

    /// @brief 变量定义
    AST_OP_VAR_DECL,

    /// @brief 多个变量定义列表
    AST_OP_VAR_DECLS,

    /// @brief 逻辑与运算符
    AST_OP_AND,

    /// @brief 逻辑非运算符
    AST_OP_NOT,

    /// @brief if语句运算符
    AST_OP_IF,

    /// @brief while语句运算符
    AST_OP_WHILE,
    // TODO 抽象语法树其它内部节点运算符追加

    /// @brief 最大标识符，表示非法运算符
    AST_OP_MAX,
};

/// @brief 抽象语法树AST的节点描述类
class ast_node {
public:
    /// @brief 父节点
    ast_node * parent;

    /// @brief 孩子节点
    std::vector<ast_node *> sons;

    /// @brief 节点类型
    ast_operator_type node_type;

    /// @brief 行号信息，主要针对叶子节点有用
    uint32_t line_no;

    /// @brief 节点值的类型，可用于函数返回值类型
    ValueType type;

    /// @brief 无符号整数字面量值
    uint32_t integer_val;

    /// @brief float类型字面量值
    float float_val;

    /// @brief 变量名，或者函数名
    std::string name;

    /// @brief 线性IR指令块，可包含多条IR指令，用于线性IR指令产生用
    InterCode blockInsts;

    /// @brief 线性IR指令或者运行产生的Value，用于线性IR指令产生用
    Value * val;

    /// @brief 构造函数
    /// @param _type 节点值的类型
    /// @param line_no 行号
    ast_node(ValueType _type, int32_t _line_no);

    /// @brief 针对无符号整数字面量的构造函数
    /// @param attr 无符号整数字面量
    ast_node(digit_int_attr attr);

    /// @brief 针对float字面量的构造函数
    /// @param attr float型实数字面量
    ast_node(digit_real_attr attr);

    /// @brief 针对标识符ID的叶子构造函数
    /// @param attr 字符型标识符
    ast_node(var_id_attr attr);

    /// @brief 创建指定节点类型的节点
    /// @param _node_type 节点类型
    /// @param _line_no 行号
    ast_node(ast_operator_type _node_type, int32_t _line_no = -1);
};

/// @brief 判断是否是叶子节点
/// @param type 节点类型
/// @return true：是叶子节点 false：内部节点
bool isLeafNode(ast_operator_type type);

/// @brief 创建指定节点类型的节点
/// @param type 节点类型
/// @param  可变参数，最后一个孩子节点必须指定为nullptr。如果没有孩子，则指定为nullptr
/// @return 创建的节点
ast_node * new_ast_node(ast_operator_type type, ...);

/// @brief 向父节点插入一个节点
/// @param parent 父节点
/// @param node 节点
ast_node * insert_ast_node(ast_node * parent, ast_node * node);

/// @brief 创建无符号整数的叶子节点
/// @param val 词法值
/// @param line_no 行号
ast_node * new_ast_leaf_node(digit_int_attr attr);

/// @brief 创建实数的叶子节点
/// @param val 词法值
/// @param line_no 行号
ast_node * new_ast_leaf_node(digit_real_attr attr);

/// @brief 创建标识符的叶子节点
/// @param val 词法值
/// @param line_no 行号
ast_node * new_ast_leaf_node(var_id_attr attr);

/// @brief 创建具备指定类型的节点
/// @param type 节点值类型
/// @param line_no 行号
/// @return 创建的节点
ast_node * new_ast_leaf_node(BasicType type, int32_t line_no);

/// @brief AST资源清理
void free_ast();

/// @brief抽象语法树的根节点指针
extern ast_node * ast_root;

/// @brief 创建函数定义类型的内部AST节点
/// @param line_no 行号
/// @param func_name 函数名
/// @param block 函数体语句块
/// @param params 函数形参，可以没有参数
/// @return 创建的节点
ast_node * create_func_def(uint32_t line_no, const char * func_name, ast_node * block, ast_node * params = nullptr);

/// @brief 创建变量定义类型的内部AST节点
/// @param line_no 行号
/// @param var_name 变量名
/// @param value 变量初始值，可以没有值
/// @return 创建的节点
ast_node * create_var_decl(uint32_t line_no, const char * var_name, ast_node * var_type, ast_node * value = nullptr);

/// @brief 创建函数形式参数的节点
/// @param line_no 行号
/// @param param_name 参数名
/// @return 创建的节点
ast_node * create_func_formal_param(uint32_t line_no, const char * param_name);

/// @brief 创建AST的内部节点
/// @param node_type 节点类型
/// @param first_param 第一个孩子节点
/// @return 创建的节点
ast_node * create_contain_node(ast_operator_type node_type, ast_node * first_param = nullptr);

/// @brief 创建函数调用的节点
/// @param line_no 行号
/// @param func_name 被调用的函数名
/// @param params 实参节点
/// @return 创建的节点
ast_node * create_func_call(uint32_t line_no, const char * func_name, ast_node * params = nullptr);
