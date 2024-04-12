%{
#include <cstdio>
#include <cstring>

// 词法分析头文件
#include "FlexLexer.h"

// bison生成的头文件
#include "BisonParser.h"

// 抽象语法树函数定义原型头文件
#include "AST.h"

// LR分析失败时所调用函数的原型声明
void yyerror(char * msg);

%}

// 联合体声明，用于后续终结符和非终结符号属性指定使用
%union {
    class ast_node * node;

    struct digit_int_attr integer_num;
    struct digit_real_attr float_num;
    struct var_id_attr var_id;
};

// 文法的开始符号
%start  CompileUnit

// 指定文法的终结符号，<>可指定文法属性
// 对于单个字符的算符或者分隔符，在词法分析时可直返返回对应的字符即可
%token <integer_num> T_DIGIT
%token <var_id> T_ID
%token T_FUNC T_RETURN T_ADD T_SUB

%type <node> CompileUnit

// 指定文法的非终结符号，<>可指定文法属性
%type <node> FuncDef
%type <node> FuncFormalParams
%type <node> Block

%type <node> FuncFormalParam
%type <node> FuncBasicParam

%type <node> BlockItemList
%type <node> BlockItem

%type <node> Statement
%type <node> Expr
%type <node> AddExp UnaryExp LVal
%type <node> PrimaryExp
%type <node> RealParamList

%%

/* 编译单元可包含若干个函数，main函数作为程序的入口，必须存在 */
CompileUnit : FuncDef {
        $$ = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT, $1);
        ast_root = $$;
    }
    | Statement {
        $$ = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT, $1);
        ast_root = $$;
    }
    | CompileUnit FuncDef {
        $$ = insert_ast_node($1, $2);
    }
    | CompileUnit Statement {
        $$ = insert_ast_node($1, $2);
    }
    ;

// 函数定义
FuncDef : T_FUNC T_ID '(' ')' Block  {
        $$ = create_func_def($2.lineno, $2.id, $5, nullptr);
    }
    | T_FUNC T_ID '(' FuncFormalParams ')' Block {
        $$ = create_func_def($2.lineno, $2.id, $6, $4);
    }
    ;

// 函数参数
FuncFormalParams : FuncFormalParam  {
        $$ = create_contain_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS, $1);
    }
    | FuncFormalParams ',' FuncFormalParam {
        $$ = insert_ast_node($1, $3);
    }
    ;

// 函数参数，目前只支持基本类型参数
FuncFormalParam : FuncBasicParam  {
        $$ = $1;
    }
    ;

// 基本类型函数参数，默认整型
FuncBasicParam : T_ID {
        $$ = create_func_formal_param($1.lineno, $1.id);
    }
    ;

// 语句块
Block : '{' '}' {
        // 语句块没有语句
        $$ = nullptr;
    }
    | '{' BlockItemList '}' {
        // 语句块含有语句
        $$ = $2;
    }
    ;

// 语句块内语句列表
BlockItemList : BlockItem {
        // 第一个左侧的孩子节点归约成Block父节点，后续语句可不断作为孩子追加到block中
        // 创建一个AST_OP_BLOCK类型的中间节点，孩子为Statement($1)
        $$ = new_ast_node(ast_operator_type::AST_OP_BLOCK, $1, nullptr);
    }
    | BlockItemList BlockItem  {
        // 采用左递归的文法产生式，可以使得Block节点在上个产生式创建，后续递归追加孩子节点
        // 请注意，不要采用右递归，左递归翻遍孩子的追加
        // BlockItem($2)作为Block($1)的孩子 
        $$ = insert_ast_node($1, $2);
    }
    ;

// 目前语句块内项目只能是语句
BlockItem : Statement  {
        $$ = $1;
    }
    ;

/* 语句 */
Statement : T_ID '=' Expr ';' {
        // 归约到Statement时要执行的语义动作程序
        // 赋值语句，不显示值

		// 变量节点
		ast_node * id_node = new_ast_leaf_node(var_id_attr{$1.id, $1.lineno});

		free($1.id);

        // 创建一个AST_OP_ASSIGN类型的中间节点，孩子为Id和Expr($3)
        $$ = new_ast_node(ast_operator_type::AST_OP_ASSIGN, id_node, $3, nullptr);
    }
    | Expr ';' {
        // Expr归约到Statement时要执行的语义动作程序
        // 表达式语句，不显示表达式的值

        // 创建一个AST_OP_EXPR类型的中间节点，孩子为Expr($1)
        $$ = new_ast_node(ast_operator_type::AST_OP_EXPR, $1, nullptr);
    }
    | Expr {
        // Expr归约到Statement时要执行的语义动作程序
        // 表达式语句，需要显示表达式的值

        // 创建一个AST_OP_EXPR_SHOW类型的中间节点，孩子为Expr($1)
        $$ = new_ast_node(ast_operator_type::AST_OP_EXPR_SHOW, $1, nullptr);
    }
    | T_RETURN Expr ';' {
        // 返回语句
        $$ = new_ast_node(ast_operator_type::AST_OP_RETURN_STATEMENT, $2, nullptr);
    }
    ;

Expr : AddExp {
        $$ = $1;
    }
    ;

/* 加法表达式 */
AddExp : AddExp T_ADD UnaryExp {
        /* Expr = Expr + Term */

        // 创建一个AST_OP_ADD类型的中间节点，孩子为Expr($1)和Term($3)
        $$ = new_ast_node(ast_operator_type::AST_OP_ADD, $1, $3, nullptr);
    }
    | AddExp T_SUB UnaryExp {
        /* Expr = Expr + Term */

        // 创建一个AST_OP_ADD类型的中间节点，孩子为Expr($1)和Term($3)
        $$ = new_ast_node(ast_operator_type::AST_OP_SUB, $1, $3, nullptr);
    }
    | UnaryExp {
        /* Expr = Term */
        $$ = $1;
    }
    ;

UnaryExp : PrimaryExp {
        $$ = $1;
    }
    | T_ID '(' ')' {
        // 用户自定义的不含实参的函数调用
        $$ = create_func_call($1.lineno, $1.id, nullptr);
    }
    | T_ID '(' RealParamList ')' {
        // 用户自定义的含有实参的参数调用
        $$ = create_func_call($1.lineno, $1.id, $3);
    }

PrimaryExp :  '(' Expr ')' {
        /* PrimaryExp = Expr */
        $$ = $2;
    }
    | T_DIGIT {
        // 无符号整数识别

        // 终结符作为抽象语法树的叶子节点进行创建
        $$ = new_ast_leaf_node(digit_int_attr{$1.val, $1.lineno});
    }
    | LVal  {
        // 左值
        $$ = $1;
    }
    ;

LVal : T_ID {
        // 终结符作为抽象语法树的叶子节点进行创建
        $$ = new_ast_leaf_node(var_id_attr{$1.id, $1.lineno});

		// 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
		free($1.id);
    }
/* 实参列表 */
RealParamList : Expr {
        $$ = create_contain_node(ast_operator_type::AST_OP_FUNC_REAL_PARAMS, $1);
    }
    | RealParamList ',' Expr {
        $$ = insert_ast_node($1, $3);
    }
    ;
%%

// 语法识别错误要调用函数的定义
void yyerror(char * msg)
{
    printf("Line %d: %s\n", yylineno, msg);
}
